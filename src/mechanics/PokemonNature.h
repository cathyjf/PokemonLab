/* 
 * File:   PokemonNature.h
 * Author: Catherine
 *
 * Created on March 28, 2009, 12:59 AM
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

#ifndef _POKEMON_NATURE_H_
#define _POKEMON_NATURE_H_

#include <string>
#include "stat.h"

namespace shoddybattle {

class PokemonNature {
public:

    /** Obtain an arbitrary nature by index. **/
    static const PokemonNature *getNature(const int idx) {
        return &m_natures[idx];
    }

    static const PokemonNature *getNatureByCanonicalName(
            const std::string &name) {
        for (int i = 0; i < m_natureCount; ++i) {
            const PokemonNature *nature = &m_natures[i];
            if (nature->m_name == name) {
                return nature;
            }
        }
        return NULL;
    }

    /** Determine if two natures are equal. **/
    bool operator==(const PokemonNature &rhs) const {
        return m_id == rhs.m_id;
    }
    bool operator!=(const PokemonNature &rhs) const {
        return !(*this == rhs);
    }

    /** Obtain the internal value of this nature. **/
    unsigned int getInternalValue() const {
        return m_id;
    }

    /** Return the effect of this nature on an arbitrary stat. **/
    double getEffect(STAT i) const {
        return (i == m_benefit) ? 1.1 : ((i == m_harm) ? 0.9 : 1.0);
    }

    /** Return which stat this nature benefits. **/
    STAT getBenefits() const {
        return m_benefit;
    }

    /** Return which stat this nature harms. **/
    STAT getHarms() const {
        return m_harm;
    }

private:
    STAT m_benefit, m_harm;
    unsigned int m_id; /** Internal ID of the nature. **/
    std::string m_name;
    PokemonNature(int internal, STAT benefit, STAT harm, std::string name):
       m_benefit(benefit), m_harm(harm), m_id(internal), m_name(name) { }
    static const PokemonNature m_natures[25];
    static const int m_natureCount;
};

}

#endif
