/* 
 * File:   Pokemon.cpp
 * Author: Catherine
 *
 * Created on March 28, 2009, 12:12 AM
 *
 * This file is a part of Shoddy Battle.
 * Copyright (C) 2009  Catherine Fitzpatrick and Benjamin Gwin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, visit the Free Software Foundation, Inc.
 * online at http://gnu.org.
 */

#include <iostream>
#include <list>

#include "Pokemon.h"
#include "PokemonSpecies.h"
#include "../mechanics/PokemonNature.h"
#include "../mechanics/PokemonType.h"
#include "../mechanics/JewelMechanics.h"
#include "../text/Text.h"
#include "../scripting/ScriptMachine.h"
#include "BattleField.h"

using namespace shoddybattle;
using namespace std;
using namespace boost;

namespace shoddybattle {

Pokemon::Pokemon(const PokemonSpecies *species,
        const string &nickname,
        const PokemonNature *nature,
        const string &ability,
        const string &item,
        const int *iv,
        const int *ev,
        const int level,
        const int gender,
        const bool shiny,
        const vector<string> &moves,
        const vector<int> &ppUps) {
    memcpy(m_iv, iv, sizeof(int) * STAT_COUNT);
    memcpy(m_ev, ev, sizeof(int) * STAT_COUNT);
    m_species = species;
    m_nickname = nickname;
    if (m_nickname.empty()) {
        m_nickname = m_species->getSpeciesName();
    } else if (m_nickname.length() > 19) {
        m_nickname = m_nickname.substr(0, 19);
    }
    m_nature = nature;
    m_types = species->getTypes();
    m_level = level;
    m_gender = gender;
    m_ppUps = ppUps;
    m_abilityName = ability;
    m_itemName = item;
    m_shiny = shiny;
    std::vector<std::string>::const_iterator i = moves.begin();
    for (; i != moves.end(); ++i) {
        const MoveTemplate *move = species->getMove(*i);
        if (move) {
            m_moveProto.push_back(move);
        }
    }
    m_pp.resize(m_moveProto.size());
    m_moveUsed.resize(m_moveProto.size(), false);
    m_machine = NULL;
    m_cx = NULL;
    m_field = NULL;
    m_object = NULL;
    m_fainted = false;
    m_item = NULL;
    m_ability = NULL;
    m_legalSwitch = true;
    m_slot = -1;
}

Pokemon::~Pokemon() {
    if (!m_machine)
        return;

    ScriptContext *cx = m_machine->acquireContext();

    if (m_object) {
        cx->removeRoot(m_object);
    }

    vector<MoveObject *>::iterator i = m_moves.begin();
    for (; i != m_moves.end(); ++i) {
        cx->removeRoot(*i);
    }

    STATUSES::iterator j = m_effects.begin();
    for (; j != m_effects.end(); ++j) {
        cx->removeRoot(*j);
    }

    m_machine->releaseContext(cx);
}

string Pokemon::getSpeciesName() const {
    return m_species->getSpeciesName();
}

int Pokemon::getSpeciesId() const {
    return m_species->getSpeciesId();
}

unsigned int Pokemon::getBaseStat(const STAT i) const {
    return m_species->getBaseStat(i);
}

/**
 * Determine the legal actions a pokemon can take this turn.
 */
void Pokemon::determineLegalActions() {
    // todo: can this pokemon switch legally?

    const int count = m_moves.size();
    m_legalMove.resize(count, false);
    for (int i = 0; i < count; ++i) {
        MoveObject *move = m_moves[i];
        if (move) {
            m_legalMove[i] = !m_field->vetoSelection(this, move);
        }
    }
}

/**
 * Determine whether the selection of a move should be vetoed.
 */
bool Pokemon::vetoSelection(Pokemon *user, MoveObject *move) {
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        if ((*i)->vetoSelection(m_cx, user, move))
            return true;
    }
    return false;
}

/**
 * Determine whether the execution of a move should be vetoed, according to
 * the effects on this pokemon.
 */
bool Pokemon::vetoExecution(Pokemon *user, Pokemon *target, MoveObject *move) {
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        if ((*i)->vetoExecution(m_cx, m_field, user, target, move))
            return true;
    }
    return false;
}

/**
 * Send this pokemon out onto the field.
 */
void Pokemon::switchIn(const int slot) {
    m_slot = slot;

    // todo: apply field + party effects?

    // inform status effects of switching in
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        (*i)->switchIn(m_cx);
    }
}

/**
 * Switch this pokemon out of the field.
 */
