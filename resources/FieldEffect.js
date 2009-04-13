/*
 * File:   FieldEffect.js
 * Author: Catherine
 *
 * Created on April 13 2009, 2:30 AM
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
 * A status effect that is always present on every pokemon, but whose
 * state varies. One might wonder why this is only one effect, as opposed
 * to the collection of effects it was in Shoddy Battle 1; the answer is
 * that the effect of any field effect actually depends on what other field
 * effects, if any, are present.
 *
 * Specifically, if the "host" of the battle kills a switching pokemon with
 * Pursuit in Platinum then all effects with a greater value than currently
 * greatest set one are all set to be on, with their turns remaining at
 * zero (indefinite duration).
 *
 * These mechanics are of central importance to the Platinum metagame and
 * as such I have considered them carefully when deciding how to implement
 * the FieldEffect. This code appears to be a mess, but it is carefully
 * written to simulate exactly the in game mechanics, and that is the whole
 * point of a simulator.
 *
 */

function FieldEffect() {
}

FieldEffect.TRICK_ROOM = 0;
FieldEffect.FOG = 1;
FieldEffect.GRAVITY = 2;
FieldEffect.UPROAR = 3;
FieldEffect.HAIL = 4;
FieldEffect.SUNNY_DAY = 5;
FieldEffect.SANDSTORM = 6;
FieldEffect.RAIN = 7;

FieldEffect.prototype = new StatusEffect();

/**
 * A flag for whether each type of effect is enabled.
 */
FieldEffect.prototype.flags = [false, false, false, false,
    false, false, false, false];

/**
 * Number of turns remaining on each effect. Zero denotes the effect will
 * continue indefinitely.
 */
FieldEffect.prototype.turns = [0, 0, 0, 0, 0, 0, 0, 0];

/**
 * Note: This appears to be very buggy, but it is actually correctly simulating
 * the game mechanics.
 */
FieldEffect.prototype.tickWeather = function(subject, i) {
    if (i == FieldEffect.RAIN) {
        if (subject.hasAbility("Rain Dish")) {
            // heal here

            // we are done ticking this weather
            return;
        }
    }
    if (this.flags[FieldEffect.RAIN]) {
        if (subject.hasAbility("Dry Skin")) {
            if (subject.hp != subject.stat[S_HP]) {
                // heal

                // done ticking
                return;
            }
        }
    }
    if (this.flags[FieldEffect.SANDSTORM] ||
        this.flags[FieldEffect.HAIL] ||
        (this.flags[FieldEffect.SUN] &&
            (subject.hasAbility("Dry Skin") ||
                subject.hasAbility("Solarpower")))) {
        // The pokemon needs to be damaged -- but what kind of damage?

        if (this.flags[FieldEffect.SUN]) {
            // damaged by ability
        } else if (this.flags[FieldEffect.SANDSTORM]) {
            // damaged by sand
        } else {
            // damaged by hail
        }
    }
}

FieldEffect.prototype.tick = function(subject) {
    for (var i = 0; i < 8; ++i) {
        if (this.flags[i]) {
            if (i > FieldEffect.UPROAR) {
                // weather
                this.tickWeather(subject, i);
            } else if (i == FieldEffect.TRICK_ROOM) {

            } else if (i == FieldEffect.FOG) {

            } else if (i == FieldEffect.GRAVITY) {

            } else if (i == FieldEffect.UPROAR) {

            }

            if (this.turns[i] != 0) {
                if (--this.turns[i] == 0) {
                    // unapply the effect.
                    this.flags[i] = false;
                }
            }
        }
    }
}

