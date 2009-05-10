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
            // TODO: remove status from the subject
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

        // TODO: When this Pokémon is burned, frozen, paralyzed, poisoned,
        // or sleeping, its Speed increases by 50%
        return null; // (temporary)
    }
});




