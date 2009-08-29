/* 
 * File:   glicko2.h
 * Author: Catherine
 *
 * Created on August 28, 2009, 1:06 PM
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

#ifndef _GLICKO2_H_
#define _GLICKO2_H_

#include <vector>

namespace shoddybattle { namespace glicko2 {

struct MATCH {
    int score;
    double opponentRating;
    double opponentDeviation;
};

struct PLAYER {
    double rating, deviation, volatility;
};

/**
 * Find a player's new score, deviation, and volatility at the end of
 * the rating period.
 */
void updatePlayer(PLAYER &player,
        const std::vector<MATCH> &matches,
        const double system);

/**
 * Get a rating estimate based on a mean rating and rating deviation using
 * the GLIXARE approach (i.e. returns the chance of a player with the given
 * stats winning against a player who has just joined the ladder).
 */
double getRatingEstimate(const double rating, const double deviation);

}} // namespace shoddybattle::glicko2

#endif

