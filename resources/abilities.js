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

// This is just a test.
/**ability = new Ability("Own Tempo");
ability.modifier = function(field, user, target, move, critical) {
    // make normal attacks by the ability holder 100 times as strong
    if (user != this.subject)
        return null;
    if (move.type == Type.NORMAL)
        return [1, 100, 1];
    return null;
};
ability.transformHealthChange = function(hp, indirect) {
    // all health changes to the subject take off exactly 10/48 of max hp
    return parseInt(this.subject.stat[0] / 48 * 10);
}**/


