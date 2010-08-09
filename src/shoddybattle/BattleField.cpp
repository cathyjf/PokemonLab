/* 
 * File:   BattleField.cpp
 * Author: Catherine
 *
 * Created on April 2, 2009, 10:20 PM
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

#include <boost/bind.hpp>
#include <boost/shared_array.hpp>
#include "BattleField.h"
#include "../mechanics/BattleMechanics.h"
#include "../scripting/ScriptMachine.h"
#include "../text/Text.h"
#include <iostream>
#include <list>
#include <cmath>

using namespace std;
using namespace boost;

namespace shoddybattle {

struct BattleFieldImpl {
    FieldObjectPtr object;
    const BattleMechanics *mech;
    GENERATION generation;
    ScriptMachine *machine;
    ScriptContext *context;
    ScriptContextPtr contextRef;
    Pokemon::ARRAY teams[TEAM_COUNT];
    int partySize;
    shared_ptr<PokemonParty> active[TEAM_COUNT];
    STATUSES effects;      // field effects
    bool descendingSpeed;
    std::stack<BattleField::EXECUTION> executing;
    MoveObjectPtr lastMove;
    bool narration;
    int host;

    typedef map<pair<Pokemon *, Pokemon *>, bool> RANDOM_MAP;

    BattleFieldImpl():
            mech(NULL),
            machine(NULL),
            context(NULL),
            narration(true),
            host(0) { }

    void sortInTurnOrder(vector<Pokemon::PTR> &, vector<const PokemonTurn *> &);
    bool speedComparator(RANDOM_MAP &random,
            Pokemon *p1, Pokemon *p2) {
        const int s1 = p1->getStat(S_SPEED);
        const int s2 = p2->getStat(S_SPEED);
        if (s1 != s2) {
            if (descendingSpeed)
                return (s1 > s2);
            return (s2 > s1);
        }
        pair<Pokemon *, Pokemon *> elem(p1, p2);
        RANDOM_MAP::iterator i = random.find(elem);
        if (i != random.end()) {
            return i->second;
        }
        const bool b = mech->getCoinFlip();
        random.insert(RANDOM_MAP::value_type(elem, b));
        return b;
    }

    inline void decodeIndex(int &idx, int &party) {
        if (idx >= partySize) {
            idx -= partySize;
            party = 1;
        } else {
            party = 0;
        }
    }

    void applyEffects(Pokemon *pokemon) {
        for (STATUSES::const_iterator i = effects.begin();
                i != effects.end(); ++i) {
            if (!(*i)->isActive(context))
                continue;

            pokemon->applyStatus(NULL, i->get());
        }
    }

    void initialise(BattleField *field,
        const BattleMechanics *mech,
        GENERATION generation,
        ScriptMachine *machine,
        Pokemon::ARRAY teams[TEAM_COUNT],
        const std::string trainer[TEAM_COUNT],
        const int activeParty,
        vector<StatusObject> &clauses);
    
    void terminate() {
        if (object) {
            object.reset();
        }
        contextRef.reset();
        context = NULL;
        machine = NULL;
    }

    ~BattleFieldImpl() {
        terminate();
    }
};

PokemonTurn PokemonTurn::NOP(TT_NOP, 0);

void BattleField::setNarrationEnabled(const bool enabled) {
    m_impl->narration = enabled;
}

bool BattleField::isNarrationEnabled() const {
    return m_impl->narration;
}

void BattleField::terminate() {
    m_impl.reset();
}

int BattleField::getPartySize() const {
    return m_impl->partySize;
}

MoveObjectPtr BattleField::getLastMove() const {
    return m_impl->lastMove;
}

const BattleField::EXECUTION *BattleField::topExecution() const {
    if (m_impl->executing.empty()) {
        return NULL;
    }
    return &m_impl->executing.top();
}

void BattleField::pushExecution(const BattleField::EXECUTION &exec) {
    m_impl->executing.push(exec);
}

void BattleField::popExecution() {
    m_impl->executing.pop();
}

GENERATION BattleField::getGeneration() const {
    return m_impl->generation;
}

ScriptObject *BattleField::getObject() {
    return m_impl->object.get();
}

/**
 * Send a message to a particular party.
 */
ScriptValue PokemonParty::sendMessage(const string &message,
        int argc, ScriptValue *argv) {
    ScriptValue ret;
    ret.setFailure();
    for (int i = 0; i < m_size; ++i) {
        Pokemon::PTR p = m_party[i].pokemon;
        if (p && !p->isFainted()) {
            ScriptValue v = p->sendMessage(message, argc, argv);
            if (!v.failed()) {
                ret = v;
            }
        }
    }
    return ret;
}

/**
 * Send a message to the whole field.
 */
ScriptValue BattleField::sendMessage(const string &message,
        int argc, ScriptValue *argv) {
    ScriptValue ret;
    ret.setFailure();
    for (int i = 0; i < TEAM_COUNT; ++i) {
        ScriptValue v = m_impl->active[i]->sendMessage(message, argc, argv);
        if (!v.failed()) {
            ret = v;
        }
    }
    return ret;
}

