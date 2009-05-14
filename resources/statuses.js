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

function makeEffect(obj) {
    if (obj.ctor == undefined) {
        obj.ctor = function() { };
    }
    var ctor = obj.ctor;
    ctor.prototype = new StatusEffect(obj.id);
    for (var i in obj) {
        ctor.prototype[i] = obj[i];
    }
    this[obj.id] = ctor;
}

/**
 * Confusion
 */
makeEffect({
    id : "ConfusionEffect",
    name : Text.status_effects_confusion(0),
    applyEffect : function() {
        var field = this.subject.field;
        field.print(Text.status_effects_confusion(1, this.subject));
        this.turns = field.random(2, 5);
        return true;
    },
    vetoExecution : function(field, user, target, move) {
        if (user != this.subject)
            return false;
        if (target != null)
            return false;
        field.print(Text.status_effects_confusion(3, user));
        if (--this.turns <= 0) {
            field.print(Text.status_effects_confusion(2, user));
            user.removeStatus(this);
            return false;
        }
        if (field.random(0.5)) {
            return false;
        }
        field.print(Text.status_effects_confusion(4, user));
        var confuse = field.getMove("__confusion");
        var damage = field.calculate(confuse, user, user, 1);
        user.hp -= damage;
        return true;
    },
    informFailure : function(subject) {
        subject.field.print(Text.status_effects_confusion(5, subject));
    }
});


/**
 * Poison
 */
makeEffect({
    id : "PoisonEffect",
    lock : StatusEffect.SPECIAL_EFFECT,
    name : Text.status_effects_poison(0),
    tier : 6,
    subtier : 4,
    switchOut : function() { return false; },
    applyEffect : function() {
        this.subject.field.print(Text.status_effects_poison(1, this.subject));
        return true;
    },
    tick : function() {
        if (this.subject.sendMessage("informPoisonDamage"))
            return;

        var damage = Math.floor(this.subject.getStat(Stat.HP) / 8);
        if (damage < 1) damage = 1;
        this.subject.field.print(Text.status_effects_poison(3, this.subject));
        this.subject.hp -= damage;
    }
});


/**
 * Paralysis
 *
 */
makeEffect({
    id : "ParalysisEffect",
    lock : StatusEffect.SPECIAL_EFFECT,
    name : Text.status_effects_paralysis(0),
    switchOut : function() { return false; },
    applyEffect : function() {
        var field = this.subject.field;
        field.print(Text.status_effects_paralysis(1, this.subject));
        return true;
    },
    vetoExecution : function(field, user, target, move) {
        if (user != this.subject)
            return false;
        if (target != null)
            return false;
        if (field.random(0.75))
            return false;
        field.print(Text.status_effects_paralysis(2, user));
        return true;
    },
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.SPEED)
            return null;
        if (this.subject.sendMessage("informParalysisMod"))
            return null;
        // reduces speed to 25%, priority of 3
        return [0.25, 3];
    }
});


/**
 * StatChange
 *
 * Changes a stat level on one pokemon.
 */
makeEffect({
    id : "StatChangeEffect",
    ctor : function(stat, delta) {
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
    },
    singleton : false,
    name : "StatChangeEffect",
    informFailure : function() { },
    unapplyEffect : function() {
        var present = this.subject.getStatLevel(this.stat);
        this.subject.setStatLevel(this.stat, present - this.delta);
    }
});


/**
 * Sleep
 *
 * For 1-4 turns, the afflicted pokemon is prevented from using a move other
 * than Sleep Talk or Snore. If the afflicted pokemon has the Early Bird
 * ability than each turn deducts two turns from the counter rather than
 * one.
 */
makeEffect({
    id : "SleepEffect",
    lock : StatusEffect.SPECIAL_EFFECT,
    name : Text.status_effects_sleep(0),
    switchOut : function() { return false; },
    applyEffect : function() {
        var field = this.subject.field;
        this.turns = field.random(1, 4); // random duration
        field.print(Text.status_effects_sleep(1, this.subject));
        return true;
    },
    vetoExecution : function(field, user, target, move) {
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
    }
});


