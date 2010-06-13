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
 * Make a new effect object. The first parameter is the parent effect and the
 * second parameter is the object whose properties will be added to a new
 * instance of the parent.
 */
function makeEffect(f, obj) {
    if (obj.ctor == undefined) {
        obj.ctor = function() { };
    }
    var ctor = obj.ctor;
    ctor.prototype = new f(obj.id);
    for (var i in obj) {
        ctor.prototype[i] = obj[i];
    }
    this[obj.id] = ctor;
}

/**
 * Freeze
 *
 * The subject is prevented from using moves other than Sacred Fire,
 * Flame Wheel, and Flare Blitz, all of which also cure the status when used.
 * Additionally, each turn there is a 20% chance for the status to end.
 * Moreover, if the subject is damaged by a Fire-type move, the status ends.
 *
 * Ice-type pokemon are immune to freeze.
 */
makeEffect(StatusEffect, {
    id : "FreezeEffect",
    lock : StatusEffect.SPECIAL_EFFECT,
    name : Text.status_effects_freeze(0),
    vetoTier : 0,
    applyEffect : function() {
        if (this.subject.isType(Type.ICE)
                || this.subject.getStatus("SunEffect")) {
            return false;
        }
        var field = this.subject.field;
        field.print(Text.status_effects_freeze(1, this.subject));
        this.subject.sendMessage("informFreeze");
        return true;
    },
    vetoExecution : function(field, user, target, move) {
        if (user != this.subject)
            return false;
        if ((target != null) || field.random(0.2)) {
            user.removeStatus(this);
            return false;
        }
        var name = move.name;
        if ((name == "Sacred Fire")
                || (name == "Flame Wheel")
                || (name == "Flare Blitz")) {
            return false;
        }
        field.print(Text.status_effects_freeze(3, user));
        return true;
    },
    informDamaged : function(user, move, damage) {
        if (move.type != Type.FIRE)
            return;
        if (damage <= 0)
            return;
        this.subject.removeStatus(this);
    },
    informBeginTurn : function() {
        return true;
    },
    unapplyEffect : function() {
        this.subject.field.print(Text.status_effects_freeze(2, this.subject));
    }
});


/**
 * Flinch
 *
 * The subject's next move to be executed is vetoed. The status vanishes at
 * the end of the turn on which it was applied.
 */
makeEffect(StatusEffect, {
    id : "FlinchEffect",
    name : Text.status_effects_flinch(0),
    tier : 0,
    vetoTier : 6,
    vetoExecution : function(field, user, target, move) {
        if (user != this.subject)
            return false;
        if (target != null)
            return false;
        if (user.sendMessage("informMaybeFlinch"))
            return false;
        field.print(Text.status_effects_flinch(1, user));
        user.sendMessage("informFlinched");
        return true;
    },
    tick : function() {
        this.subject.removeStatus(this);
    }
});


/**
 * Infatuation (Attract)
 *
 * The afflicted pokemon has a 50% chance of failing to attack on its turn.
 * The effect ends when the inducer leaves the field.
 */
makeEffect(StatusEffect, {
    id : "AttractEffect",
    name : Text.status_effects_attract(0),
    vetoTier : 9,
    applyEffect : function() {
        var field = this.subject.field;
        field.print(Text.status_effects_attract(3, this.subject, this.inducer));
        if (this.subject.sendMessage("informAttracted", this.inducer)) {
            return false;
        }
        return true;
    },
    informWithdraw : function(subject) {
        if (subject == this.inducer) {
            this.subject.removeStatus(this);
        }
    },
    vetoExecution : function(field, user, target, move) {
        if (target != null)
            return false;
        if (user != this.subject)
            return false;
        field.print(Text.status_effects_attract(1, this.subject, this.inducer));
        if (field.random(0.5))
            return false;
        field.print(Text.status_effects_attract(2, this.subject));
        return true;
    }
});


/**
 * Confusion
 *
 * For 2-5 turns, the afflicted pokemon has a 50% chance of attacking itself
 * with a 40 base power typeless attack which never criticals and never
 * misses. The turn counter decrements whenever the message "X is confused!"
 * is displayed. If the counter is zero after the decrement then the pokemon
 * snaps out of confusion; hence, a pokemon has at most four chances to
 * attack itself in confusion.
 */
