/* 
 * File:   PokemonNature.cpp
 * Author: Catherine
 *
 * Created on March 28, 2009, 1:52 AM
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

#include "PokemonNature.h"

namespace shoddybattle {

/**
 * Set of all natures.
 */
const PokemonNature PokemonNature::m_natures[25] = {
    PokemonNature(0, S_NONE, S_NONE, "Hardy"),
    PokemonNature(1, S_ATTACK, S_DEFENCE, "Lonely"),
    PokemonNature(2, S_ATTACK, S_SPEED, "Brave"),
    PokemonNature(3, S_ATTACK, S_SPATTACK, "Adamant"),
    PokemonNature(4, S_ATTACK, S_SPDEFENCE, "Naughty"),
    PokemonNature(5, S_DEFENCE, S_ATTACK, "Bold"),
    PokemonNature(6, S_NONE, S_NONE, "Docile"),
    PokemonNature(7, S_DEFENCE, S_SPEED, "Relaxed"),
    PokemonNature(8, S_DEFENCE, S_SPATTACK, "Impish"),
    PokemonNature(9, S_DEFENCE, S_SPDEFENCE, "Lax"),
    PokemonNature(10, S_SPEED, S_ATTACK, "Timid"),
    PokemonNature(11, S_SPEED, S_DEFENCE, "Hasty"),
    PokemonNature(12, S_NONE, S_NONE, "Serious"),
    PokemonNature(13, S_SPEED, S_SPATTACK, "Jolly"),
    PokemonNature(14, S_SPEED, S_SPDEFENCE, "Naive"),
    PokemonNature(15, S_SPATTACK, S_ATTACK, "Modest"),
    PokemonNature(16, S_SPATTACK, S_DEFENCE, "Mild"),
    PokemonNature(17, S_SPATTACK, S_SPEED, "Quiet"),
    PokemonNature(18, S_NONE, S_NONE, "Bashful"),
    PokemonNature(19, S_SPATTACK, S_SPDEFENCE, "Rash"),
    PokemonNature(20, S_SPDEFENCE, S_ATTACK, "Calm"),
    PokemonNature(21, S_SPDEFENCE, S_DEFENCE, "Gentle"),
    PokemonNature(22, S_SPDEFENCE, S_SPEED, "Sassy"),
    PokemonNature(23, S_SPDEFENCE, S_SPATTACK, "Careful"),
    PokemonNature(24, S_NONE, S_NONE, "Quirky"),
};

const int PokemonNature::m_natureCount =
        sizeof(m_natures) / sizeof(m_natures[0]);

}

