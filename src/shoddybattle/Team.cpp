/* 
 * File:   Team.cpp
 * Author: Catherine
 *
 * Created on April 2, 2009, 11:23 PM
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

#include "Team.h"
#include "ObjectTeamFile.h"
#include "PokemonSpecies.h"
#include "../mechanics/PokemonNature.h"
#include <iostream>

using namespace std;

namespace shoddybattle {

Pokemon::PTR getPokemon(const SpeciesDatabase *data, POKEMON &p) {
    const PokemonSpecies *species = data->getSpecies(p.species);
    const PokemonNature *nature = PokemonNature::getNature(p.nature);

    vector<string> moves;
    vector<int> ppUps;
    for (int i = 0; i < P_MOVE_COUNT; ++i) {
        moves.push_back(p.moves[i]);
        ppUps.push_back(p.ppUp[i]);
    }

    return Pokemon::PTR(new Pokemon(species,
            p.nickname,
            nature,
            p.ability,
            p.item,
            p.iv, p.ev,
            p.level,
            p.gender,
            p.shiny,
            moves, ppUps));
}

bool loadTeam(const std::string file,
        const SpeciesDatabase &data,
        Pokemon::ARRAY &team) {
    // right now, there are only object team files.
    vector<POKEMON> pokemon;
    if (!readObjectTeamFile(file, pokemon))
        return false;
    
    vector<POKEMON>::iterator i = pokemon.begin();
    for (; i != pokemon.end(); ++i) {
        Pokemon::PTR p = getPokemon(&data, *i);
        team.push_back(p);
    }
    return true;
}

}