void Pokemon::switchOut() {
    // Remove effects that do not survive switches.
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        if ((*i)->switchOut(m_cx)) {
            removeStatus(*i);
        }
    }
    // Restore original ability.
    setAbility(m_abilityName);
    // Indicate that the pokemon is no longer active.
    m_slot = -1;
    // Clear this pokemon's memory.
    m_memory.clear();
    m_moveUsed.clear();
    m_moveUsed.resize(m_moves.size(), false);
    // Adjust the memories other active pokemon.
    shared_ptr<PokemonParty> *active = m_field->getActivePokemon();
    for (int i = 0; i < TEAM_COUNT; ++i) {
        PokemonParty &party = *active[i];
        const int size = party.getSize();
        for (int j = 0; j < size; ++j) {
            Pokemon::PTR p = party[j].pokemon;
            if (p) {
                p->removeMemory(this);
            }
        }
    }
}

/**
 * Get a status effect by ID.
 */
StatusObject *Pokemon::getStatus(const string &id) {
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if ((*i)->isRemovable(m_cx))
            continue;

        if ((*i)->getId(m_cx) == id)
            return *i;
    }
    return NULL;
}

/**
 * Execute an arbitrary move on a particular target.
 */
bool Pokemon::useMove(MoveObject *move,
        Pokemon *target, const int targets) {
    if (m_field->vetoExecution(this, target, move)) {
        // vetoed for this target
        return false;
    }
    if (move->attemptHit(m_cx, m_field, this, target)) {
        move->use(m_cx, m_field, this, target, targets);
    } else {
        vector<string> args;
        args.push_back(getName());
        args.push_back(target->getName());
        TextMessage msg(4, 2, args); // attack missed
        m_field->print(msg);
    }
    return true;
}

/**
 * Force a pokemon to carry out a particular turn next round.
 */
void Pokemon::setForcedTurn(const PokemonTurn &turn) {
    m_forcedTurn = shared_ptr<PokemonTurn>(new PokemonTurn(turn));
}

/**
 * Transform a stat level according to this pokemon.
 */
bool Pokemon::getTransformedStatLevel(Pokemon *user, Pokemon *target,
        STAT stat, int *level) {
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if ((*i)->isRemovable(m_cx))
            continue;

        if ((*i)->transformStatLevel(m_cx, user, target, stat, level))
            return true;
    }
    return false;
}

/**
 * Get the effective value of a stat.
 */
unsigned int Pokemon::getStat(const STAT stat) {
    PRIORITY_MAP mods;
    m_field->getStatModifiers(stat, *this, mods);
    mods[0] = getStatMultiplier(stat, m_statLevel[stat]);
    int value = getRawStat(stat);
    const int count = mods.size();
    for (int i = 0; i <= count; ++i) {
        value *= mods[i];
    }
    return value;
}

/**
 * Execute an arbitrary move on a set of targets.
 */
bool Pokemon::executeMove(MoveObject *move,
        Pokemon *target, bool inform) {

    // TODO: research if this should be inside an inform check
    if (m_field->vetoExecution(this, NULL, move)) {
        // vetoed
        return false;
    }

    m_field->informUseMove(this, move);

    if (move->getFlag(m_cx, F_UNIMPLEMENTED)) {
        cout << "But it's unimplemented..." << endl;
        return false;
    }

    TARGET tc = move->getTargetClass(m_cx);

    if ((tc == T_USER) || (tc == T_ALLY) || (tc == T_ALLIES)) {
        move->use(m_cx, m_field, this, NULL, 0);
        return true;
    }
    
    // Build a list of targets.
    vector<Pokemon *> targets;
    m_field->getTargetList(tc, targets, this, target);

    if (tc == T_LAST_ENEMY) {
        move->use(m_cx, m_field, this, targets[0], 0);
        return true;
    }

    int targetCount = targets.size();
    if (targetCount == 0) {
        m_field->print(TextMessage(4, 3)); // no target
        return true;
    }

    if (inform) {
        for (vector<Pokemon *>::iterator i = targets.begin();
                i != targets.end(); ++i) {
            (*i)->informTargeted(this, move);
        }
    }

    BattleField::EXECUTION entry = { this, move };
    m_field->pushExecution(entry);

    if (isEnemyTarget(tc)) {
        for (vector<Pokemon *>::iterator i = targets.begin();
                i != targets.end(); ++i) {
            useMove(move, *i, targetCount);
            if ((*i)->isFainted()) {
                --targetCount;
            }
        }
    } else {
        // no target as such
        move->use(m_cx, m_field, this, NULL, 0);
    }

    m_field->popExecution();

    return true;
}

/**
 * Remove one of this pokemon's memories, perhaps due to a pokemon leaving
 * the field.
 */
void Pokemon::removeMemory(Pokemon *pokemon) {
    MEMORY entry = { pokemon, NULL };
    m_memory.remove(entry);
}

