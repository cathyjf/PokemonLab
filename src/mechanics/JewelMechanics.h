/* 
 * File:   JewelMechanics.h
 * Author: Catherine
 *
 * Created on March 28, 2009, 12:45 AM
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

#ifndef _JEWEL_MECHANICS_H_
#define _JEWEL_MECHANICS_H_

#include "BattleMechanics.h"

namespace shoddybattle {

class Pokemon;
class BattleField;
class MoveObject;

class JewelMechanicsImpl;

class JewelMechanics : public BattleMechanics {
public:
    JewelMechanics();
    ~JewelMechanics();
    bool getCoinFlip(double) const;
    unsigned int calculateStat(const Pokemon &p, const STAT i) const;
    int calculateDamage(BattleField &field, MoveObject &move,
            Pokemon &user, Pokemon &target, const int targets) const;
    bool isCriticalHit(BattleField &field, MoveObject &move,
            Pokemon &user, Pokemon &target) const;
    bool attemptHit(BattleField &field, MoveObject &move,
            Pokemon &user, Pokemon &target) const;
    int getRandomInt(const int lower, const int upper) const;
private:
    JewelMechanicsImpl *m_impl;
};

}

#endif