/**
 * Sort a set of pokemon by speed.
 */
template <class T>
void BattleField::sortBySpeed(T &pokemon) {
    sort(pokemon.begin(), pokemon.end(),
            boost::bind(&BattleFieldImpl::speedComparator,
                m_impl, BattleFieldImpl::RANDOM_MAP(), _1, _2));
}

/**
 * Determine whether a particular turn is legal.
 */
bool BattleField::isTurnLegal(Pokemon *pokemon,
        const PokemonTurn *turn,
        const bool replacement) const {
    if (turn->type == TT_SWITCH) {
        if (!replacement && !pokemon->isSwitchLegal())
            return false;
        if (turn->id < 0)
            return false;
        Pokemon::ARRAY &arr = m_impl->teams[pokemon->getParty()];
        const int size = arr.size();
        if (turn->id >= size)
            return false;
        Pokemon::PTR p = arr[turn->id];
        return ((p->getSlot() == -1) && !p->isFainted());
    } else if (turn->type == TT_MOVE) {
        if (replacement) {
            // Client must send a TT_SWITCH action if it's a replacement.
            return false;
        }
    } else {
        // Illegal value of turn->type.
        return false;
    }

    if (pokemon->getForcedTurn()) {
        // If the pokemon has a forced move then it doesn't matter what
        // action the client sent in.
        return true;
    }

    if (turn->id < 0)
        return false;
    if (turn->id >= pokemon->getMoveCount())
        return false;
    if (!pokemon->isMoveLegal(turn->id))
        return false;

    MoveObjectPtr move = pokemon->getMove(turn->id);
    TARGET tc = move->getTemplate(m_impl->context)->getTargetClass();
    if (!isTargeted(tc))
        return true;

    int idx = turn->target;
    if (idx < 0)
        return false;
    int party = 0;
    m_impl->decodeIndex(idx, party);

    if (idx >= m_impl->partySize)
        return false;

    Pokemon::PTR target = (*m_impl->active[party])[idx].pokemon;
    if (!target || target->isFainted())
        return false;

    if (tc == T_NONUSER) {
        // Target cannot be the user.
        if (target.get() == pokemon)
            return false;
    } else if (tc == T_ENEMY) {
        // Target cannot be a member of the user's party.
        if (party == pokemon->getParty())
            return false;
    } else if (tc == T_ALLY) {
        // Target cannot be an enemy, and cannot be the user unless the
        // active party size is one.
        if (party != pokemon->getParty())
            return false;
        if ((m_impl->partySize > 1) && (target.get() == pokemon))
            return false;
    } else if (tc == T_USER_OR_ALLY) {
        // Target cannot be an enemy.
        if (party != pokemon->getParty())
            return false;
    }

    return true;
}

/**
 * Get a random inactive pokemon from the same party as the provided pokemon.
 */
Pokemon *BattleField::getRandomInactivePokemon(Pokemon *pokemon) {
    vector<bool> switches;
    getLegalSwitches(pokemon, switches);
    if (find(switches.begin(), switches.end(), true) == switches.end()) {
        // No inactive pokemon available.
        return NULL;
    }
    const BattleMechanics *mech = m_impl->mech;
    const int party = pokemon->getParty();
    const int length = switches.size();
    while (true) {
        const int idx = mech->getRandomInt(0, length - 1);
        if (switches[idx]) {
            return m_impl->teams[party][idx].get();
        }
    }
}

/**
 * Get a list of legal switches.
 */
void BattleField::getLegalSwitches(Pokemon *pokemon, vector<bool> &switches) {
    const int party = pokemon->getParty();
    Pokemon::ARRAY &arr = m_impl->teams[party];
    const int size = arr.size();
    for (int i = 0; i < size; ++i) {
        Pokemon::PTR p = arr[i];
        const bool value = (p && (p->getSlot() == -1) && !p->isFainted());
        switches.push_back(value);
    }
}

/**
 * Get a random target from a particular party.
 */
Pokemon *BattleField::getRandomTarget(const int partyIdx) const {
    // Build a list of all possible targets.
    vector<Pokemon::PTR> intermediate;
    PokemonParty &party = *m_impl->active[partyIdx].get();
    for (int i = 0; i < party.getSize(); ++i) {
        Pokemon::PTR p = party[i].pokemon;
        if (p && !p->isFainted()) {
            intermediate.push_back(p);
        }
    }

    if (intermediate.empty()) {
        return NULL;
    }

    // Choose a random target from the list.
    const int r = m_impl->mech->getRandomInt(
            0, intermediate.size() - 1);
    return intermediate[r].get();
}

/**
 * Build a list of targets for a move.
 */
