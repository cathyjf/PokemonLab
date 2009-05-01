/*
 * File:   stat.cpp
 * Author: Catherine
 *
 * Created on April 30, 2009, 11:58 PM
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

#include "stat.h"

namespace shoddybattle {

namespace {

double STAT_MULTIPLIER[] = { 4.0, 3.5, 3.0, 2.5, 2.0, 1.5, 1.0,
        2.0/3.0, 0.5, 0.4, 1.0/3.0, 2.0/7.0, 0.25 };

double EVASION_MULTIPLIER[] = { 3.0, 8.0/3.0, 7.0/3.0, 2.0, 5.0/3.0, 4.0/3.0,
        1.0, 0.75, 0.6, 0.5, 3.0/7.0, 3.0/8.0, 1.0/3.0 };

} // anonymous namespace

double getStatMultiplier(const STAT i, const int level) {
    if ((i != S_EVASION) && (i != S_ACCURACY)) {
        return STAT_MULTIPLIER[6 - level];
    }
    return EVASION_MULTIPLIER[6 - level];
}

}

