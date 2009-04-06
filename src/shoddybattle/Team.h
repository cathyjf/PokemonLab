/* 
 * File:   Team.h
 * Author: Catherine
 *
 * Created on April 2, 2009, 10:57 PM
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

#ifndef _TEAM_H_
#define _TEAM_H_

#include "Pokemon.h"
#include <string>

namespace shoddybattle {

class SpeciesDatabase;

/**
 * Load a team from disc into the provided Pokemon::ARRAY.
 */
bool loadTeam(const std::string file,
        const SpeciesDatabase &data,
        Pokemon::ARRAY &team);

}

#endif