void BattleField::getTargetList(TARGET mc, std::vector<Pokemon *> &targets,
        Pokemon *user, Pokemon *target) {
    shared_ptr<PokemonParty> *active = m_impl->active;
    if (isTargeted(mc)) {
        if (!target) {
            targets.push_back(getRandomTarget(1 - user->getParty()));
        } else if (!target->isFainted()) {
            targets.push_back(target);
        }
    } else if ((mc == T_ENEMIES) || (mc == T_ENEMY_FIELD)) {
        PokemonParty &party = *active[1 - user->getParty()].get();
        for (int i = 0; i < party.getSize(); ++i) {
            Pokemon::PTR p = party[i].pokemon;
            if (p && !p->isFainted()) {
                targets.push_back(p.get());
            }
        }
        sortBySpeed(targets);
    } else if (mc == T_RANDOM_ENEMY) {
        Pokemon *rand = getRandomTarget(1 - user->getParty());
        if (rand) {
            targets.push_back(rand);
        }
    } else if (mc == T_NONE) {
        targets.push_back(user->getMemoryPokemon());
    } else if (mc == T_OTHERS) {
        for (int i = 0; i < TEAM_COUNT; ++i) {
            PokemonParty &party = *active[i].get();
            for (int j = 0; j < party.getSize(); ++j) {
                Pokemon::PTR p = party[j].pokemon;
                if (p && !p->isFainted() && (p.get() != user)) {
                    targets.push_back(p.get());
                }
            }
        }
        sortBySpeed(targets);
    } else if (mc == T_ALL) {
        for (int i = 0; i < TEAM_COUNT; ++i) {
            PokemonParty &party = *active[i].get();
            for (int j = 0; j < party.getSize(); ++j) {
                Pokemon::PTR p = party[j].pokemon;
                if (p && !p->isFainted()) {
                    targets.push_back(p.get());
                }
            }
        }
        sortBySpeed(targets);
    } else if (mc == T_ALLIES) {
        Pokemon::ARRAY &team = m_impl->teams[user->getParty()];
        const int size = team.size();
        for (int i = 0; i < size; ++i) {
            Pokemon::PTR p = team[i];
            if (!p->isFainted()) {
                targets.push_back(p.get());
            }
        }
        sortBySpeed(targets);
    }
}

shared_ptr<PokemonParty> *BattleField::getActivePokemon() {
    return m_impl->active;
}

/**
 * Place the active pokemon into a single vector, independent of party.
 */
void BattleField::getActivePokemon(Pokemon::ARRAY &v,
        Pokemon::ARRAY *inactive) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        PokemonParty &party = *m_impl->active[i];
        for (int j = 0; j < m_impl->partySize; ++j) {
            Pokemon::PTR p = party[j].pokemon;
            if (p) {
                if (!p->isFainted()) {
                    v.push_back(p);
                } else if (inactive) {
                    inactive->push_back(p);
                }
            }
        }
    }
}

namespace {

/**
 * Used for sorting pokemon into turn order.
 */
struct TurnOrderEntity {
    Pokemon::PTR pokemon;
    const PokemonTurn *turn;
    bool move;
    int party, position;
    int priority;
    int speed;
    int inherentPriority;
};

bool turnOrderComparator(BattleFieldImpl *impl,
        BattleFieldImpl::RANDOM_MAP &random,
        const TurnOrderEntity &p1, const TurnOrderEntity &p2) {
    // first: is one pokemon switching?
    if (!p1.move && p2.move) {
        return true;    // p1 goes first
    } else if (!p2.move && p1.move) {
        return false;   // p2 goes first
    } else if (!p1.move && !p2.move) {
        if (p1.party == p2.party) {
            return (p1.position < p2.position);
        }
        // host goes first
        return (p1.party == impl->host);
    }

    // second: move priority
    if (p1.priority != p2.priority) {
        return (p1.priority > p2.priority);
    }

    // third: inherent priority (certain items, abilities, etc.)
    if (p1.inherentPriority != p2.inherentPriority) {
        return (p1.inherentPriority > p2.inherentPriority);
    }

    // fourth: speed
    if (p1.speed > p2.speed) {
        return impl->descendingSpeed;
    } else if (p1.speed < p2.speed) {
        return !impl->descendingSpeed;
    }

    // finally: coin flip
    pair<Pokemon *, Pokemon *> elem(p1.pokemon.get(), p2.pokemon.get());
    BattleFieldImpl::RANDOM_MAP::iterator i = random.find(elem);
    if (i != random.end()) {
        return i->second;
    }
    const bool b = impl->mech->getCoinFlip();
    random.insert(BattleFieldImpl::RANDOM_MAP::value_type(elem, b));
    return b;
}

} // anonymous namespace

/**
 * Sort a list of pokemon in turn order.
 */
