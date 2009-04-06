/* 
 * File:   PokemonType.h
 * Author: Catherine
 *
 * Created on March 28, 2009, 2:01 AM
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

#include <string>

namespace shoddybattle {

class Text;

const int TYPE_COUNT = 18;

class PokemonType {
public:
    /** Constants for all of the types. */
    static const PokemonType NORMAL;
    static const PokemonType FIRE;
    static const PokemonType WATER;
    static const PokemonType ELECTRIC;
    static const PokemonType GRASS;
    static const PokemonType ICE;
    static const PokemonType FIGHTING;
    static const PokemonType POISON;
    static const PokemonType GROUND;
    static const PokemonType FLYING;
    static const PokemonType PSYCHIC;
    static const PokemonType BUG;
    static const PokemonType ROCK;
    static const PokemonType GHOST;
    static const PokemonType DRAGON;
    static const PokemonType DARK;
    static const PokemonType STEEL;
    static const PokemonType TYPELESS;

    double getMultiplier(const PokemonType &type) const {
        return m_multiplier[m_type][type.m_type];
    }

    static const PokemonType *getByCanonicalName(const std::string name) {
        for (int i = 0; i < TYPE_COUNT; ++i) {
            const PokemonType *p = m_list[i];
            if (p->m_name == name)
                return p;
        }
        return NULL;
    }

    /**
     * Get the name of this type.
     */
    std::string getName(Text &) const;
    
    int getTypeValue() const {
        return m_type;
    }

private:
    unsigned int m_type;
    std::string m_name; // "Canonical name" - not to be displayed to the user
    static const PokemonType *m_list[TYPE_COUNT];
    static const double m_multiplier[TYPE_COUNT][TYPE_COUNT];

    PokemonType(int type, std::string name): m_type(type), m_name(name) { }
};

}

