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

#include <boost/shared_array.hpp>
#include "BattleField.h"
#include "../mechanics/BattleMechanics.h"
#include "../scripting/ScriptMachine.h"
#include <iostream>
#include <list>

using namespace std;
using namespace boost;

namespace shoddybattle {

struct BattleFieldImpl {
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
};

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

    // TODO: beginTurn

    for (int i = 0; i < count; ++i) {
        Pokemon::PTR p = pokemon[i];
        const PokemonTurn *turn = ordered[i];

        cout << p->getSpeciesName() << ", ";
        cout << p->getMove(turn->id)->getName(m_impl->context) << ", ";
        cout << p->getStat(S_SPEED) << endl;
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
                        this, &user, &target, &obj, critical, mods);
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
}

}

