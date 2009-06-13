/* 
 * File:   JewelMechanics.cpp
 * Author: Catherine
 *
 * Created on March 28, 2009, 12:36 AM
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

#include <boost/random.hpp>

#include "JewelMechanics.h"
#include "PokemonNature.h"
#include "PokemonType.h"
#include "../shoddybattle/Pokemon.h"
#include "../shoddybattle/BattleField.h"
#include "../moves/PokemonMove.h"
#include "../scripting/ScriptMachine.h"

#include <ctime>

using namespace std;
using namespace boost;

namespace shoddybattle {

/**
 * Table of chances to get a critical hit.
 */
static const double CRITICAL_TABLE[] = { 0.0625, 0.125, 0.25, 0.375, 0.5 };

typedef mt11213b GENERATOR;

struct JewelMechanicsImpl {
    GENERATOR rand;
};

JewelMechanics::JewelMechanics() {
    m_impl = new JewelMechanicsImpl();
    m_impl->rand = GENERATOR(time(NULL));
}

JewelMechanics::~JewelMechanics() {
    delete m_impl;
}

/**
 * Calculate a pokemon stat using the Pokemon DPP formula.
 */
unsigned int JewelMechanics::calculateStat(
        const Pokemon &p, const STAT i) const {
    unsigned int base = p.getBaseStat(i);
    unsigned int common =
            (int)((int)(((2.0 * base)
            + p.getIv(i)
            + (p.getEv(i) / 4.0)))
            * (p.getLevel() / 100.0));
    if (i == S_HP) {
        if (base == 1) {    // base 1 hp => 1 hp total
            return 1;
        } else {
            return common + 10 + p.getLevel();
        }
    }
    return (int)((common + 5) * p.getNature()->getEffect(i));
}

bool JewelMechanics::getCoinFlip(double p) const {
    bernoulli_distribution<> dist(p);
    variate_generator<GENERATOR &, bernoulli_distribution<> >
            coin(m_impl->rand, dist);
    return coin();
}

template <class T>
inline void multiplyBy(T &value, PRIORITY_MAP &elements) {
    PRIORITY_MAP::const_iterator i = elements.begin();
    for (; i != elements.end(); ++i) {
        double val = i->second;
        value *= val;
    }
}

bool JewelMechanics::attemptHit(BattleField &field, MoveObject &move,
        Pokemon &user, Pokemon &target) const {
    ScriptContext *cx = field.getContext();
    int accuracy = move.getAccuracy(cx) * 100;

    int level0 = user.getStatLevel(S_ACCURACY);
    if (!user.getTransformedStatLevel(&user, &target, S_ACCURACY, &level0)) {
        target.getTransformedStatLevel(&user, &target, S_ACCURACY, &level0);
    }

    int level1 = user.getStatLevel(S_EVASION);
    if (!user.getTransformedStatLevel(&user, &target, S_EVASION, &level1)) {
        target.getTransformedStatLevel(&user, &target, S_EVASION, &level1);
    }

    const int level = level0 - level1;

    PRIORITY_MAP mods;
    field.getStatModifiers(S_ACCURACY, user, mods);
    mods[0] = getStatMultiplier(S_ACCURACY, level);

    // Calculate effective accuracy.
    multiplyBy(accuracy, mods);

    if (accuracy == 0) {
        return true;
    }

    return ((getRandomInt(0, 99) + 1) <= accuracy);
}

bool JewelMechanics::isCriticalHit(BattleField &field, MoveObject &move,
        Pokemon &user, Pokemon &target) const {
    int term = 0;
    ScriptContext *cx = field.getContext();
    if (move.getFlag(cx, F_NO_CRITICAL)) {
        return false;
    }
    if (move.getFlag(cx, F_HIGH_CRITICAL)) {
        term += 1;
    }
    term += user.getCriticalModifier();
    if (term > 4) {
        term = 4;
    }
    bernoulli_distribution<> dist(CRITICAL_TABLE[term]);
    variate_generator<GENERATOR &, bernoulli_distribution<> >
            generator(m_impl->rand, dist);
    return generator();
}

inline int multiplyOut(PRIORITY_MAP &elements) {
    PRIORITY_MAP::const_iterator i = elements.begin();
    double value = i->second;
    ++i;
    for (; i != elements.end(); ++i) {
        value = floor(value * i->second);
    }
    return int(value);
}

