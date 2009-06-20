/*
 * File:   constants.js
 * Author: Catherine
 *
 * Created on April 14, 2009, 2:41 AM
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

const Generation = {
    DP : 0,
    PLATINUM : 1,
    PLATINUM_FAKE : 2
};

const Stat = {
    HP : 0,
    ATTACK : 1,
    DEFENCE : 2,
    SPEED : 3,
    SPATTACK : 4,
    SPDEFENCE : 5,
    ACCURACY : 6,
    EVASION : 7,
    NONE : -1
};

const MoveClass = {
    PHYSICAL : 0,
    SPECIAL : 1,
    OTHER : 2
};

const Type = {
    NORMAL : 0,
    FIRE : 1,
    WATER : 2,
    ELECTRIC : 3,
    GRASS : 4,
    ICE : 5,
    FIGHTING : 6,
    POISON : 7,
    GROUND : 8,
    FLYING : 9,
    PSYCHIC : 10,
    BUG : 11,
    ROCK : 12,
    GHOST : 13,
    DRAGON : 14,
    DARK : 15,
    STEEL : 16,
    TYPELESS : 17
};

const Gender = {
    MALE : 1,
    FEMALE : 2,
    NONE : 0
};

const Flag = {
    CONTACT : 0,
    PROTECT : 1,
    FLINCH : 2,
    REFLECT : 3,
    SNATCH : 4,
    MEMORABLE : 5,
    HIGH_CRITICAL : 6,
    UNIMPLEMENTED : 7,
    INTERNAL : 8,
    NO_CRITICAL : 9
};