void BattleFieldImpl::sortInTurnOrder(vector<Pokemon::PTR> &pokemon,
        vector<const PokemonTurn *> &turns) {
    vector<TurnOrderEntity> entities;

    const int count = pokemon.size();
    assert(count == (int)turns.size());
    for (int i = 0; i < count; ++i) {
        TurnOrderEntity entity;
        Pokemon::PTR p = pokemon[i];
        const PokemonTurn *turn = turns[i];
        entity.pokemon = p;
        entity.turn = turn;
        entity.move = (turn->type == TT_MOVE);
        if (entity.move) {
            MoveObjectPtr move = p->getMove(turn->id);
            assert(move);
            entity.priority = move->getPriority(context);
        }
        entity.party = p->getParty();
        entity.position = p->getPosition();
        entity.speed = p->getStat(S_SPEED);
        entity.inherentPriority = p->getInherentPriority();
        entities.push_back(entity);
    }

    // sort the entities
    sort(entities.begin(), entities.end(),
            boost::bind(turnOrderComparator, this,
                BattleFieldImpl::RANDOM_MAP(), _1, _2));

    // reorder the parameter vectors
    for (int i = 0; i < count; ++i) {
        TurnOrderEntity &entity = entities[i];
        pokemon[i] = entity.pokemon;
        turns[i] = entity.turn;
    }
}

BattleField::BattleField():
        m_impl(shared_ptr<BattleFieldImpl>(new BattleFieldImpl())) { }

ScriptMachine *BattleField::getScriptMachine() {
    return m_impl->machine;
}

const BattleMechanics *BattleField::getMechanics() const {
    return m_impl->mech;
}

/**
 * Switch to a new pokemon.
 */
void BattleField::executeSwitchAction(Pokemon *p, const int idx) {
    const int party = p->getParty();
    const int slot = p->getSlot();
    withdrawPokemon(p);
    sendOutPokemon(party, slot, idx);
}

/**
 * Withdraw an active pokemon.
 */
void BattleField::withdrawPokemon(Pokemon *p) {
    if (p->isFainted()) {
        //TODO: call switchOut() instead?
        p->setSlot(-1);
        return;
    }
    informWithdraw(p);
    ScriptValue argv[] = { p };
    for (int i = 0; i < TEAM_COUNT; ++i) {
        m_impl->active[i]->sendMessage("informWithdraw", 1, argv);
    }
    p->switchOut(); // Note: clears slot.
}

/**
 * Send out a new pokemon.
 */
void BattleField::sendOutPokemon(const int party,
        const int slot, const int idx) {
    Pokemon::PTR replacement = m_impl->teams[party][idx];
    (*m_impl->active[party])[slot].pokemon = replacement;
    replacement->setSlot(slot);
    informSendOut(replacement.get());
    m_impl->applyEffects(replacement.get());
    replacement->switchIn();
}

/**
 * Apply a status effect to the whole field.
 */
StatusObjectPtr BattleField::applyStatus(StatusObject *effect) {
    StatusObjectPtr ret = effect->cloneAndRoot(m_impl->context);
    ret->disableClone(m_impl->context);
    bool applied = false;
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted() && p->applyStatus(NULL, ret.get())) {
                applied = true;
            }
        }
    }
    if (!applied) {
        return StatusObjectPtr();
    }
    // TODO: Call a function to inform that the effect was applied.
    m_impl->effects.push_back(ret);
    return ret;
}

/**
 * Remove a status effect from the field.
 */
void BattleField::removeStatus(StatusObject *effect) {
    effect->unapplyEffect(m_impl->context);
    effect->dispose(m_impl->context);
}

/**
 * Print a message to the BattleField.
 */
void BattleField::print(const TextMessage &msg) {
    if (!isNarrationEnabled())
        return;

    const vector<string> &args = msg.getArgs();
    const int argc = args.size();
    const char *argv[argc];
    for (int i = 0; i < argc; ++i) {
        argv[i] = args[i].c_str();
    }
    string message = m_impl->machine->getText(
            msg.getCategory(), msg.getMessage(), argc, argv);
    cout << message << endl;
}

void BattleField::informSendOut(Pokemon *p) {
    const int party = p->getParty();
    const string trainer = m_impl->active[party]->getName();
    cout << trainer << " sent out " << p->getName()
            << " (lvl " << p->getLevel() << " " << p->getSpeciesName() << ")"
            << "!" << endl;
}

void BattleField::informWithdraw(Pokemon *p) {
    const int party = p->getParty();
    const string trainer = m_impl->active[party]->getName();
    cout << trainer << " withdrew " << p->getName() << "!" << endl;
}

void BattleField::informUseMove(Pokemon *p, MoveObject *move) {
    cout << p->getName() << " used "
            << move->getName(m_impl->context) << "!" << endl;
}

void BattleField::informHealthChange(Pokemon *p, const int delta) {
    const int numerator = floor(48.0 * (double)delta
            / (double)p->getRawStat(S_HP) + 0.5);
    cout << p->getName() << " lost " << numerator << "/48 of its health!"
            << endl;
}

void BattleField::informFainted(Pokemon *p) {
    cout << p->getName() << " fainted!" << endl;
}

void BattleField::informStatusChange(Pokemon *, StatusObject *, const bool) {

}

/**
 * Determine whether to veto the selection of a move.
 */