makeEffect(StatusEffect, {
    id : "ConfusionEffect",
    name : Text.status_effects_confusion(0),
    vetoTier : 5,
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
        if (--this.turns <= 0) {
            field.print(Text.status_effects_confusion(2, user));
            user.removeStatus(this);
            return false;
        }
        field.print(Text.status_effects_confusion(3, user));
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
 * Burn
 *
 * The afflicted pokemon is hurt 1/8 of its max hp at the end of each turn,
 * and Mod1 takes on a 50% modifier when the subject uses a physical attack.
 *
 * Fire-type pokemon are inherently immune to Burn.
 */
makeEffect(StatusEffect, {
    id : "BurnEffect",
    lock : StatusEffect.SPECIAL_EFFECT,
    name : Text.status_effects_burn(0),
    tier : 6,
    subtier : 4,
    switchOut : function() { return false; },
    applyEffect : function() {
        if (this.subject.isType(Type.FIRE)) {
            return false;
        }
        this.subject.field.print(Text.status_effects_burn(1, this.subject));
        return true;
    },
    tick : function() {
        if (this.subject.sendMessage("informBurnDamage"))
            return;

        var damage = Math.floor(this.subject.getStat(Stat.HP) / 8);
        if (damage < 1) damage = 1;
        this.subject.field.print(Text.status_effects_burn(2, this.subject));
        this.subject.hp -= damage;
    },
    modifier : function(field, user, target, move, critical) {
        if (user != this.subject)
            return null;
        if (move.moveClass != MoveClass.PHYSICAL)
            return null;
        if (user.sendMessage("informBurnMod"))
            return null;
        // 50% modifier in Mod1; first position
        return [1, 0.5, 0];
    }
});


/**
 * Poison
 *
 * The afflicted pokemon is hurt 1/8 of its max hp at the end of each turn.
 * Poison- and Steel-type pokemon are immune to poison.
 */
makeEffect(StatusEffect, {
    id : "PoisonEffect",
    lock : StatusEffect.SPECIAL_EFFECT,
    name : Text.status_effects_poison(0),
    tier : 6,
    subtier : 4,
    switchOut : function() { return false; },
    applyEffect : function() {
        if (this.subject.isType(Type.POISON)
                || this.subject.isType(Type.STEEL)) {
            return false;
        }
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
 * Toxic (bad poisoning)
 *
 * Each turn the afflicted pokemon loses x/16 of its max HP, where x is the
 * number of turns since the status was applied, including the turn it was
 * applied; or, the number of turns since the pokemon became active, if the
 * pokemon was already badly poisoned when it was sent out.
 *
 * The variable x does not increase beyond 15.
 *
 * Poison- and Steel-type pokemon are immune to bad poison.
 */
makeEffect(StatusEffect, {
    id : "ToxicEffect",
    lock : StatusEffect.SPECIAL_EFFECT,
    name : Text.status_effects_poison(0),
    tier : 6,
    subtier : 4,
    turns_ : 0,
    switchOut : function() { return false; },
    applyEffect : function() {
        if (this.subject.isType(Type.POISON)
                || this.subject.isType(Type.STEEL)) {
            return false;
        }
        this.subject.field.print(Text.status_effects_poison(2, this.subject));
        return true;
    },
    switchIn : function() {
        this.turns_ = 0;
    },
    tick : function() {
        if (this.turns_ < 15) {
            ++this.turns_;
        }

        if (this.subject.sendMessage("informPoisonDamage"))
            return;

        var hp = Math.floor(this.subject.getStat(Stat.HP) * this.turns_ / 16);
        if (hp < 1) hp = 1;
        this.subject.field.print(Text.status_effects_poison(3, this.subject));
        this.subject.hp -= hp;
    }
});


/**
 * Paralysis
 *
 * The afflicted pokemon has a 25% chance of failing to act on any given turn.
 * Additionally, the subject's speed is reduced to 25% of its original value.
 */
makeEffect(StatusEffect, {
    id : "ParalysisEffect",
    lock : StatusEffect.SPECIAL_EFFECT,
    name : Text.status_effects_paralysis(0),
    vetoTier : 10,
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
makeEffect(StatusEffect, {
    id : "StatChangeEffect",
    ctor : function(stat, delta) {
        this.stat_ = stat;
        this.delta_ = delta;
        this.applyEffect = function() {
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

            if (!this.silent) {
                this.subject.field.print(Text.status_effects_stat_level(message,
                    this.subject, Text.stats_long(stat)));
            }

            if (this.delta == 0)
                return false;

            this.subject.setStatLevel(stat, result);
            return true;
        }
    },
    singleton : false,
    name : "StatChangeEffect",
    toString : function() {
        return this.name + ";" + this.stat_ + ";" + this.delta_;
    },
    informFailure : function() { },
    unapplyEffect : function() {
        var present = this.subject.getStatLevel(this.stat_);
        this.subject.setStatLevel(this.stat_, present - this.delta_);
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
makeEffect(StatusEffect, {
    id : "SleepEffect",
    lock : StatusEffect.SPECIAL_EFFECT,
    name : Text.status_effects_sleep(0),
    tier : 6,
    subtier : 12, // TODO: Research to get the correct tier/subtier.
    vetoTier : 0,
    switchOut : function() { return false; },
    applyEffect : function() {
        var field = this.subject.field;
        this.turns = field.random(1, 4); // random duration
        if (this.text_) {
            field.print(this.text_(this.subject));
        } else {
            field.print(Text.status_effects_sleep(1, this.subject));
        }
        this.subject.sendMessage("informSleep");
        return true;
    },
    unapplyEffect : function() {
        this.subject.sendMessage("informWakeUp");
    },
    informBeginTurn : function() {
        return true;
    },
    wakeUp : function() {
        this.subject.removeStatus(this);
        this.subject.field.print(Text.status_effects_sleep(2, this.subject));
    },
    tick : function() {
        if (!this.subject.sendMessage("informSleepCheck", this)) {
            this.subject.field.sendMessage("informSleepTick", this);
        }
    },
    vetoExecution : function(field, user, target, move) {
        if (user != this.subject)
            return false;
        // Only handle the "overall veto", not target specific.
        if (target != null)
            return false;
        if (this.subject.sendMessage("informSleepCheck", this))
            return false;
        if (this.turns <= 0) {
            this.wakeUp();
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


