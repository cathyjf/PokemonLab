/*
 * File:   statuses.js
 * Author: Catherine
 *
 * Created on April 3 2009, 8:39 PM
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
 * Paralysis
 *
 */
function ParalysisEffect() { }
ParalysisEffect.prototype = new StatusEffect("ParalysisEffect");
ParalysisEffect.prototype.lock = StatusEffect.SPECIAL_EFFECT;
ParalysisEffect.prototype.name = Text.status_effects_paralysis(0);
ParalysisEffect.prototype.applyEffect = function() {
    field.print(Text.status_effects_paralysis(1, this.subject));
    return true;
};
ParalysisEffect.prototype.vetoExecution = function(field, user, target, move) {
    if (user != this.subject)
        return false;
    if (target != null)
        return false;
    if (field.random(0.75))
        return false;
    field.print(Text.status_effects_paralysis(2));
    return true;
};
// TODO: speed drop


/**
 * StatChange
 *
 * Changes a stat level on one pokemon.
 */
function StatChangeEffect(stat, delta) {
    this.applyEffect = function() {
        this.stat = stat;
        var present = this.subject.getStatLevel(stat);
        var result = present + delta;
        if (result < -6) {
            result = -6;
        } else if (result > 6) {
            result = 6;
        }
        this.delta = result - present;

        var message = 0;
        if (this.delta <= -2) {
            message = 3;
        } else if (this.delta == -1) {
            message = 2;
        } else if (this.delta == 1) {
            message = 0;
        } else if (this.delta >= 2) {
            message = 1;
        } else if (delta < 0) {
            message = 4;
        } else {
            message = 5;
        }

        this.subject.field.print(Text.status_effects_stat_level(message,
                this.subject, Text.stats_long(stat)));

        if (this.delta == 0)
            return false;

        this.subject.setStatLevel(stat, result);
        return true;
    }
}
StatChangeEffect.prototype = new StatusEffect("StatChangeEffect");
StatChangeEffect.prototype.singleton = false;
StatChangeEffect.prototype.name = "StatChangeEffect";
StatChangeEffect.prototype.informFailure = function() { };
StatChangeEffect.prototype.unapplyEffect = function() {
    var present = this.subject.getStatLevel(this.stat);
    this.subject.setStatLevel(this.stat, present + this.delta);
}


/**
 * Sleep
 *
 * For 1-4 turns, the afflicted pokemon is prevented from using a move other
 * than Sleep Talk or Snore. If the afflicted pokemon has the Early Bird
 * ability than each turn deducts two turns from the counter rather than
 * one.
 */
function SleepEffect() { }
SleepEffect.prototype = new StatusEffect("SleepEffect");
SleepEffect.prototype.lock = StatusEffect.SPECIAL_EFFECT;
SleepEffect.prototype.name = Text.status_effects_sleep(0);
SleepEffect.prototype.switchOut = function() { return false; };
SleepEffect.prototype.applyEffect = function() {
    var field = this.subject.field;
    this.turns = field.random(1, 4); // random duration
    field.print(Text.status_effects_sleep(1, this.subject));
    return true;
};
SleepEffect.prototype.vetoExecution = function(field, user, target, move) {
    if (user != this.subject)
        return false;
    // Only handle the "overall veto", not target specific.
    if (target != null)
        return false;
    if (this.turns <= 0) {
        user.removeStatus(this);
        field.print(Text.status_effects_sleep(2, user));
        return false;
    }
    --this.turns;
    if (user.hasAbility("Early Bird")) { // this is really a part of Sleep
        --this.turns;
    }

    // Sleep Talk and Snore are not vetoed by Sleep.
    var name = move.name;
    if ((name == "Sleep Talk") || (name == "Snore"))
        return false;

    field.print(Text.status_effects_sleep(3, user))
    return true;
};