bool BattleField::vetoSelection(Pokemon *user, MoveObject *move) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted() && p->vetoSelection(user, move)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Determine whether to veto a switch by the subject.
 */
bool BattleField::vetoSwitch(Pokemon *subject) {
    ScriptValue args[] = { subject };
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted()) {
                ScriptValue v = p->sendMessage("vetoSwitch", 1, args);
                if (!v.failed() && v.getBool()) {
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * Determine whether to veto the execution of a move. A single effect wanting
 * to veto the execution is enough to do so.
 */
/**bool BattleField::vetoExecution(Pokemon *user, Pokemon *target,
        MoveObject *move) {
    if (user->vetoExecution(user, target, move))
        return true;
    
    vector<Pokemon *> pokemon;
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted()) {
                Pokemon *pp = p.get();
                if (pp != user) {
                    pokemon.push_back(pp);
                }
            }
        }
    }

    sortBySpeed(pokemon);
    for (vector<Pokemon *>::iterator i = pokemon.begin();
            i != pokemon.end(); ++i) {
        if ((*i)->vetoExecution(user, target, move)) {
            return true;
        }
    }
    return false;
}**/

namespace {

/**
 * The user's own statuses are checked first, then index order.
 */
bool vetoExecutionPredicate(Pokemon::PTR p1, Pokemon::PTR p2, Pokemon *user) {
    if (p1.get() == user)
        return true;
    if (p2.get() == user)
        return false;
    int diff = p1->getParty() - p2->getParty();
    if (diff != 0) {
        return (diff < 0);
    }
    return ((p1->getSlot() - p2->getSlot()) < 0);
}

} // anonymous namespace

/**
 * Determine whether to veto the execution of a move. A single effect wanting
 * to veto the execution is enough to do so.
 */
bool BattleField::vetoExecution(Pokemon *user, Pokemon *target,
        MoveObject *move) {
    Pokemon::ARRAY pokemon;
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted()) {
                pokemon.push_back(p);
            }
        }
    }
    sort(pokemon.begin(), pokemon.end(),
            boost::bind(vetoExecutionPredicate, _1, _2, user));
    for (Pokemon::ARRAY::iterator i = pokemon.begin();
            i != pokemon.end(); ++i) {
        if ((*i)->vetoExecution(user, target, move)) {
            return true;
        }
    }
    return false;
}

/**
 * Begin the battle for a particular party.
 */
void BattleField::beginBattle(const int party) {
    for (int i = 0; i < m_impl->partySize; ++i) {
        PokemonSlot &slot = (*m_impl->active[party])[i];
        Pokemon::PTR p = slot.pokemon;
        if (p) {
            p->setSlot(i);
            informSendOut(p.get());
        }
    }
}

/**
 * Begin the battle.
 */
void BattleField::beginBattle() {
    // Host goes second (if I remember correctly). TODO: Check this.
    beginBattle(1 - m_impl->host);
    beginBattle(m_impl->host);
    Pokemon::ARRAY pokemon;
    getActivePokemon(pokemon);
    vector<Pokemon *> entries;
    for (Pokemon::ARRAY::iterator i = pokemon.begin();
            i != pokemon.end(); ++i) {
        entries.push_back(i->get());
    }
    sortBySpeed(entries);
    vector<Pokemon *>::iterator i = entries.begin();
    for (; i != entries.end(); ++i) {
        m_impl->applyEffects(*i);
        (*i)->switchIn();
    }
}

namespace {

struct EffectEntity {
    Pokemon::PTR subject;
    StatusObject *effect;
    int speed;
    double tier;
    int subtier;
};

bool effectComparator(const bool descendingSpeed,
        const EffectEntity &e1, const EffectEntity &e2) {
    if (e1.tier != e2.tier) {
        return (e1.tier < e2.tier);
    }
    if (e1.speed != e2.speed) {
        const bool cmp = e1.speed > e2.speed;
        return descendingSpeed ? cmp : !cmp;
    }
    return e1.subtier < e1.subtier;
}

} // anonymous namespace

/**
 * Process end of turn effects.
 */
