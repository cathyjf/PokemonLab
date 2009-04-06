/* 
 * File:   ObjectTeamFile.h
 * Author: Catherine
 *
 * Created on March 31, 2009, 1:03 AM
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

/**
 * The first version of Shoddy Battle used a team format based on the Java
 * serialisation protocol via the Java ObjectInputStream and ObjectOutputStream
 * classes.
 *
 * This file provides a function that can be used to load a Shoddy Battle 1
 * team file.
 */

#ifndef _OBJECT_TEAM_FILE_H_
#define _OBJECT_TEAM_FILE_H_

#include <string>
#include <vector>

namespace shoddybattle {

const static int P_MOVE_COUNT = 4;
const static int P_STAT_COUNT = 6;

/**
 * The Pokemon data read directly from the file.
 */
struct POKEMON {
    std::string species;
    int speciesId;
    int level;
    int nature; // (Internal ID.)
    std::string moves[P_MOVE_COUNT];
    int ppUp[P_MOVE_COUNT];
    std::string ability;
    std::string item;
    bool shiny;
    int gender;
    std::string nickname;
    int iv[P_STAT_COUNT];
    int ev[P_STAT_COUNT];
};

/**
 * Load a Shoddy Battle 1 team file ("Object Team File") and populate the
 * vector with the pokemon from the file.
 *
 * Returns false if the file is not an Object Team File.
 */
bool readObjectTeamFile(const std::string file, std::vector<POKEMON> &p);

}

#endif
