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
 * Damage listener.
 *
 * Remembers the last attack to hit the subject from an enemy pokemon.
 * Vanishes at end of turn.
 */
function DamageListener() { }
DamageListener.prototype = new StatusEffect("DamageListener");
DamageListener.prototype.applyEffect = function() {
    this.party = -1;
    this.position = -1;
    this.move = null;
    this.damage = 0;
};
DamageListener.prototype.predicate = function() {
    return true;
};
DamageListener.prototype.informDamaged = function(user, move, damage) {
    // Pain Split does not count for the purpose of DamageListener.
    if (this.predicate(move) && (move.name != "Pain Split")) {
        this.party = user.party;
        this.position = user.position;
        this.move = move;
        this.damage = damage;
    }
};
DamageListener.prototype.tick = function() {
    this.subject.removeStatus(this);
};

/**
 * Paralysis
 *
 */
function ParalysisEffect() { }
ParalysisEffect.prototype = new StatusEffect("ParalysisEffect");
ParalysisEffect.prototype.lock = StatusEffect.SPECIAL_EFFECT;
ParalysisEffect.prototype.name = Text.status_effects_paralysis(0);
ParalysisEffect.prototype.applyEffect = function() {
    field.print(Text.status_effects_paralysis(1, this.subject.name));
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
    field.print(Text.status_effects_sleep(1, this.subject.name));
    return true;
};
SleepEffect.prototype.vetoExecution = function(field, user, target, move) {
    if (user != this.subject)
        return false;
    // Only handle the "overall veto", not target specific.
    if (target != null)
        return false;
    --this.turns;
    if (user.hasAbility("Early Bird")) { // this is really a part of Sleep
        --this.turns;
    }
    if (this.turns <= 0) {
        user.removeStatus(this);
        field.print(Text.status_effects_sleep(2, user.name));
        return false;
    }

    // Sleep Talk and Snore are not vetoed by Sleep.
    var name = move.name;
    if ((name == "Sleep Talk") || (name == "Snore"))
        return false;

    field.print(Text.status_effects_sleep(3, user.name))
    return true;
};