void BattleField::tickEffects() {
    ScriptContext *cx = m_impl->context;

    for (STATUSES::const_iterator i = m_impl->effects.begin();
            i != m_impl->effects.end(); ++i) {
        if (!(*i)->isActive(cx))
            continue;

        cx->callFunctionByName(i->get(), "beginTick", 0, NULL);
    }

    vector<EffectEntity> effects;
    for (int i = 0; i < TEAM_COUNT; ++i) {
        PokemonParty &party = *m_impl->active[i];
        for (int j = 0; j < m_impl->partySize; ++j) {
            Pokemon::PTR p = party[j].pokemon;
            if (p && !p->isFainted()) {
                const STATUSES &statuses = p->getEffects();
                STATUSES::const_iterator k = statuses.begin();
                for (; k != statuses.end(); ++k) {
                    if ((*k)->isActive(cx)) {
                        const int speed = p->getStat(S_SPEED);
                        const double tier = (*k)->getTier(cx);
                        const int subtier = (*k)->getSubtier(cx);
                        EffectEntity entity = { p, k->get(), speed, tier,
                                subtier };
                        effects.push_back(entity);
                    }
                }
            }
        }
    }

    sort(effects.begin(), effects.end(), boost::bind(effectComparator,
            m_impl->descendingSpeed, _1, _2));

    int tier = -1;
    for (vector<EffectEntity>::iterator i = effects.begin();
            i != effects.end(); ++i) {

        if (tier != i->tier) {
            if ((tier != -1) && determineVictory()) {
                return;
            }
            tier = i->tier;
        }

        Pokemon::PTR subject = i->subject;
        StatusObject *effect = i->effect;
        effect->setSubject(cx, subject.get());
        effect->tick(cx);

        if (i->tier == 6) {
            if (determineVictory()) {
                return;
            }
        }
    }

    for (STATUSES::const_iterator i = m_impl->effects.begin();
            i != m_impl->effects.end(); ++i) {
        if (!(*i)->isActive(cx))
            continue;

        cx->callFunctionByName(i->get(), "endTick", 0, NULL);
    }

    for (int i = 0; i < TEAM_COUNT; ++i) {
        PokemonParty &party = *m_impl->active[i];
        for (int j = 0; j < m_impl->partySize; ++j) {
            Pokemon::PTR p = party[j].pokemon;
            if (p && !p->isFainted()) {
                p->removeStatuses();
            }
        }
    }

    Pokemon::removeStatuses(m_impl->effects,
            boost::bind(&StatusObject::isRemovable, _1, m_impl->context));

    determineVictory();
}

/**
 * Count the number of living pokemon on a team.
 */
int BattleField::getAliveCount(const int party, const bool reserve) const {
    Pokemon::ARRAY &arr = m_impl->teams[party];
    const int size = arr.size();
    int alive = 0;
    for (int i = 0; i < size; ++i) {
        if (arr[i] && !arr[i]->isFainted()) {
            if (!reserve || (arr[i]->getSlot() == -1)) {
                ++alive;
            }
        }
    }
    return alive;
}

/**
 * Determine whether the match has ended.
 */
bool BattleField::determineVictory() {
    int victory = -1;
    for (STATUSES::const_iterator i = m_impl->effects.begin();
            i != m_impl->effects.end(); ++i) {
        if (!(*i)->isActive(m_impl->context))
            continue;

        if (m_impl->context->hasProperty(i->get(), "determineVictory")) {
            ScriptValue v = m_impl->context->callFunctionByName(i->get(),
                   "determineVictory", 0, NULL);
            if (!v.failed()) {
                const int val = v.getInt();
                if (val != -1) {
                    if ((victory != -1) && (victory != val)) {
                        informVictory(-1);
                        return true;
                    }
                    victory = val;
                }
            }
        }
    }
    if (victory != -1) {
        informVictory(victory);
        return true;
    }

    vector<bool> fainted(TEAM_COUNT, true);
    int total = TEAM_COUNT;
    for (int i = 0; i < TEAM_COUNT; ++i) {
        Pokemon::ARRAY &arr = m_impl->teams[i];
        const int size = arr.size();
        for (int j = 0; j < size; ++j) {
            if (arr[j] && !arr[j]->isFainted()) {
                fainted[i] = false;
                --total;
                break;
            }
        }
    }

    if (total == 0) {
        return false;
    }

    if (total == TEAM_COUNT) {
        informVictory(-1); // draw
    } else {
        informVictory(fainted[0] ? 1 : 0);
    }
    return true;
}

/**
 * Inform that one team won the battle.
 */
void BattleField::informVictory(const int party) {
    if (party != -1) {
        PokemonParty &obj = *m_impl->active[party];
        cout << obj.getName() << " wins!" << endl;
    } else {
        cout << "It's a draw!" << endl;
    }
}

/**
 * Get the fainted pokemon.
 */
void BattleField::getFaintedPokemon(Pokemon::ARRAY &pokemon) {
    int alive[TEAM_COUNT];
    for (int i = 0; i < TEAM_COUNT; ++i) {
        alive[i] = getAliveCount(i, true);
    }
    for (int i = 0; i < TEAM_COUNT; ++i) {
        PokemonParty &party = *m_impl->active[i];
        for (int j = 0; j < m_impl->partySize; ++j) {
            Pokemon::PTR p = party[j].pokemon;
            if (p && p->isFainted() && (alive[i] > 0)) {
                pokemon.push_back(p);
                --alive[i];
            }
        }
    }
}

/**
 * Process a set of replacements.
 */
void BattleField::processReplacements(const std::vector<PokemonTurn> &turns) {
    vector<Pokemon::PTR> pokemon;
    getFaintedPokemon(pokemon);

    const int count = pokemon.size();
    if (count != (int)turns.size())
        throw BattleFieldException();

    vector<const PokemonTurn *> ordered;
    for (int i = 0; i < count; ++i) {
        const PokemonTurn *turn = &turns[i];
        if (turn->type != TT_SWITCH) {
            throw BattleFieldException();
        }
        ordered.push_back(turn);
    }

    m_impl->sortInTurnOrder(pokemon, ordered);

    for (int i = 0; i < count; ++i) {
        Pokemon::PTR p = pokemon[i];
        const PokemonTurn *turn = ordered[i];

        executeSwitchAction(p.get(), turn->id);
    }
}

