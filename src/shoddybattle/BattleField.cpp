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
#include "../scripting/ScriptMachine.h"
#include <iostream>

using namespace std;
using namespace boost;

namespace shoddybattle {

struct BattleFieldImpl {
    const BattleMechanics *mech;
    ScriptMachine *machine;
    ScriptContext *context;
    Pokemon::ARRAY teams[TEAM_COUNT];
    int partySize;
    shared_array<Pokemon::PTR> active[TEAM_COUNT];
};

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
 * Get the modifiers in play for a particular hit. Checks all of the active
 * pokemon for "modifier" properties.
 */
void BattleField::getModifiers(Pokemon &user, Pokemon &target,
        MoveObject &obj, const bool critical, MODIFIERS &mods) {
    for (int i = 0; i < TEAM_COUNT; ++i) {
        for (int j = 0; j < m_impl->partySize; ++j) {
            Pokemon::PTR p = m_impl->active[i][j];
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
    for (int i = 0; i < TEAM_COUNT; ++i) {
        m_impl->teams[i] = teams[i];
        m_impl->active[i] = shared_array<Pokemon::PTR>(
                new Pokemon::PTR[activeParty]);

        // Set first n pokemon to be the active pokemon.
        int bound = activeParty;
        const int size = teams[i].size();
        if (activeParty > size) {
            bound = size;
        }
        for (int j = 0; j < bound; ++j) {
            m_impl->active[i][j] = m_impl->teams[i][j];
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

