/* 
 * File:   stat.h
 * Author: Catherine
 *
 * Created on March 28, 2009, 12:33 AM
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

#ifndef _STAT_H_
#define _STAT_H_

namespace shoddybattle {

/**
 * Number of "ordinary" stats, i.e., stats other than accuracy and evade.
 */
const int STAT_COUNT = 6;

/**
 * Total number of stats.
 */
const int TOTAL_STAT_COUNT = 8;

/**
 * The different pokemon stats.
 */
enum STAT {
    S_HP = 0,
    S_ATTACK = 1,
    S_DEFENCE = 2,
    S_SPEED = 3,
    S_SPATTACK = 4,
    S_SPDEFENCE = 5,
    S_ACCURACY = 6,
    S_EVASION = 7,
    S_NONE = -1
};

enum TARGET {
    T_USER,
    T_ALLY,
    T_ALLIES,
    T_SINGLE,
    T_ENEMIES,
    T_RANDOM_ENEMY,
    T_LAST_ENEMY,
    T_ENEMY_FIELD,
    T_OTHERS,
    T_ALL,
};

inline bool isEnemyTarget(const TARGET t) {
    return ((t == T_SINGLE) ||
            (t == T_ENEMIES) ||
            (t == T_RANDOM_ENEMY) ||
            (t == T_OTHERS));
    // T_LAST_ENEMY intentionally omitted
}

class StatException {
    
};

}

#endif


