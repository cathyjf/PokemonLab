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

function Ability(name) {
    this.name = this.id = name;
    Ability[name] = this;
}

function makeStatusImmuneAbility(name, immune) {
    var ability = new Ability(name);
    ability.transformStatus = function(subject, status) {
        if (subject != this.subject)
            return status;
        if (status.id == immune)
            return null;
        return status;
    };
    ability.tier = 0;   // TODO: tiers
    ability.tick = function() {
        // TODO: remove status from the subject
    };
}

Ability.prototype = new StatusEffect();

var ability = new Ability("Levitate");
ability.vetoExecution = function(field, user, target, move) {
    if (target != this.subject)
        return false;
    if (move.type != Type.FLYING)
        return false;

    // TODO: use field.print and english.lang
    print(target.name + " evaded the attack!");
    return true;
};

makeStatusImmuneAbility("Insomnia", "SleepEffect");
makeStatusImmuneAbility("Vital Spirit", "SleepEffect");

new Ability("Early Bird"); // no implementation needed

ability = new Ability("Stall");
ability.inherentPriority = function() { return -2; };

// this is just a test
ability = new Ability("Own Tempo");
//ability.vetoSelection = function(user, move) {
//    return (move.name == "Mirror Move");
//};

