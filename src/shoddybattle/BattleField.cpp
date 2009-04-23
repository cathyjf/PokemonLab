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

using namespace std;
using namespace boost;

namespace shoddybattle {

struct BattleFieldImpl {
    FieldObject *object;
    const BattleMechanics *mech;
    GENERATION generation;
    ScriptMachine *machine;
    ScriptContext *context;
    Pokemon::ARRAY teams[TEAM_COUNT];
    int partySize;
    shared_ptr<PokemonParty> active[TEAM_COUNT];
    STATUSES m_effects;      // field effects
    bool descendingSpeed;
    std::stack<BattleField::EXECUTION> executing;

    BattleFieldImpl():
            object(NULL),
            mech(NULL),
            machine(NULL),
            context(NULL) { }

    void sortInTurnOrder(vector<Pokemon::PTR> &, vector<const PokemonTurn *> &);
    bool speedComparator(Pokemon *p1, Pokemon *p2) {
        const int s1 = p1->getStat(S_SPEED);
        const int s2 = p2->getStat(S_SPEED);
        if (s1 != s2) {
            if (descendingSpeed)
                return (s1 > s2);
            return (s2 > s1);
        }
        return mech->getCoinFlip();
    }

    inline void decodeIndex(int &idx, int &party) {
        if (idx >= partySize) {
            idx -= partySize;
            party = 1;
        } else {
            party = 0;
        }
    }

    void initialise(BattleField *field,
        const BattleMechanics *mech,
        GENERATION generation,
        ScriptMachine *machine,
        Pokemon::ARRAY teams[TEAM_COUNT],
        const std::string trainer[TEAM_COUNT],
        const int activeParty);

    void cleanUp() {
        if (object) {
            context->removeRoot(object);
            object = NULL;
        }
        if (context) {
            machine->releaseContext(context);
            context = NULL;
        }
        machine = NULL;
    }

    ~BattleFieldImpl() {
        cleanUp();
    }
};

int BattleField::getPartySize() const {
    return m_impl->partySize;
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
    return m_impl->object;
}

/**
 * Sort a set of pokemon by speed.
 */
void BattleField::sortBySpeed(std::vector<Pokemon *> &pokemon) {
    sort(pokemon.begin(), pokemon.end(),
            boost::bind(&BattleFieldImpl::speedComparator, m_impl, _1, _2));
}

/**
 * Determine whether a particular turn is legal.
 */
bool BattleField::isTurnLegal(Pokemon *pokemon, const PokemonTurn *turn) const {
    
}

/**
 * Get a random target from a particular party.
 */
Pokemon *BattleField::getRandomTarget(const int partyIdx) const {
    // build a list of all possible targets
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

    // choose one randomly
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
    if (mc == T_SINGLE) {
        if (target == NULL) {
            target = getRandomTarget(1 - user->getParty());
        }
        targets.push_back(target);
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
    } else if (mc == T_LAST_ENEMY) {
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
    }
}

shared_ptr<PokemonParty> *BattleField::getActivePokemon() {
    return m_impl->active;
}

/**
 * Place the active pokemon into a single vector, indepedent of party.
 */
void BattleField::getActivePokemon(vector<Pokemon::PTR> &v) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        PokemonParty &party = *m_impl->active[i];
        for (int j = 0; j < m_impl->partySize; ++j) {
            Pokemon::PTR p = party[j].pokemon;
            if (p) { // could be an empty slot
                v.push_back(p);
            }
        }
    }
}

/**
 * Used for sorting pokemon into turn order.
 */
struct TurnOrder {
    struct Entity {
        Pokemon::PTR pokemon;
        const PokemonTurn *turn;
        bool move;
        int party, position;
        int priority;
        int speed;
        int inherentPriority;
    };
    const BattleMechanics *mechanics;
    bool descendingSpeed;
    bool operator()(const Entity &p1, const Entity &p2) {
        // first: is one pokemon switching?
        if (!p1.move && p2.move) {
            return true;    // p1 goes first
        } else if (p2.move && !p2.move) {
            return false;   // p2 goes first
        } else if (!p1.move && !p2.move) {
            if (p1.party == p2.party) {
                return (p1.position < p2.position);
            }
            // default to coinflip
            return mechanics->getCoinFlip();
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
            return descendingSpeed;
        } else if (p1.speed < p2.speed) {
            return !descendingSpeed;
        }

        // finally: coin flip
        return mechanics->getCoinFlip();
    }
};

