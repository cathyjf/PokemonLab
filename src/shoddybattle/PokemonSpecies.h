/* 
 * File:   PokemonSpecies.h
 * Author: Catherine
 *
 * Created on March 30, 2009, 6:52 PM
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

#ifndef _POKEMON_SPECIES_H_
#define _POKEMON_SPECIES_H_

#include <set>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../mechanics/stat.h"

namespace shoddybattle {

/**
 * Different ways a pokemon could have learned a move.
 */
enum MOVE_ORIGIN {
    MO_LEVEL,
    MO_TUTOR,
    MO_TM,
    MO_HM,
    MO_EGG,
    MO_NONE = -1
};

enum GENDER {
    G_MALE = 1,
    G_FEMALE = 2,
    G_BOTH = G_MALE | G_FEMALE,
    G_NONE = 0
};

class MoveTemplate;
typedef std::map<std::string, const MoveTemplate *> MOVE_LIST;

typedef std::map<MOVE_ORIGIN, std::vector<std::string> > MOVESET;

class PokemonSpecies;
typedef std::map<int, PokemonSpecies *> SPECIES_SET;

class PokemonType;
typedef std::vector<const PokemonType *> TYPE_LIST;

typedef std::vector<std::string> ABILITY_LIST;

class MoveDatabase;
class SpeciesDatabase;

/**
 * A species of pokemon. Each PokemonSpecies object is immutable and cannot be
 * copied or assigned to.
 */
class PokemonSpecies {
public:
    /**
     * Load a set of species by reading a species XML file.
     */
    static bool loadSpecies(const std::string file, SpeciesDatabase &set);

    unsigned int getBaseStat(STAT i) const { return m_base[i]; }
    std::string getSpeciesName() const { return m_name; }
    unsigned int getSpeciesId() const { return m_id; }
    unsigned int getPossibleGenders() const { return m_gender; }
    const TYPE_LIST &getTypes() const { return m_types; }
    const MOVESET &getMoveset() const { return m_moveset; }
    const ABILITY_LIST &getAbilities() const { return m_abilities; }
    const MOVE_LIST &getMoveList() const { return m_moves; }

    std::set<std::string> populateMoveList(const MoveDatabase &);
    const MoveTemplate *getMove(const std::string name) const {
        return m_moves[name];
    }

private:
    PokemonSpecies(void *);

    PokemonSpecies(const PokemonSpecies &);
    PokemonSpecies &operator=(const PokemonSpecies &);
    
    std::string m_name;
    unsigned int m_id;
    unsigned int m_gender;  // Possible genders for this species.
    TYPE_LIST m_types;
    unsigned int m_base[STAT_COUNT];
    MOVESET m_moveset;
    mutable MOVE_LIST m_moves;
    double m_mass;
    ABILITY_LIST m_abilities;
};

/**
 * A database of species, loaded from an XML file.
 */
class SpeciesDatabase {
public:
    SpeciesDatabase() { }
    SpeciesDatabase(const std::string file) {
        loadSpecies(file);
    }
    void loadSpecies(const std::string file) {
        PokemonSpecies::loadSpecies(file, *this);
    }
    const PokemonSpecies *getSpecies(const int id) const {
        return m_set[id];
    }
    const PokemonSpecies *getSpecies(const std::string name) const {
        SPECIES_SET::const_iterator i = m_set.begin();
        for (; i != m_set.end(); ++i) {
            const PokemonSpecies *p = i->second;
            if (p->getSpeciesName() == name)
                return p;
        }
        return NULL;
    }
    const SPECIES_SET &getSpeciesSet() const {
        return m_set;
    }
    /**
     * Populate the move lists of the species in this database, using the
     * provided move database. Returns the set of moves in the SpeciesDatabase
     * that were not found in the MoveDatabase.
     */
    std::set<std::string> populateMoveLists(const MoveDatabase &p) {
        std::set<std::string> ret;
        SPECIES_SET::iterator i = m_set.begin();
        for (; i != m_set.end(); ++i) {
            std::set<std::string> set = i->second->populateMoveList(p);
            ret.insert(set.begin(), set.end());
        }
        return ret;
    }
    ~SpeciesDatabase() {
        SPECIES_SET::iterator i = m_set.begin();
        for (; i != m_set.end(); ++i) {
            delete i->second;
        }
    }
private:
    mutable SPECIES_SET m_set;
    SpeciesDatabase(const SpeciesDatabase &);
    SpeciesDatabase &operator=(const SpeciesDatabase &);
    friend bool PokemonSpecies::loadSpecies(const std::string file,
            SpeciesDatabase &set);
};

}

#endif
