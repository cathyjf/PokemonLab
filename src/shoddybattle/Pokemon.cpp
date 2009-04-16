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
        const std::string nickname,
        const PokemonNature *nature,
        const std::string ability,
        const std::string item,
        const int *iv,
        const int *ev,
        const int level,
        const int gender,
        const bool shiny,
        const std::vector<std::string> &moves,
        const std::vector<int> &ppUps) {
    memcpy(m_iv, iv, sizeof(int) * STAT_COUNT);
    memcpy(m_ev, ev, sizeof(int) * STAT_COUNT);
    m_species = species;
    m_nickname = nickname;
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
    m_machine = NULL;
    m_field = NULL;
    m_object = NULL;
    m_fainted = false;
    m_item = NULL;
    m_ability = NULL;
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

unsigned int Pokemon::getBaseStat(const STAT i) const {
    return m_species->getBaseStat(i);
}

/**
 * Determine whether the execution of a move should be vetoed, according to
 * the effects on this pokemon.
 */
bool Pokemon::vetoExecution(ScriptContext *cx,
        Pokemon *user, Pokemon *target, MoveObject *move) {
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(cx))
            continue;

        if ((*i)->vetoExecution(cx, m_field, user, target, move))
            return true;
    }
    return false;
}

/**
 * Get a status effect by ID.
 */
StatusObject *Pokemon::getStatus(ScriptContext *cx, const string id) {
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if ((*i)->isRemovable(cx))
            continue;

        if ((*i)->getId(cx) == id)
            return *i;
    }
    return NULL;
}

/**
 * Execute an arbitrary move on a particular target.
 */
bool Pokemon::useMove(ScriptContext *cx, MoveObject *move,
        Pokemon *target, const int targets) {
    if (m_field->vetoExecution(this, target, move)) {
        // vetoed for this target
        return false;
    }
    if (move->attemptHit(cx, m_field, this, target)) {
        move->use(cx, m_field, this, target, targets);
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
 * Execute an arbitrary move on a set of targets.
 */
bool Pokemon::executeMove(ScriptContext *cx, MoveObject *move,
        Pokemon *target, bool inform) {

    // TODO: research if this should be inside an inform check
    if (m_field->vetoExecution(this, NULL, move)) {
        // vetoed
        return false;
    }
    
    cout << getName() << " used " << move->getName(cx) << "!" << endl;

    if (move->getFlag(cx, F_UNIMPLEMENTED)) {
        cout << "But it's unimplemented..." << endl;
        return false;
    }

    TARGET tc = move->getTargetClass(cx);

    if ((tc == T_USER) || (tc == T_ALLY) || (tc == T_ALLIES)) {
        move->use(cx, m_field, this, NULL, 0);
        return true;
    }
    
    // Build a list of targets.
    vector<Pokemon *> targets;
    m_field->getTargetList(tc, targets, this, target);

    if (tc == T_LAST_ENEMY) {
        move->use(cx, m_field, this, targets[0], 0);
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
            (*i)->informTargeted(cx, this, move);
        }
    }

    if (isEnemyTarget(tc)) {
        for (vector<Pokemon *>::iterator i = targets.begin();
                i != targets.end(); ++i) {
            useMove(cx, move, *i, targetCount);
            if ((*i)->isFainted()) {
                --targetCount;
            }
        }
    } else {
        // no target as such
        move->use(cx, m_field, this, NULL, 0);
    }

    return true;
}

/**
 * Remove one of this pokemon's memories, perhaps due to a pokemon leaving
 * the field.
 */
void Pokemon::removeMemory(Pokemon *pokemon) {
    RECENT_MOVE entry = { pokemon, NULL };
    m_memory.remove(entry);
}

/**
 * Set one of this pokemon's moves to a different move.
 */
void Pokemon::setMove(const int i, MoveObject *move) {
    if (m_moves.size() <= i) {
        m_moves.resize(i + 1, NULL);
    }
    MoveObject *p = m_moves[i];
    if (p) {
        ScriptContext *cx = m_field->getContext();
        cx->removeRoot(p);
    }
    m_moves[i] = move;
}

/**
 * Set one of this pokemon's moves to a different move, by name. This
 * function will probably only be used for testing, as actual logic is more
 * likely to be use the MoveObject * version above.
 */
void Pokemon::setMove(const int i, const string &name) {
    ScriptContext *cx = m_field->getContext();
    MoveDatabase *moves = m_machine->getMoveDatabase();
    const MoveTemplate *tpl = moves->getMove(name);
    assert(tpl);
    MoveObject *move = cx->newMoveObject(tpl);
    setMove(i, move);
}

/**
 * Remove defunct statuses from this pokemon.
 */
void Pokemon::removeStatuses(ScriptContext *cx) {
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if (!(*i)->isRemovable(cx))
            continue;

        // this is a list so it is safe to remove an arbitrary element
        cx->removeRoot(*i);
        m_effects.erase(i);
    }
}

/**
 * Return whether the pokemon has the specified ability.
 */
bool Pokemon::hasAbility(const string &name) {
    if (m_ability == NULL)
        return false;
    ScriptContext *cx = m_field->getContext();
    return (m_ability->getId(cx) == name);
}