/**
 * Execute a move action.
 */
void BattleField::executePendingMoveAction(Pokemon *p) {
    PokemonTurn *turn = p->getTurn();
    // Note that the following line can change the fields of *turn.
    p->sendMessage("informBeginExecution", 0, NULL);

    const bool choice = (p->getForcedTurn() == NULL);
    const int id = turn->id;
    MoveObjectPtr move = p->getMove(id);
    Pokemon *target = NULL;
    TARGET tc = move->getTargetClass(m_impl->context);
    if (isTargeted(tc)) {
        int targetIdx = turn->target;
        if (targetIdx == -1) {
            target = getRandomTarget(1 - p->getParty());
        } else {
            int party = 0;
            m_impl->decodeIndex(targetIdx, party);
            Pokemon::PTR t = (*m_impl->active[party])[targetIdx].pokemon;
            if (!t || t->isFainted()) {
                // If this is a single target move and there is no
                // target then choose a random target from among
                // the available enemies.

                // TODO: Handle T_ALLY and T_USER_OR_ALLY here.
                target = getRandomTarget(1 - p->getParty());
            } else {
                target = t.get();
            }
        }
    }

    p->clearForcedTurn();
    if (p->executeMove(move, target)) {
        if (choice) {
            // Only deduct pp if the move was chosen freely.
            // TODO: Actually deduct this after the field-wide veto
            //       check, but before the move is used (matters for
            //       Mimic).
            p->deductPp(id);
        }
        // Set last move used.
        p->setLastMove(move);
        m_impl->lastMove = move;
    }
    ScriptValue val[] = { p, move.get() };
    sendMessage("informFinishedExecution", 2, val);
}

/**
 * Execute an action.
 */
bool BattleField::executePendingAction(Pokemon *p) {
    PokemonTurn *turn = p->getTurn();
    const bool execute = turn && !p->isFainted() && p->isActive();
    if (execute) {
        if (turn->type == TT_MOVE) {
            executePendingMoveAction(p);
        } else {
            executeSwitchAction(p, turn->id);
        }
    }
    p->setTurn(NULL);
    return execute;
}

/**
 * Process a turn.
 */
void BattleField::processTurn(vector<PokemonTurn> &turns) {
    vector<Pokemon::PTR> pokemon, inactive;
    getActivePokemon(pokemon, &inactive);
    const int count = pokemon.size();
    if (count != (int)turns.size())
        throw BattleFieldException();

    for_each(inactive.begin(), inactive.end(),
            boost::bind(&Pokemon::setTurn, _1, &PokemonTurn::NOP));
    
    vector<const PokemonTurn *> ordered;
    for (int i = 0; i < count; ++i) {
        Pokemon::PTR p = pokemon[i];
        PokemonTurn *turn = &turns[i];

        PokemonTurn *forced = p->getForcedTurn();
        Pokemon::FORCED_TYPE type = p->getForcedType();
        if (forced && ((type == Pokemon::FORCED_ACTION)
                || (turn->type != TT_SWITCH))) {
            turn = forced;
        }
        
        p->setTurn(turn);
        ordered.push_back(turn);
        p->clearDamagedFlag();
    }

    // Determine whether to sort speeds in ascending or descending order for
    // this turn. Note that the choice to send this message to the first
    // pokemon is arbitrary; any pokemon would work, since any effect that
    // effects speed sorting should be present on all pokemon.
    ScriptValue v = pokemon[0]->sendMessage("informSpeedSort", 0, NULL);
    m_impl->descendingSpeed = v.failed() ? true : v.getBool();
    
    m_impl->sortInTurnOrder(pokemon, ordered);

    // Begin the turn.
    for (int i = 0; i < count; ++i) {
        Pokemon::PTR p = pokemon[i];
        p->clearRecentDamage();
        const PokemonTurn *turn = ordered[i];

        if (turn->type == TT_MOVE) {
            MoveObjectPtr move = p->getMove(turn->id);
            Pokemon *target = NULL;
            int idx = turn->target;
            if (idx != -1) { // exactly one target
                int party = 0;
                m_impl->decodeIndex(idx, party);
                target = (*m_impl->active[party])[idx].pokemon.get();
            }
            move->beginTurn(m_impl->context, this, p.get(), target);
        }
    }

    // Execute the actions.
    for (int i = 0; i < count; ++i) {
        if (executePendingAction(pokemon[i].get())) {
            if (determineVictory()) {
                return;
            }
        }
    }

    for_each(inactive.begin(), inactive.end(),
            boost::bind(&Pokemon::setTurn, _1, (PokemonTurn *)NULL));

    // Execute end of turn effects.
    tickEffects();
}