inline void multiplyBy(int &damage, const int position, MODIFIERS &mods) {
    multiplyBy(damage, mods[position]);
}

int JewelMechanics::getRandomInt(const int lower, const int upper) const {
    boost::uniform_int<> range(lower, upper);
    variate_generator<GENERATOR &, uniform_int<> > r(m_impl->rand, range);
    return r();
}

/**
 * Calculate damage.
 */
int JewelMechanics::calculateDamage(BattleField &field, MoveObject &move,
        Pokemon &user, Pokemon &target, const int targets,
        const bool weight) const {
    
    ScriptContext *cx = field.getContext();

    MOVE_CLASS cls = move.getMoveClass(cx);
    assert(cls != MC_OTHER);

    STAT stat0 = (cls == MC_PHYSICAL) ? S_ATTACK : S_SPATTACK;
    STAT stat1 = (cls == MC_PHYSICAL) ? S_DEFENCE : S_SPDEFENCE;

    const bool critical = isCriticalHit(field, move, user, target);
    MODIFIERS mods;
    field.getModifiers(user, target, move, critical, targets, mods);

    if (targets > 1) {
        mods[1][2] = 0.75;
    }

    PRIORITY_MAP statMod;

    int attack = user.getRawStat(stat0);
    field.getStatModifiers(stat0, user, statMod);
    int level = user.getStatLevel(stat0);
    if (!user.getTransformedStatLevel(&user, &target, stat0, &level)) {
        target.getTransformedStatLevel(&user, &target, stat0, &level);
    }
    if (critical && (level < 0)) {
        level = 0;
    }
    statMod[0] = getStatMultiplier(stat0, level);
    multiplyBy(attack, statMod);

    statMod.clear();
    int defence = target.getRawStat(stat1);
    field.getStatModifiers(stat1, target, statMod);
    level = target.getStatLevel(stat1);
    if (!user.getTransformedStatLevel(&user, &target, stat1, &level)) {
        target.getTransformedStatLevel(&user, &target, stat1, &level);
    }
    if (critical && (level > 0)) {
        level = 0;
    }
    statMod[0] = getStatMultiplier(stat1, level);
    multiplyBy(defence, statMod);

    mods[0][0] = move.getPower(cx); // base power
    const int power = multiplyOut(mods[0]);
    
    int damage = user.getLevel() * 2 / 5;
    damage += 2;
    damage *= power;
    damage *= attack;
    damage /= 50;
    damage /= defence;
    multiplyBy(damage, 1, mods); // "Mod1" in X-Act's essay
    damage += 2;
    if (critical) {
        int factor = 2;
        ScriptValue v = user.sendMessage("informCritical", 0, NULL);
        if (!v.failed()) {
            factor = v.getInt();
        }
        damage *= factor;
    }
    multiplyBy(damage, 2, mods); // "Mod2"

    if (weight) {
        damage *= getRandomInt(217, 255) * 100;
        damage /= 255;
        damage /= 100;
    }

    const PokemonType *moveType = move.getType(cx);
    if (user.isType(moveType)) {
        ScriptValue v = user.sendMessage("informStab", 0, NULL);
        double stab = 1.5;
        if (!v.failed()) {
            stab = v.getDouble(cx);
        }
        damage *= stab;
    }

    double effectiveness = 1.0;
    const TYPE_ARRAY &types = target.getTypes();
    for (TYPE_ARRAY::const_iterator i = types.begin(); i != types.end(); ++i) {
        const double factor = moveType->getMultiplier(**i);
        damage *= factor;
        effectiveness *= factor;
    }

    if (effectiveness == 0.0) {
        vector<string> args;
        args.push_back(target.getToken());
        field.print(TextMessage(4, 1, args));
        return 0;
    }

    if (critical) {
        field.print(TextMessage(4, 9));
        target.sendMessage("informCriticalHit", 0, NULL);
    }

    if (effectiveness < 1.0) {
        field.print(TextMessage(4, 8));
    } else if (effectiveness > 1.0) {
        field.print(TextMessage(4, 7));
    }

    multiplyBy(damage, 3, mods); // "Mod3"

    if (damage < 1) damage = 1;
    return damage;
}

}

