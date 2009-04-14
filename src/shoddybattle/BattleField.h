/* 
 * File:   BattleField.h
 * Author: Catherine
 *
 * Created on April 2, 2009, 10:10 PM
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

#ifndef _BATTLE_FIELD_H_
#define _BATTLE_FIELD_H_

#include <boost/shared_array.hpp>
#include <vector>
#include "Pokemon.h"
#include "../scripting/ObjectWrapper.h"

namespace shoddybattle {

const int TEAM_COUNT = 2;
const int MAX_TEAM_SIZE = 6;

class BattleFieldImpl;
class PokemonSlotImpl;

class BattleMechanics;

class ScriptMachine;
class ScriptContext;
class MoveObject;

class ScriptObject;

class BattleFieldException {
    
};

class TextMessage {
public:
    TextMessage(const int category, const int msg,
            const std::vector<std::string> &args = std::vector<std::string>()):
            m_category(category),
            m_msg(msg),
            m_args(args) { }
    int getCategory() const {
        return m_category;
    }
    int getMessage() const {
        return m_msg;
    }
    const std::vector<std::string> &getArgs() const {
        return m_args;
    }
private:
    const int m_category;
    const int m_msg;
    const std::vector<std::string> m_args;
};

enum TURN_TYPE {
    TT_MOVE = 0,
    TT_SWITCH = 1
};

struct PokemonTurn {
    TURN_TYPE type;
    int id;         // either id of move or the pokemon to which to switch
    int target;  // target of the move

    PokemonTurn(const TURN_TYPE tt, const int idx, const int ta = -1):
            type(tt),
            id(idx),
            target(ta) {

    }
};

struct PokemonSlot {
    Pokemon::PTR pokemon;
    std::string item;
};

struct PokemonParty {
public:
    PokemonParty(const int size): m_size(size) {
        m_party = boost::shared_array<PokemonSlot>(new PokemonSlot[size]);
    }
    PokemonSlot &operator[](const int i) {
        return m_party[i];
    }
    const STATUSES &getEffects() const {
        return m_effects;
    }
    int getSize() const {
        return m_size;
    }
private:
    const int m_size;
    boost::shared_array<PokemonSlot> m_party;
    STATUSES m_effects;     // party-specific status effects
};

/**
 * Encapsulate the idea of a battle, including turn processing, move execution,
 * field effects, and more.
 *
 * BattleField objects cannot be copied or assigned to.
 */
class BattleField : public ObjectWrapper {
public:
    BattleField();
    virtual ~BattleField();

    /**
     * Initialise the BattleField using the provided BattleMechanics and
     * pokemon teams. The activeParty parameter specifices the size of each
     * team's active party. For singles, this would be one. For doubles, it
     * would be two. Shoddy Battle actually accepts arbitrarily high values
     * of this number with mechanics "extrapolating" from the game. However,
     * activeParty > 2 is not strictly a pokemon simulation, since there is
     * no equilvalent mode in the real game.
     */
    void initialise(const BattleMechanics *mech,
            ScriptMachine *machine,
            Pokemon::ARRAY teams[TEAM_COUNT],
            const int activeParty);

    /**
     * Process a turn.
     */
    void processTurn(const std::vector<PokemonTurn> &turn);

    /**
     * Get the modifiers in play for a particular hit.
     */
    void getModifiers(Pokemon &, Pokemon &, MoveObject &,
            const bool, MODIFIERS &);

    /**
     * Sort a set of pokemon by speed.
     */
    void sortBySpeed(std::vector<Pokemon *> &pokemon);

    /**
     * Get the active pokemon.
     */
    boost::shared_ptr<PokemonParty> *getActivePokemon();

    /**
     * Determine whether the execution of a move should be vetoed.
     */
    bool vetoExecution(Pokemon *, Pokemon *, MoveObject *);
    
    /**
     * Obtain the BattleMechanics in use on this BattleField.
     */
    const BattleMechanics *getMechanics() const;

    /**
     * Print a message to the BattleField.
     */
    virtual void print(const TextMessage &msg);

    virtual void informHealthChange(Pokemon *, const int);

    virtual void informFainted(Pokemon *);

    ScriptMachine *getScriptMachine();

    ScriptContext *getContext();

    ScriptObject *getObject();

private:
    BattleFieldImpl *m_impl;
    BattleField(const BattleField &);
    BattleField &operator=(const BattleField &);
};

}

#endif
