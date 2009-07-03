/* 
 * File:   PokemonMove.h
 * Author: Catherine
 *
 * Created on March 29, 2009, 2:43 AM
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

#ifndef _POKEMON_MOVE_H_
#define _POKEMON_MOVE_H_

#include <string>
#include <map>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include "../mechanics/stat.h"

namespace shoddybattle {

class Pokemon;
class PokemonType;
class BattleField;

enum MOVE_CLASS {
    MC_PHYSICAL = 0,
    MC_SPECIAL,
    MC_OTHER
};

const int FLAG_COUNT = 11;

enum MOVE_FLAG {
    F_CONTACT,          // Makes contact?
    F_PROTECT,          // Can be stopped by Protect and Detect?
    F_FLINCH,           // Can gain a flinch chance?
    F_REFLECT,          // Can be reflected by Magic Coat?
    F_SNATCH,           // Can be snatched?
    F_MEMORABLE,        // Using this move qualifies a pokemon for the target
                        // of a "lastenemy" targeting move.
    F_HIGH_CRITICAL,    // Does this move have a high chance of a critical hit?
    F_UNIMPLEMENTED,    // Is this move unimplemented?
    F_INTERNAL,         // Is this move for internal use only?
    F_NO_CRITICAL,      // Does this move never critical?
    F_SOUND             // Is this move sound-based?
};

class ScriptMachine;
class ScriptContext;
class ScriptObject;
class ScriptFunction;

class MoveTemplateImpl;

class MoveTemplate {
public:
    std::string getName() const;
    int getId() const;
    MOVE_CLASS getMoveClass() const;
    TARGET getTargetClass() const;
    unsigned int getPp() const;
    double getAccuracy() const;
    unsigned int getPower() const;
    int getPriority() const;
    const PokemonType *getType() const;
    bool getFlag(const MOVE_FLAG i) const;
    boost::shared_ptr<ScriptFunction> getInitFunction() const;
    boost::shared_ptr<ScriptFunction> getUseFunction() const;
    boost::shared_ptr<ScriptFunction> getAttemptHitFunction() const;

private:
    friend class MoveDatabase;
    MoveTemplate(MoveTemplateImpl *);
    ~MoveTemplate();
    MoveTemplateImpl *m_pImpl;

    MoveTemplate(const MoveTemplate &);
    MoveTemplate &operator=(const MoveTemplate &);
};

typedef std::map<std::string, MoveTemplate *> MOVE_DATABASE;
typedef std::map<int, std::string> INT_MOVE_DATABASE;

class MoveDatabase {
public:
    MoveDatabase(ScriptMachine &machine): m_machine(machine) { }

    /**
     * Load moves from an xml file. Can be called more than once.
     */
    void loadMoves(const std::string file);

    /**
     * Get a move by ID.
     */
    std::string getMove(const int id) const {
        return m_intData[id];
    }

    /**
     * Get a move by name.
     */
    const MoveTemplate *getMove(const std::string name) const {
        return m_data[name];
    }

    ~MoveDatabase();

private:
    mutable MOVE_DATABASE m_data;
    mutable INT_MOVE_DATABASE m_intData;
    ScriptMachine &m_machine;
    MoveDatabase(const MoveDatabase &);
    MoveDatabase &operator=(const MoveDatabase &);
};

}

#endif