/**
 * Set one of this pokemon's moves to a different move.
 */
void Pokemon::setMove(const int i, MoveObject *move, const int pp) {
    if (m_moves.size() <= i) {
        m_moves.resize(i + 1, NULL);
        m_pp.resize(i + 1, 0);
        m_moveUsed.resize(i + 1, false);
    }
    MoveObject *p = m_moves[i];
    if (p) {
        ScriptContext *cx = m_field->getContext();
        cx->removeRoot(p);
    }
    m_moves[i] = move;
    m_pp[i] = pp;
}

/**
 * Set one of this pokemon's moves to a different move, by name. This
 * function will probably only be used for testing, as actual logic is more
 * likely to be use the MoveObject * version above.
 */
void Pokemon::setMove(const int i, const string &name, const int pp) {
    ScriptContext *cx = m_field->getContext();
    MoveDatabase *moves = m_machine->getMoveDatabase();
    const MoveTemplate *tpl = moves->getMove(name);
    assert(tpl);
    MoveObject *move = cx->newMoveObject(tpl);
    setMove(i, move, pp);
}

/**
 * Remove defunct statuses from this pokemon.
 */
void Pokemon::removeStatuses() {
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if (!(*i)->isRemovable(m_cx))
            continue;

        // this is a list so it is safe to remove an arbitrary element
        m_cx->removeRoot(*i);
        m_effects.erase(i);
    }
}

/**
 * Return whether the pokemon has the specified ability.
 */
bool Pokemon::hasAbility(const string &name) {
    if (m_ability == NULL)
        return false;
    return (m_ability->getId(m_cx) == name);
}

/**
 * Apply a StatusEffect to this pokemon. Makes a copy of the parameter before
 * applying it to the pokemon.
 */
StatusObject *Pokemon::applyStatus(Pokemon *inducer, StatusObject *effect) {
    if (!effect)
        return NULL;

    // todo: locks

    if (effect->isSingleton(m_cx)) {
        StatusObject *singleton = getStatus(effect->getId(m_cx));
        if (singleton)
            return singleton;
    }

    StatusObject *applied = effect->cloneAndRoot(m_cx);
    if (inducer != NULL) {
        applied->setInducer(m_cx, inducer);
    }
    applied->setSubject(m_cx, this);

    m_field->transformStatus(this, &applied);
    if (applied == NULL) {
        return NULL;
    }

    if (!applied->applyEffect(m_cx)) {
        m_cx->removeRoot(applied);
        return NULL;
    }

    m_effects.push_back(applied);
    return applied;
}

/**
 * Transform a StatusEffect.
 */
bool Pokemon::transformStatus(Pokemon *subject, StatusObject **status) {
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        if ((*i)->transformStatus(m_cx, subject, status)) {
            if (*status == NULL) {
                return true;
            }
        }
    }
    return true;
}

/**
 * Remove a StatusEffect from this pokemon.
 */
void Pokemon::removeStatus(StatusObject *status) {
    status->unapplyEffect(m_cx);
    status->dispose(m_cx);
}

/**
 * Get critical hit chance (additive) modifiers.
 */
int Pokemon::getCriticalModifier() const {
    int ret = 0;
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        ret += (*i)->getCriticalModifier(m_cx);
    }
    return ret;
}

/**
 * Transform a health change.
 */
int Pokemon::transformHealthChange(int hp, bool indirect) const {
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        (*i)->transformHealthChange(m_cx, hp, indirect, &hp);
    }
    return hp;
}

/**
 * Check if this Pokemon has "inherent priority". This is used for certain
 * items and abilities.
 */
int Pokemon::getInherentPriority() const {
    int ret = 0;
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        int v = (*i)->getInherentPriority(m_cx);
        if (abs(v) > abs(ret)) {
            ret = v;
        }
    }
    return ret;
}

/**
 * Check for stat modifiers on all status effects.
 */
void Pokemon::getStatModifiers(STAT stat,
        Pokemon *subject, PRIORITY_MAP &mods) {
    MODIFIER mod;
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        if ((*i)->getStatModifier(m_cx, m_field, stat, subject, mod)) {
            // position unused
            mods[mod.priority] = mod.value;
        }
    }
}

/**
 * Check for modifiers on all status effects.
 */
void Pokemon::getModifiers(Pokemon *user, Pokemon *target,
        MoveObject *obj, const bool critical, MODIFIERS &mods) {
    MODIFIER mod;
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        if ((*i)->getModifier(m_cx, m_field,
                user, target, obj, critical, mod)) {
            mods[mod.position][mod.priority] = mod.value;
        }
    }
}

/**
 * Cause this pokemon to faint.
 */