/**
 * Apply a StatusEffect to this pokemon. Makes a copy of the parameter before
 * applying it to the pokemon.
 */
StatusObject *Pokemon::applyStatus(ScriptContext *cx,
        Pokemon *inducer, StatusObject *effect) {
    if (!effect)
        return NULL;

    if (effect->isSingleton(cx)) {
        StatusObject *singleton = getStatus(cx, effect->getId(cx));
        if (singleton)
            return singleton;
    }
    
    // TODO: implement properly

    StatusObject *applied = effect->cloneAndRoot(cx);
    if (inducer != NULL) {
        applied->setInducer(cx, inducer);
    }
    applied->setSubject(cx, this);
    if (!applied->applyEffect(cx)) {
        cx->removeRoot(applied);
        return NULL;
    }

    m_effects.push_back(applied);
    return applied;
}

/**
 * Remove a StatusEffect from this pokemon.
 */
void Pokemon::removeStatus(ScriptContext *scx, StatusObject *status) {
    status->unapplyEffect(scx);
    status->dispose(scx);
}

/**
 * Get critical hit chance (additive) modifiers.
 */
int Pokemon::getCriticalModifier(ScriptContext *cx) const {
    int ret = 0;
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(cx))
            continue;

        ret += (*i)->getCriticalModifier(cx);
    }
    return ret;
}

/**
 * Transform a health change.
 */
int Pokemon::transformHealthChange(ScriptContext *cx, int hp,
        bool indirect) const {
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(cx))
            continue;

        (*i)->transformHealthChange(cx, hp, indirect, &hp);
    }
    return hp;
}

/**
 * Check if this Pokemon has "inherent priority". This is used for certain
 * items and abilities.
 */
int Pokemon::getInherentPriority(ScriptContext *cx) const {
    int ret = 0;
    for (STATUSES::const_iterator i = m_effects.begin();
            i != m_effects.end(); ++i) {
        if (!(*i)->isActive(cx))
            continue;

        int v = (*i)->getInherentPriority(cx);
        if (abs(v) > abs(ret)) {
            ret = v;
        }
    }
    return ret;
}

/**
 * Check for stat modifiers on all status effects.
 */
void Pokemon::getStatModifiers(ScriptContext *cx,
        STAT stat, Pokemon *subject, PRIORITY_MAP &mods) {
    MODIFIER mod;
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if (!(*i)->isActive(cx))
            continue;

        if ((*i)->getStatModifier(cx, m_field, stat, subject, mod)) {
            // position unused
            mods[mod.priority] = mod.value;
        }
    }
}

/**
 * Check for modifiers on all status effects.
 */
void Pokemon::getModifiers(ScriptContext *cx,
        Pokemon *user, Pokemon *target,
        MoveObject *obj, const bool critical, MODIFIERS &mods) {
    MODIFIER mod;
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if (!(*i)->isActive(cx))
            continue;

        if ((*i)->getModifier(cx, m_field, user, target, obj, critical, mod)) {
            mods[mod.position][mod.priority] = mod.value;
        }
    }
}

/**
 * Set the current hp of the pokemon, and also inform the BattleField, which
 * can cause side effects such as the printing of messages.
 */
void Pokemon::setHp(ScriptContext *cx, const int hp, const bool indirect) {
    // TODO: implement this function properly
    if (m_fainted) {
        return;
    }
    const int delta = transformHealthChange(cx, m_hp - hp, indirect);
    m_hp -= delta;
    m_field->informHealthChange(this, delta);
    if (m_hp <= 0) {
        m_field->informFainted(this);
        m_fainted = true;
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
 * Inform that this pokemon was targeted by a move.
 */
void Pokemon::informTargeted(ScriptContext *cx,
        Pokemon *user, MoveObject *move) {
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if (!(*i)->isActive(cx))
            continue;

        (*i)->informTargeted(cx, user, move);
    }

    if (move->getFlag(cx, F_MEMORABLE)) {
        list<RECENT_MOVE>::iterator i = m_memory.begin();
        for (; i != m_memory.end(); ++i) {
            if (i->user == user) {
                m_memory.erase(i);
                break;
            }
        }
        const RECENT_MOVE entry = { user, move->getTemplate() };
        m_memory.push_back(entry);
    }
}

void Pokemon::initialise(BattleField *field, const int party, const int j) {
    m_field = field;
    m_party = party;
    m_position = j;
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
    ScriptContext *cx = m_machine->acquireContext();

    // Create pokemon object.
    m_object = cx->newPokemonObject(this);

    // Create move objects.
    vector<const MoveTemplate *>::const_iterator i = m_moveProto.begin();
    for (; i != m_moveProto.end(); ++i) {
        MoveObject *obj = cx->newMoveObject(*i);
        m_moves.push_back(obj);
    }

    // Create ability object.
    StatusObject ability = cx->getAbility(m_abilityName);
    if (!ability.isNull()) {
        m_ability = applyStatus(cx, NULL, &ability);
    } else {
        cout << "No such ability: " << m_abilityName << endl;
    }

    m_machine->releaseContext(cx);
}

}