/**
 * Sort a list of pokemon in turn order.
 */
void BattleFieldImpl::sortInTurnOrder(vector<Pokemon::PTR> &pokemon,
        vector<const PokemonTurn *> &turns) {
    vector<TurnOrder::Entity> entities;

    const int count = pokemon.size();
    assert(count == turns.size());
    for (int i = 0; i < count; ++i) {
        TurnOrder::Entity entity;
        Pokemon::PTR p = pokemon[i];
        const PokemonTurn *turn = turns[i];
        entity.pokemon = p;
        entity.turn = turn;
        entity.move = (turn->type == TT_MOVE);
        if (entity.move) {
            MoveObject *move = p->getMove(turn->id);
            assert(move != NULL);
            entity.priority = move->getPriority(context);
        }
        entity.party = p->getParty();
        entity.position = p->getPosition();
        entity.speed = p->getStat(S_SPEED);
        entity.inherentPriority = p->getInherentPriority();
        entities.push_back(entity);
    }

    // sort the entities
    TurnOrder order;
    order.descendingSpeed = descendingSpeed;
    order.mechanics = mech;
    sort(entities.begin(), entities.end(), order);

    // reorder the parameter vectors
    for (int i = 0; i < count; ++i) {
        TurnOrder::Entity &entity = entities[i];
        pokemon[i] = entity.pokemon;
        turns[i] = entity.turn;
    }
}

BattleField::BattleField() {
    m_impl = new BattleFieldImpl();
}

BattleField::~BattleField() {
    delete m_impl;
}

ScriptMachine *BattleField::getScriptMachine() {
    return m_impl->machine;
}

const BattleMechanics *BattleField::getMechanics() const {
    return m_impl->mech;
}

/**
 * Switch an active pokemon.
 */
void BattleField::switchPokemon(Pokemon *p, const int idx) {
    const int party = p->getParty();
    const int slot = p->getSlot();
    p->switchOut(); // note: clears slot
    Pokemon::PTR replacement = m_impl->teams[party][idx];
    (*m_impl->active[party])[slot].pokemon = replacement;
    informWithdraw(p);
    informSendOut(replacement.get());
    replacement->switchIn(slot);
}

/**
 * Print a message to the BattleField.
 */
void BattleField::print(const TextMessage &msg) {
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
    cout << trainer << " sent out " << p->getName() << "!" << endl;
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
    const int numerator = 48.0 * (double)delta / (double)p->getStat(S_HP) + 0.5;
    cout << p->getName() << " lost " << numerator << "/48 of its health!"
            << endl;
}

void BattleField::informFainted(Pokemon *p) {
    cout << p->getName() << " fainted!" << endl;
}

/**
 * Determine whether to veto the selection of a move.
 */
bool BattleField::vetoSelection(Pokemon *user, MoveObject *move) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && p->vetoSelection(user, move)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Determine whether to veto the execution of a move. A single effect wanting
 * to veto the execution is enough to do so.
 */
bool BattleField::vetoExecution(Pokemon *user, Pokemon *target,
        MoveObject *move) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && p->vetoExecution(user, target, move)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Begin the battle.
 */
void BattleField::beginBattle() {
    // todo: maybe sort these by speed? not sure on these mechanics
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p) {
                informSendOut(p.get());
                p->switchIn(j);
            }
        }
    }
}

namespace {

struct EffectEntity {
    Pokemon::PTR subject;
    StatusObject *effect;
    int speed;
    int tier;
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
                        const int tier = (*k)->getTier(cx);
                        // todo: subtier
                        EffectEntity entity = { p, *k, speed, tier, 0 };
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
        // todo: use subject? - maybe not.
        effect->tick(cx);

        if (i->tier == 6) {
            if (determineVictory()) {
                return;
            }
        }
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

