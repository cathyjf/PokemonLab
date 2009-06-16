/*
 * File:   abilities.js
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

/**
 * Ability function
 *
 * Constructs a new ability and makes it a property of the Ability function.
 */
function Ability(name) {
    this.name = this.id = name;
    Ability[name] = this;
}

Ability.prototype = new StatusEffect();

/**
 * Allows for a nicer syntax for making an ability.
 */
function makeAbility(obj) {
    var ability = new Ability(obj.name);
    for (var p in obj) {
        ability[p] = obj[p];
    }
}

function makeStatusImmuneAbility(ability, immune) {
    makeAbility({
        name : ability,
        transformStatus : function(subject, status) {
            if (subject != this.subject)
                return status;
            if (status.id == immune)
                return null;
            return status;
        },
        tier : 0, // TODO
        tick : function() {
            var effect = this.subject.getStatus(immune);
            if (effect) {
                this.subject.removeStatus(effect);
                // todo: message
            }
        }
    });
}

/*******************
 * Levitate
 *******************/
makeAbility({
    name : "Levitate",
    vetoExecution : function(field, user, target, move) {
        if (target != this.subject)
            return false;
        if (move.type != Type.GROUND)
            return false;

        field.print(Text.ability_messages(25, target));
        return true;
    }
});

/*******************
 * Insomnia
 *******************/
makeStatusImmuneAbility("Insomnia", "SleepEffect");

/*******************
 * Vital Spirit
 *******************/
makeStatusImmuneAbility("Vital Spirit", "SleepEffect");

/*******************
 * Early Bird
 *******************/
makeAbility({
    name : "Early Bird"
});

/*******************
 * Stall
 *******************/
makeAbility({
    name : "Stall",
    inherentPriority : function() {
        return -1;
    }
});

/*******************
 * Poison Heal
 *******************/
makeAbility({
    name : "Poison Heal",
    informPoisonDamage : function() {
        var max = this.subject.getStat(Stat.HP);
        if (max == this.subject.hp) {
            // no need to heal
            return true;
        }
        this.subject.field.print(Text.ability_messages(32, this.subject));
        var damage = Math.floor(max / 8);
        this.subject.hp += damage;
        return true;
    }
});

/*******************
 * Quick Feet
 *******************/
makeAbility({
    name : "Quick Feet",
    informParalysisMod : function() {
        // nullify the paralysis speed mod
        return true;
    },
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.SPEED)
            return null;
        if (subject.getStatus(StatusEffect.SPECIAL_EFFECT) == null)
            return null;

        // 50% speed increase; priority of 1 (ability modifier)
        return [1.5, 1];
    }
});

/*******************
 * Heatproof
 *******************/
makeAbility({
    name : "Heatproof",
    informBurnDamage : function() {
        // Subject loses 1/16 hp rather than 1/8.
        var damage = Math.floor(this.subject.getStat(Stat.HP) / 16);
        if (damage < 1) damage = 1;
        this.subject.field.print(Text.status_effects_burn(2, this.subject));
        this.subject.hp -= damage;
        return true;
    },
    modifier : function(field, user, target, move, critical) {
        if (target != this.subject)
            return null;
        if (move.type != Type.FIRE)
            return null;
        // 50% base power mod; 6th (target ability) position.
        return [0, 0.5, 6];
    }
});

/*******************
 * Steadfast
 *******************/
makeAbility({
    name : "Steadfast",
    informFlinched : function() {
        // TODO: Steadfast specific message?
        var subject = this.subject;
        subject.applyStatus(subject, new StatChangeEffect(Stat.SPEED, 1));
        return false;
    }
});

/*******************
 * Skill Link
 *******************/
makeAbility({
    name : "Skill Link",
    informMultipleHitMove : function() {
        // Always do five hits.
        return 5;
    }
});

/*******************
 * Rock Head
 *******************/
makeAbility({
    name : "Rock Head",
    informRecoilDamage : function(recoil) {
        return (recoil > 0);
    }
});

/*******************
 * Anger Point
 *******************/
makeAbility({
    name : "Anger Point",
    informCriticalHit : function() {
        var subject = this.subject;
        subject.field.print(Text.ability_messages(1, subject));
        var effect = new StatChangeEffect(Stat.ATTACK, 12);
        effect.silent = true;
        subject.applyStatus(subject, effect);
    }
});

/*******************
 * Adaptability
 *******************/
makeAbility({
    name : "Adaptability",
    informStab : function() {
        return 2;
    }
});

/*******************
 * Sniper
 *******************/
makeAbility({
    name : "Sniper",
    informCritical : function() {
        return 3;
    }
});

/*******************
 * Damp
 *******************/
makeAbility({
    name : "Damp",
    informExplosion : function() {
        this.subject.field.print(Text.ability_messages(6, this.subject));
        return true;
    }
})
