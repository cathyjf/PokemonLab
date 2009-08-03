/*
 * File:   BattleMechanics.h
 * Author: Catherine
 *
 * Created on March 28, 2009, 12:24 AM
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

#ifndef _BATTLE_MECHANICS_H_
#define _BATTLE_MECHANICS_H_

#include "stat.h"
#include <vector>

namespace shoddybattle {

class Pokemon;
class BattleField;
class MoveObject;
class PokemonType;

/**
 * A general notion of battle mechanics, allowing for the calculation of some
 * fundamental quantities.
 */
class BattleMechanics {
public:
    virtual bool getCoinFlip(double = 0.5) const = 0;
    virtual unsigned int calculateStat(const Pokemon &p, const STAT i) const = 0;
    virtual int calculateDamage(BattleField &field, MoveObject &move,
        Pokemon &user, Pokemon &target, const int targets,
        const bool weight = true) const = 0;
    virtual bool attemptHit(BattleField &field, MoveObject &move,
            Pokemon &user, Pokemon &target) const = 0;
    virtual int getRandomInt(const int lower, const int upper) const = 0;
    virtual double getEffectiveness(BattleField &field,
            const PokemonType *, Pokemon *, Pokemon *,
            std::vector<double> *) const = 0;
    virtual bool isCriticalHit(BattleField &field, MoveObject &move,
            Pokemon &user, Pokemon &target) const = 0;
    virtual ~BattleMechanics() { }
protected:
    BattleMechanics() { }
private:
    BattleMechanics(const BattleMechanics &);
    BattleMechanics &operator=(const BattleMechanics &);
};

}

#endif