    determineVictory();
}

/**
 * Determine whether the match has ended.
 */
bool BattleField::determineVictory() {
    vector<bool> fainted(TEAM_COUNT, true);
    int total = 0;
    for (int i = 0; i < TEAM_COUNT; ++i) {
        Pokemon::ARRAY &arr = m_impl->teams[i];
        const int size = arr.size();
        for (int j = 0; j < size; ++j) {
            if (!arr[j]->isFainted()) {
                fainted[i] = false;
                ++total;
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
 * Process a turn.
 */
void BattleField::processTurn(const vector<PokemonTurn> &turns) {
    vector<Pokemon::PTR> pokemon;
    getActivePokemon(pokemon);
    const int count = pokemon.size();
    if (count != turns.size())
        throw BattleFieldException();
    
    vector<const PokemonTurn *> ordered;
    for (int i = 0; i < count; ++i) {
        Pokemon::PTR p = pokemon[i];
        const PokemonTurn *turn = &turns[i];

        PokemonTurn *forced = p->getForcedTurn();
        if (forced) {
            turn = forced;
        }
        
        p->setTurn(turn);
        ordered.push_back(turn);
    }
    
    m_impl->sortInTurnOrder(pokemon, ordered);

    // begin the turn
    for (int i = 0; i < count; ++i) {
        Pokemon::PTR p = pokemon[i];
        p->clearRecentDamage();
        const PokemonTurn *turn = ordered[i];

        if (turn->type == TT_MOVE) {
            MoveObject *move = p->getMove(turn->id);
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

    // execute the actions
    for (int i = 0; i < count; ++i) {
        Pokemon::PTR p = pokemon[i];
        const PokemonTurn *turn = ordered[i];

        if (p->isFainted()) {
            // Can't execute anything if we've fainted.
            continue;
        }
        
        if (turn->type == TT_MOVE) {
            MoveObject *move = p->getMove(turn->id);
            Pokemon *target = NULL;
            TARGET tc = move->getTargetClass(m_impl->context);
            if ((tc == T_SINGLE) || (tc == T_ALLY)) {
                int idx = turn->target;
                assert(idx != -1);
                int party = 0;
                m_impl->decodeIndex(idx, party);
                Pokemon::PTR p = (*m_impl->active[party])[idx].pokemon;
                if (!p->isFainted()) {
                    target = p.get();
                }
            }

            const bool choice = (p->getForcedTurn() == NULL);
            p->clearForcedTurn();
            if (p->executeMove(move, target) && choice) {
                // only deduct pp if the move was chosen freely
                p->deductPp(turn->id);
            }
        } else {
            switchPokemon(p.get(), turn->id);
        }
    }

    if (determineVictory()) {
        return;
    }

    // Execute end of turn effects.
    tickEffects();
}

/**
 * Transform a status effect.
 */
void BattleField::transformStatus(Pokemon *subject, StatusObject **status) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p) {
                p->transformStatus(subject, status);
                if (status == NULL) {
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
        MoveObject &obj, const bool critical, MODIFIERS &mods) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            Pokemon::PTR p = slot.pokemon;
            if (p && !p->isFainted()) {
                p->getModifiers(&user, &target, &obj, critical, mods);
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
        const int activeParty) {
    try {
        m_impl->initialise(this, mech, generation,
                machine, teams, trainer, activeParty);
    } catch (BattleFieldException &e) {
        m_impl->cleanUp();
        throw e;
    }
}

void BattleFieldImpl::initialise(BattleField *field,
        const BattleMechanics *mech,
        GENERATION generation,
        ScriptMachine *machine,
        Pokemon::ARRAY teams[TEAM_COUNT],
        const std::string trainer[TEAM_COUNT],
        const int activeParty) {
    if (mech == NULL) {
        throw BattleFieldException();
    }
    if (machine == NULL) {
        throw BattleFieldException();
    }
    this->machine = machine;
    this->context = machine->acquireContext();
    this->object = context->newFieldObject(field);
    this->mech = mech;
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
            p->initialise(field, i, j);
        }
    }
}

}

