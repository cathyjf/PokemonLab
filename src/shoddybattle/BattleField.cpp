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
    ScriptMachine *machine;
    ScriptContext *context;
    Pokemon::ARRAY teams[TEAM_COUNT];
    int partySize;
    shared_ptr<PokemonParty> active[TEAM_COUNT];
    STATUSES m_effects;      // field effects
    bool descendingSpeed;

    void sortInTurnOrder(vector<Pokemon::PTR> &, vector<const PokemonTurn *> &);
    void getActivePokemon(vector<Pokemon::PTR> &);
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
};

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

shared_ptr<PokemonParty> *BattleField::getActivePokemon() {
    return m_impl->active;
}

/**
 * Place the active pokemon into a single vector, indepedent of party.
 */
void BattleFieldImpl::getActivePokemon(vector<Pokemon::PTR> &v) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        PokemonParty &party = *active[i];
        for (int j = 0; j < partySize; ++j) {
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
        entity.inherentPriority = p->getInherentPriority(context);
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
    m_impl->context->removeRoot(m_impl->object);
    m_impl->machine->releaseContext(m_impl->context);
    delete m_impl;
}

ScriptMachine *BattleField::getScriptMachine() {
    return m_impl->machine;
}

const BattleMechanics *BattleField::getMechanics() const {
    return m_impl->mech;
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

void BattleField::informHealthChange(Pokemon *p, const int delta) {
    const int numerator = 48.0 * (double)delta / (double)p->getStat(S_HP) + 0.5;
    cout << p->getName() << " lost " << numerator << "/48 of its health!"
            << endl;
}

void BattleField::informFainted(Pokemon *p) {
    cout << p->getName() << " fainted!" << endl;
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
            if (p && p->vetoExecution(m_impl->context, user, target, move)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Process a turn.
 */
void BattleField::processTurn(const vector<PokemonTurn> &turns) {
    vector<Pokemon::PTR> pokemon;
    m_impl->getActivePokemon(pokemon);
    const int count = pokemon.size();
    if (count != turns.size())
        throw BattleFieldException();

    vector<const PokemonTurn *> ordered;
    for (int i = 0; i < count; ++i) {
        ordered.push_back(&turns[i]);
    }
    
    m_impl->sortInTurnOrder(pokemon, ordered);

    // begin the turn
    for (int i = 0; i < count; ++i) {
        Pokemon::PTR p = pokemon[i];
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
            if (move->getTargetClass(m_impl->context) == T_SINGLE) {
                int idx = turn->target;
                assert(idx != -1);
                int party = 0;
                m_impl->decodeIndex(idx, party);
                Pokemon::PTR p = (*m_impl->active[party])[idx].pokemon;
                if (!p->isFainted()) {
                    target = p.get();
                }
            }

            p->executeMove(m_impl->context, move, target);
        } else {
            // handle switch
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
            if (p) {
                p->getModifiers(m_impl->context,
                        &user, &target, &obj, critical, mods);
            }
        }
    }
}

/**
 * Get the ScriptContext associated with this BattleField. This is to be called
 * only from within the logic of a battle turn.
 */
ScriptContext *BattleField::getContext() {
    return m_impl->context;
}

void BattleField::initialise(const BattleMechanics *mech,
        ScriptMachine *machine,
        Pokemon::ARRAY teams[TEAM_COUNT],
        const int activeParty) {
    if (mech == NULL) {
        throw BattleFieldException();
    }
    if (machine == NULL) {
        throw BattleFieldException();
    }
    m_impl->mech = mech;
    m_impl->machine = machine;
    m_impl->partySize = activeParty;
    m_impl->descendingSpeed = true;
    for (int i = 0; i < TEAM_COUNT; ++i) {
        m_impl->teams[i] = teams[i];
        m_impl->active[i] =
                shared_ptr<PokemonParty>(new PokemonParty(activeParty));

        // Set first n pokemon to be the active pokemon.
        int bound = activeParty;
        const int size = teams[i].size();
        if (activeParty > size) {
            bound = size;
        }
        for (int j = 0; j < bound; ++j) {
            PokemonSlot &slot = (*m_impl->active[i])[j];
            slot.pokemon = m_impl->teams[i][j];
        }

        // Do some basic initialisation on each pokemon.
        Pokemon::ARRAY &team = m_impl->teams[i];
        const int length = team.size();
        for (int j = 0; j < length; ++j) {
            Pokemon::PTR p = team[j];
            if (!p) {
                throw BattleFieldException();
            }
            p->initialise(this, i, j);
        }
    }
    m_impl->context = m_impl->machine->acquireContext();
    m_impl->object = m_impl->context->newFieldObject(this);
}

}