/**
 * Get the turn pending for a particular slot.
 */
PokemonTurn *BattleField::getTurn(const int i, const int j) {
    Pokemon::PTR p = getActivePokemon(i, j);
    if (p) {
        return p->getTurn();
    }
    return &PokemonTurn::NOP;
}

/**
 * Transform a status effect.
 */
void BattleField::transformStatus(Pokemon *subject, StatusObjectPtr *status) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted()) {
                p->transformStatus(subject, status);
                if (!*status) {
                    return;
                }
            }
        }
    }
}

/**
 * Get the modifiers in play for a particular hit. Checks all of the active
 * pokemon for "modifier" properties.
 */
void BattleField::getModifiers(Pokemon &user, Pokemon &target,
        MoveObject &obj, const bool critical, const int targets,
        MODIFIERS &mods) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted()) {
                p->getModifiers(&user, &target, &obj, critical, targets, mods);
            }
        }
    }
}

/**
 * Get additional immunities or vulnerabilities in play for a given user
 * attacking a given target.
 */
void BattleField::getImmunities(Pokemon *user, Pokemon *target,
        set<const PokemonType *> &immunities,
        set<const PokemonType *> &vulnerabilities) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted()) {
                p->getImmunities(user, target, immunities, vulnerabilities);
            }
        }
    }
}

/**
 * Get the transformed effectiveness of a move of a certain
 * type against a particular target
 */
bool BattleField::getTransformedEffectiveness(const PokemonType *moveType,
        const PokemonType *type, Pokemon *target, double &factor) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted()) {
                if (p->getTransformedEffectiveness(moveType, type,
                        target, factor)) {
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * Get the stat modifiers in play for a particular hit. Checks all of the
 * active pokemon for "modifier" properties.
 */
void BattleField::getStatModifiers(STAT stat,
        Pokemon *subject, Pokemon *target, PRIORITY_MAP &mods) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted()) {
                p->getStatModifiers(stat, subject, target, mods);
            }
        }
    }
}

/**
 * Get one of the pokemon teams.
 */
const Pokemon::ARRAY &BattleField::getTeam(const int i) const {
    return m_impl->teams[i];
}

/**
 * Get the host of the battle.
 */
int BattleField::getHost() const {
    return m_impl->host;
}

/**
 * Get the ScriptContext associated with this BattleField. This is to be called
 * only from within the logic of a battle turn.
 */
ScriptContext *BattleField::getContext() {
    return m_impl->context;
}

void BattleField::initialise(const BattleMechanics *mech,
        GENERATION generation,
        ScriptMachine *machine,
        Pokemon::ARRAY teams[TEAM_COUNT],
        const std::string trainer[TEAM_COUNT],
        const int activeParty,
        vector<StatusObject> &clauses) {
    try {
        m_impl->initialise(this, mech, generation,
                machine, teams, trainer, activeParty, clauses);
    } catch (BattleFieldException &e) {
        m_impl->terminate();
        throw e;
    }
}

void BattleFieldImpl::initialise(BattleField *field,
        const BattleMechanics *mech,
        GENERATION generation,
        ScriptMachine *machine,
        Pokemon::ARRAY teams[TEAM_COUNT],
        const std::string trainer[TEAM_COUNT],
        const int activeParty,
        vector<StatusObject> &clauses) {
    if (mech == NULL) {
        throw BattleFieldException();
    }
    if (machine == NULL) {
        throw BattleFieldException();
    }
    this->machine = machine;
    this->contextRef = machine->acquireContext();
    this->context = this->contextRef.get();
    this->object = context->newFieldObject(field);
    this->mech = mech;
    this->host = mech->getCoinFlip() ? 0 : 1;
    this->generation = generation;
    this->partySize = activeParty;
    this->descendingSpeed = true;
    for (int i = 0; i < TEAM_COUNT; ++i) {
        this->teams[i] = teams[i];
        active[i] = shared_ptr<PokemonParty>(
                new PokemonParty(activeParty, trainer[i]));

        // Set first n pokemon to be the active pokemon.
        int bound = activeParty;
        const int size = teams[i].size();
        if (activeParty > size) {
            bound = size;
        }
        for (int j = 0; j < bound; ++j) {
            PokemonSlot &slot = (*active[i])[j];
            slot.pokemon = this->teams[i][j];
        }
    }
    for (int i = 0; i < TEAM_COUNT; ++i) {
        // Do some basic initialisation on each pokemon.
        Pokemon::ARRAY &team = this->teams[i];
        const int length = team.size();
        for (int j = 0; j < length; ++j) {
            Pokemon::PTR p = team[j];
            if (!p) {
                throw BattleFieldException();
            }
            p->initialise(field, contextRef, i, j);
        }
    }
    // Apply clauses to the field
    vector<StatusObject>::iterator i = clauses.begin();
    for (; i != clauses.end(); ++i) {
        field->applyStatus(&*i);
    }
}

}

