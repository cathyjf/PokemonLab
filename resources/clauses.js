/*
 * File:   clauses.js
 * Author: Catherine
 *
 * Created on August 21, 2009, 2:44 PM
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
 * Clause function
 *
 * Constructs a new clause and makes it a property of the Clause function.
 */
function Clause(name, description) {
    this.name = this.id = name;
    this.description = description;
    Clause[name] = this;
}

Clause.prototype = new StatusEffect();
Clause.prototype.type = StatusEffect.TYPE_CLAUSE;

/**
 * Allows for a nicer syntax for making a clause.
 */
function makeClause(obj) {
    var clause = new Clause(obj.name, obj.description);
    for (var p in obj) {
        clause[p] = obj[p];
    }
}

/**
 * Classic clauses allow only one instance of an effect in a party.
 * Any attempts to apply the status to a 2nd Pokemon will fail
 */ 
function makeClassicEffectClause(clause, id, description) {
    makeClause({
        name : clause,
        description: description,
        transformStatus : function(subject, status) {
            if (status.id != id)
                return status;
            var party = subject.party;
            var length = subject.field.getPartySize(party);
            for (var i = 0; i < length; ++i) {
                var p = subject.field.getPokemon(party, i);
                if (p.fainted) continue;
                var effect = p.getStatus(id);
                if (effect && effect.inducer && (effect.inducer.party != party))
                    return null;
            }
            return status;
        }
    });
}

makeClassicEffectClause("Classic Sleep Clause", "SleepEffect",
    "Only one Pokemon in a party can be asleep due to an enemy move " +
    "at any time. Any further attempts by an enemy to put" +
    " another Pokemon to sleep will fail");
makeClassicEffectClause("Classic Freeze Clause", "FreezeEffect",
    "Only one Pokemon in a party can be frozen due to an enemy move " +
    "at any time. Any further attempts by an enemy to freeze" +
    " another Pokemon will fail");

/**
 * Makes a clause which checks if pokemon has any illegal moves.
 * This is checked before the battle begins
 * param is a flag that is set on the type of disallowed move
 */
function makeIllegalMoveClause(clause, param, description) {
    makeClause({
        name: clause,
        description: description,
        validatePokemon: function(p) {
            for (var i in p.moves) {
                if (moves.param)
                    return false;
            }
            return true;
        }
    });
}

makeIllegalMoveClause("OHKO Clause", "isOneHitKill_",
    "One hit knock out moves are not allowed");


makeClause({
    name : "Strict Damage Clause",
    description : "Reported damage is never greater than the " +
                    "target's remaining health",
    transformHealthChange : function(delta, user, indirect) {
        if (delta > user.hp) {
            return user.hp;
        }
        return delta;
    }
});

