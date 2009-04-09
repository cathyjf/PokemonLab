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
 * Remove defunct statuses from this pokemon.
 */
void Pokemon::removeStatuses(ScriptContext *cx) {
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        if (!(*i)->isRemovable(cx))
            continue;

        // this is a list so it is safe to remove an arbitrary element
        m_effects.erase(i);
    }
}

/**
 * Apply a StatusEffect to this pokemon. Makes a copy of the parameter before
 * applying it to the pokemon.
 */
StatusObject *Pokemon::applyStatus(ScriptContext *cx,
        Pokemon *inducer, StatusObject *effect) {
    if (!effect)
        return NULL;
    // TODO: implement properly

    StatusObject *applied = effect->cloneAndRoot(cx);
    applied->setInducer(cx, inducer);
    applied->setSubject(cx, this);
    if (!applied->applyEffect(cx)) {
        cx->removeRoot(applied);
        return NULL;
    }

    m_effects.push_back(applied);
    return applied;
}

/**
 * Check if this Pokemon has "inherent priority". This is used for certain
 * items and abilities.
 */
int Pokemon::getInherentPriority(ScriptContext *cx) const {
    int ret = 0;
    for (STATUSES::const_iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
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
 * Check for modifiers on all status effects.
 */
void Pokemon::getModifiers(ScriptContext *cx, BattleField *field,
        Pokemon *user, Pokemon *target,
        MoveObject *obj, const bool critical, MODIFIERS &mods) {
    MODIFIER mod;
    for (STATUSES::iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
        // TODO: look in overrides

        if (!(*i)->isActive(cx))
            continue;

        if ((*i)->getModifier(cx, field, user, target, obj, critical, mod)) {
            mods[mod.position][mod.priority] = mod.value;
        }
    }
}

/**
 * Set the current hp of the pokemon, and also inform the BattleField, which
 * can cause side effects such as the printing of messages.
 */
void Pokemon::setHp(const int hp) {
    // TODO: implement this function properly
    const int delta = m_hp - hp;
    m_hp = hp;
    cout << m_nickname << " lost " << delta << " hp!" << endl;
    if (m_hp <= 0) {
        cout << m_nickname << " fainted!" << endl;
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

    m_machine->releaseContext(cx);
}

}

/**int main(int argc, char **argv) {
    Text text("languages/english.lang");
    string nature = text.getText(SC_NATURE, 6, 2, "[a]", "[b]");
    cout << nature << endl;

    int base[] = { 10, 20, 30, 40, 50, 60 };
    int iv[] = { 21, 30, 15, 31, 25, 31 };
    int ev[] = { 200, 120, 100, 0, 0, 0 };
    vector<const PokemonType *> types;
    types.push_back(&PokemonType::GROUND);
    Pokemon pokemon(base,
            iv, ev, PokemonNature::getNature(5), 100, types);
    JewelMechanics mechanics;
    for (int i = 0; i < 6; ++i) {
        cout << "Stat " << i << ": "
            << mechanics.calculateStat(pokemon, (STAT)i) << "\n";
    }
}**/
