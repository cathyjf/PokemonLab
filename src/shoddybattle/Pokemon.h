/* 
 * File:   Pokemon.h
 * Author: Catherine
 *
 * Created on March 28, 2009, 12:13 AM
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

#ifndef _POKEMON_H_
#define _POKEMON_H_

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <list>
#include <map>
#include "../mechanics/stat.h"
#include "../scripting/ObjectWrapper.h"

namespace shoddybattle {

class BattleField;
class PokemonNature;
class PokemonType;
class PokemonSpecies;
class MoveTemplate;
class PokemonTurn;

class PokemonObject;
class MoveObject;
class StatusObject;

class ScriptMachine;
class ScriptContext;

class Target;

typedef std::vector<const PokemonType *> TYPE_ARRAY;
typedef std::list<StatusObject *> STATUSES;

typedef std::map<int, double> PRIORITY_MAP;
// map<position, map<priority, value>>
typedef std::map<int, PRIORITY_MAP> MODIFIERS;

/**
 * The pokemon class contains all of the information about an arbitrary
 * Pokemon, including its stats, the moves it can use, its most recent actions,
 * its species, a battle field to which it may be attached, and so forth.
 *
 * Pokemon objects are mutable, but they cannot be copied or assigned to.
 */
class Pokemon : public ObjectWrapper {
public:
    typedef boost::shared_ptr<Pokemon> PTR;
    typedef std::vector<PTR> ARRAY;
    
    Pokemon(const PokemonSpecies *species,
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
            const std::vector<int> &ppUps);

    void initialise(BattleField *field, const int i, const int j);

    void setTurn(PokemonTurn *turn) { m_turn = turn; }
    PokemonTurn *getTurn() { return m_turn; }

    int getInherentPriority(ScriptContext *) const;
    int getCriticalModifier(ScriptContext *) const;

    bool executeMove(ScriptContext *, MoveObject *, Pokemon *target);
    bool useMove(ScriptContext *, MoveObject *, Pokemon *, const int);
    bool vetoExecution(ScriptContext *, Pokemon *, Pokemon *, MoveObject *);

    void informTargeted(ScriptContext *, Pokemon *, MoveObject *);
    
    const STATUSES &getEffects() const { return m_effects; }
    StatusObject *applyStatus(ScriptContext *, Pokemon *, StatusObject *);
    StatusObject *getStatus(ScriptContext *, const std::string);
    void getModifiers(ScriptContext *,
            Pokemon *, Pokemon *, MoveObject *, const bool, MODIFIERS &);
    void getStatModifiers(ScriptContext *,
            STAT, Pokemon *, PRIORITY_MAP &);
    void removeStatuses(ScriptContext *);
    
    int getHp() const { return m_hp; }
    void setHp(const int hp, const bool indirect = false);

    std::string getSpeciesName() const;
    std::string getName() const { return m_nickname; }

    unsigned int getBaseStat(const STAT i) const;
    unsigned int getIv(const STAT i) const { return m_iv[i]; }
    unsigned int getEv(const STAT i) const { return m_ev[i]; }
    unsigned int getStat(const STAT i) const { return m_stat[i]; }

    unsigned int getLevel() const { return m_level; }
    const PokemonNature *getNature() const { return m_nature; }
    const TYPE_ARRAY &getTypes() const { return m_types; }
    const std::vector<int> &getPpUps() const { return m_ppUps; }
    unsigned int getGender() const { return m_gender; }
    bool isShiny() const { return m_shiny; }

    ScriptObject *getObject() { return (ScriptObject *)m_object; }

    MoveObject *getMove(const int i) {
        if (m_moves.size() <= i)
            return NULL;
        return m_moves[i];
    }

    int getMoveCount() const { return m_moves.size(); }

    int getParty() const { return m_party; }
    int getPosition() const { return m_position; }

    bool isFainted() const { return m_fainted; }
    
    ~Pokemon();

private:
    const PokemonSpecies *m_species;
    unsigned int m_level;
    int m_hp; // The remaining health of the pokemon.
    bool m_fainted;
    unsigned int m_stat[STAT_COUNT];
    unsigned int m_iv[STAT_COUNT];
    unsigned int m_ev[STAT_COUNT];
    int m_statLevel[TOTAL_STAT_COUNT];  // Level of stat boost.
    unsigned int m_gender;    // This pokemon's gender.
    bool m_shiny;
    const PokemonNature *m_nature;
    TYPE_ARRAY m_types; // Current effective type.
    std::vector<int> m_ppUps;
    std::vector<const MoveTemplate *> m_moveProto;
    std::vector<MoveObject *> m_moves;
    std::string m_nickname;
    std::string m_itemName;
    std::string m_abilityName;

    ScriptMachine *m_machine;
    BattleField *m_field;
    int m_party, m_position;

    STATUSES m_effects;

    PokemonTurn *m_turn;

    PokemonObject *m_object; // Pokemon object.

    Pokemon(const Pokemon &);
    Pokemon &operator=(const Pokemon &);
};

}

#endif