void Pokemon::faint() {
    m_field->informFainted(this);
    m_fainted = true;
    //switchOut();
}

/**
 * Set the current hp of the pokemon, and also inform the BattleField, which
 * can cause side effects such as the printing of messages.
 */
void Pokemon::setHp(const int hp, const bool indirect) {
    // TODO: implement this function properly
    if (m_fainted) {
        return;
    }
    const int delta = transformHealthChange(m_hp - hp, indirect);
    m_hp -= delta;
    m_field->informHealthChange(this, delta);
    const BattleField::EXECUTION *move = m_field->topExecution();
    if (move) {
        informDamaged(move->user, move->move, delta);
    }
    if (m_hp <= 0) {
        faint();
    }
}

/**
 * Get the last pokemon that targeted this pokemon with a move.
 */
Pokemon *Pokemon::getMemoryPokemon() const {
    if (m_memory.empty())
        return NULL;
    return m_memory.back().user;
}

/**
 * Get this pokemon's memory of a move that targeted it.
 */
const MoveTemplate *Pokemon::getMemory() const {
    if (m_memory.empty())
        return NULL;
    return m_memory.back().move;
}

/**
 * Inform that this pokemon was damaged by a move.
 */
void Pokemon::informDamaged(Pokemon *user, MoveObject *move, int damage) {
    RECENT_DAMAGE entry = { user, move->getTemplate(m_cx), damage };
    m_recent.push(entry);
}

/**
 * Inform that this pokemon was targeted by a move.
 */
void Pokemon::informTargeted(Pokemon *user, MoveObject *move) {
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if (!(*i)->isActive(m_cx))
            continue;

        (*i)->informTargeted(m_cx, user, move);
    }

    if (move->getFlag(m_cx, F_MEMORABLE)) {
        list<MEMORY>::iterator i = m_memory.begin();
        for (; i != m_memory.end(); ++i) {
            if (i->user == user) {
                m_memory.erase(i);
                break;
            }
        }
        const MEMORY entry = { user, move->getTemplate(m_cx) };
        m_memory.push_back(entry);
    }
}

/**
 * Set this pokemon's ability.
 */
void Pokemon::setAbility(StatusObject *obj) {
    ScriptContext *cx = m_field->getContext();
    if (m_ability != NULL) {
        removeStatus(m_ability);
    }
    m_ability = applyStatus(NULL, obj);
}

/**
 * Set this pokemon's ability by name.
 */
void Pokemon::setAbility(const std::string &name) {
    ScriptContext *cx = m_field->getContext();
    StatusObject ability = cx->getAbility(name);
    if (!ability.isNull()) {
        setAbility(&ability);
    } else {
        cout << "Warning: No such ability as " << name << "." << endl;
    }
}

/**
 * Deduct PP from a move.
 */
void Pokemon::deductPp(MoveObject *move) {
    void *p = move->getObject();
    int j = 0;
    vector<MoveObject *>::const_iterator i = m_moves.begin();
    for (; i != m_moves.end(); ++i) {
        if (p == (*i)->getObject()) {
            deductPp(j);
            return;
        }
        ++j;
    }
}

/**
 * Deduct PP from a move slot.
 */
void Pokemon::deductPp(const int i) {
    const int pp = --m_pp[i];
    m_moveUsed[i] = true;
    m_field->informSetPp(this, i, pp);
}

void Pokemon::initialise(BattleField *field, const int party, const int idx) {
    m_field = field;
    m_party = party;
    m_position = idx;
    const BattleMechanics *mech = field->getMechanics();
    for (int i = 0; i < STAT_COUNT; ++i) {
        m_stat[i] = mech->calculateStat(*this, (STAT)i);
        m_statLevel[i] = 0;
    }
    m_statLevel[S_ACCURACY] = 0;
    m_statLevel[S_EVASION] = 0;

    // Set initial hp.
    m_hp = m_stat[S_HP];

    // Get script machine.
    m_machine = field->getScriptMachine();
    m_cx = field->getContext();

    // Create pokemon object.
    m_object = m_cx->newPokemonObject(this);

    // Create move objects.
    vector<const MoveTemplate *>::const_iterator i = m_moveProto.begin();
    int j = 0;
    for (; i != m_moveProto.end(); ++i) {
        MoveObject *obj = m_cx->newMoveObject(*i);
        m_moves.push_back(obj);
        m_pp[j] = obj->getPp(m_cx) * (5 + m_ppUps[j]) / 5;
        ++j;
    }

    // Create ability object.
    StatusObject ability = m_cx->getAbility(m_abilityName);
    if (!ability.isNull()) {
        m_ability = applyStatus(NULL, &ability);
    } else {
        cout << "No such ability: " << m_abilityName << endl;
    }
}

}
