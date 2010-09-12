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
#include "stat.h"

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
            (int)((int)(((2 * base)
            + p.getIv(i)
            + (p.getEv(i) / 4)))
            * p.getLevel() / 100);
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
    int accuracy = round(move.getAccuracy(cx) * 100);

    int level0 = user.getStatLevel(S_ACCURACY);
    if (!user.getTransformedStatLevel(&user, &target, S_ACCURACY, &level0)) {
        target.getTransformedStatLevel(&user, &target, S_ACCURACY, &level0);
    }

    int level1 = target.getStatLevel(S_EVASION);
    if (!user.getTransformedStatLevel(&user, &target, S_EVASION, &level1)) {
        target.getTransformedStatLevel(&user, &target, S_EVASION, &level1);
    }

    int level = level0 - level1;
    if (level < -6) {
        level = -6;
    } else if (level > 6) {
        level = 6;
    }

    PRIORITY_MAP mods;
    field.getStatModifiers(S_ACCURACY, &user, &target, mods);
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
    ScriptContext *cx = field.getContext();
    ScriptValue v = target.sendMessage("informAttemptCritical", 0, NULL);
    if (!v.failed() && v.getBool()) {
        return false;
    }
    if (move.getFlag(cx, F_NO_CRITICAL)) {
        return false;
    }
    int term = 0;
    if (move.getFlag(cx, F_HIGH_CRITICAL)) {
        term += 1;
    }
    term += user.getCriticalModifier();
    if (term > 4) {
        term = 4;
    }
    return getCoinFlip(CRITICAL_TABLE[term]);
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

double JewelMechanics::getEffectiveness(BattleField &field,
        const PokemonType *type,
        Pokemon *user, Pokemon *target, vector<double> *factors) const {
    set<const PokemonType *> immunities, vulnerabilities;
    field.getImmunities(user, target, immunities, vulnerabilities);
    if (immunities.find(type) != immunities.end()) {
        return 0.0;
    }
    const bool immune = (vulnerabilities.find(type) == vulnerabilities.end());
    double effectiveness = 1.0;
    const TYPE_ARRAY &types = target->getTypes();
    
    for (TYPE_ARRAY::const_iterator i = types.begin(); i != types.end(); ++i) {
        double factor;
        if (!field.getTransformedEffectiveness(type, *i, target, factor)) {
            factor = type->getMultiplier(**i);
        }
        
        if ((factor != 0.0) || immune) {
            effectiveness *= factor;
            if (factors) {
                factors->push_back(factor);
            }
        }
    }
    return effectiveness;
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
    field.getStatModifiers(stat0, &user, NULL, statMod);
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
    field.getStatModifiers(stat1, &target, NULL, statMod);
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

    vector<double> factors;
    const double effectiveness =
            getEffectiveness(field, moveType, &user, &target, &factors);

    if (effectiveness == 0.0) {
        vector<string> args;
        args.push_back(target.getToken());
        field.print(TextMessage(4, 1, args));
        return 0;
    }

    vector<double>::const_iterator i = factors.begin();
    for (; i != factors.end(); ++i) {
        damage *= *i;
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

/*inline unsigned short prev(unsigned long long &n) {
    n = (n * 4005161829L) + 171270561L;
    return static_cast<short>(n >> 16);
}

inline unsigned short next(unsigned long long &n) {
    n  = (n * 1103515245) + 24691;
    return static_cast<short>(n >> 16);
}*/

inline bool validateIvs(const Pokemon &p) {
    /*unsigned long long rand = 0;
    const unsigned int internal = p.getNature()->getInternalValue();
    const unsigned long long num1 = (p.getIv(S_DEFENCE) << 10)
            + (p.getIv(S_ATTACK) << 5)
            + p.getIv(S_HP);
    const unsigned long long num2 = (p.getIv(S_SPDEFENCE) << 10)
            + (p.getIv(S_SPATTACK) << 5)
            + p.getIv(S_SPEED);
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 65536; ++j) {
            rand = (i << 31) + (num1 << 16) + j;
            unsigned long long first = rand;
            unsigned short second = next(rand);
            if (second >= 32768) {
                second -= 32768;
            }
            if (second == num2) {
                rand = first;
                unsigned short a = prev(rand);
                unsigned short b = prev(rand);
                unsigned short c = prev(rand);
                unsigned long long p1 = (a << 16) + b;
                unsigned long long p2 = (b << 16) + c;
                if ((p1 % 25) == internal) {
                    return true;
                }
                if ((p2 % 25) == internal) {
                    return true;
                }
            }
        }
    }
    return false;*/
    return true;
}

/**
 * Validates a Pokemon using the following criteria:
 * 1) Level must be in the range [1,100]
 * 2) IVs must be in the range [0,31]
 * 3) EVs must be in the range [0,255]
 * 4) Sum of all EVs must be <= 510
 * 5) Specific natures and IV combos are not allowed
 *    for non-breeding species
 */
bool JewelMechanics::validateHiddenStats(const Pokemon &p) const {
    int level = p.getLevel();
    if ((level < 1) || (level > 100))
        return false;

    int sum = 0;
    for (int i = 0; i < STAT_COUNT; ++i) {
        STAT s = static_cast<STAT>(i);
        const int iv = p.getIv(s);
        const int ev = p.getEv(s);
        sum += ev;
        if ((iv < 0) || (iv > MAX_IV))
            return false;
        if ((ev < 0) || (ev > MAX_EV))
            return false;
    }

    if (sum > MAX_EV_TOTAL)
        return false;

    if (p.hasRestrictedIvs() && !validateIvs(p))
        return false;

    return true;
}

}

