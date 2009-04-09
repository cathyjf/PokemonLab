/* 
 * File:   PokemonType.cpp
 * Author: Catherine
 *
 * Created on March 28, 2009, 2:03 AM
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

#include "PokemonType.h"
#include "../text/Text.h"
#include <string>

using namespace std;

namespace shoddybattle {

/**
 * Pokemon type constants.
 */
const PokemonType PokemonType::NORMAL(0, "Normal");
const PokemonType PokemonType::FIRE(1, "Fire");
const PokemonType PokemonType::WATER(2, "Water");
const PokemonType PokemonType::ELECTRIC(3, "Electric");
const PokemonType PokemonType::GRASS(4, "Grass");
const PokemonType PokemonType::ICE(5, "Ice");
const PokemonType PokemonType::FIGHTING(6, "Fighting");
const PokemonType PokemonType::POISON(7, "Poison");
const PokemonType PokemonType::GROUND(8, "Ground");
const PokemonType PokemonType::FLYING(9, "Flying");
const PokemonType PokemonType::PSYCHIC(10, "Psychic");
const PokemonType PokemonType::BUG(11, "Bug");
const PokemonType PokemonType::ROCK(12, "Rock");
const PokemonType PokemonType::GHOST(13, "Ghost");
const PokemonType PokemonType::DRAGON(14, "Dragon");
const PokemonType PokemonType::DARK(15, "Dark");
const PokemonType PokemonType::STEEL(16, "Steel");
const PokemonType PokemonType::TYPELESS(17, "Typeless");

const PokemonType *PokemonType::m_list[18] = {
    &PokemonType::NORMAL,
    &PokemonType::FIRE,
    &PokemonType::WATER,
    &PokemonType::ELECTRIC,
    &PokemonType::GRASS,
    &PokemonType::ICE,
    &PokemonType::FIGHTING,
    &PokemonType::POISON,
    &PokemonType::GROUND,
    &PokemonType::FLYING,
    &PokemonType::PSYCHIC,
    &PokemonType::BUG,
    &PokemonType::ROCK,
    &PokemonType::GHOST,
    &PokemonType::DRAGON,
    &PokemonType::DARK,
    &PokemonType::STEEL,
    &PokemonType::TYPELESS
};

/**
 * Type effectiveness chart.
 */
const double PokemonType::m_multiplier[18][18] = {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.5, 0, 1, 1, 0.5, 1 },
	{ 1, 0.5, 0.5, 1, 2, 2, 1, 1, 1, 1, 1, 2, 0.5, 1, 0.5, 1, 2, 1 },
	{ 1, 2, 0.5, 1, 0.5, 1, 1, 1, 2, 1, 1, 1, 2, 1, 0.5, 1, 1, 1 },
	{ 1, 1, 2, 0.5, 0.5, 1, 1, 1, 0, 2, 1, 1, 1, 1, 0.5, 1, 1, 1 },
	{ 1, 0.5, 2, 1, 0.5, 1, 1, 0.5, 2, 0.5, 1, 0.5, 2, 1, 0.5, 1, 0.5, 1 },
	{ 1, 0.5, 0.5, 1, 2, 0.5, 1, 1, 2, 2, 1, 1, 1, 1, 2, 1, 0.5, 1 },
	{ 2, 1, 1, 1, 1, 2, 1, 0.5, 1, 0.5, 0.5, 0.5, 2, 0, 1, 2, 2, 1 },
	{ 1, 1, 1, 1, 2, 1, 1, 0.5, 0.5, 1, 1, 1, 0.5, 0.5, 1, 1, 0, 1 },
	{ 1, 2, 1, 2, 0.5, 1, 1, 2, 1, 0, 1, 0.5, 2, 1, 1, 1, 2, 1 },
	{ 1, 1, 1, 0.5, 2, 1, 2, 1, 1, 1, 1, 2, 0.5, 1, 1, 1, 0.5, 1 },
	{ 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 0.5, 1, 1, 1, 1, 0, 0.5, 1 },
	{ 1, 0.5, 1, 1, 2, 1, 0.5, 0.5, 1, 0.5, 2, 1, 1, 0.5, 1, 2, 0.5, 1 },
	{ 1, 2, 1, 1, 1, 2, 0.5, 1, 0.5, 2, 1, 2, 1, 1, 1, 1, 0.5, 1 },
	{ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 0.5, 0.5, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 0.5, 1 },
	{ 1, 1, 1, 1, 1, 1, 0.5, 1, 1, 1, 2, 1, 1, 2, 1, 0.5, 0.5, 1 },
	{ 1, 0.5, 0.5, 0.5, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 0.5, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    };

}

