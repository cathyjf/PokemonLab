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
function Clause(name) {
    this.name = this.id = name;
    Clause[name] = this;
}

Clause.prototype = new StatusEffect();

/**
 * Allows for a nicer syntax for making a clause.
 */
function makeClause(obj) {
    var clause = new Clause(obj.name);
    for (var p in obj) {
        clause[p] = obj[p];
    }
}

function makeClassicEffectClause(clause, id) {
    makeClause({
        name : clause,
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

makeClassicEffectClause("Classic Sleep Clause", "SleepEffect");
makeClassicEffectClause("Classic Freeze Clause", "FreezeEffect");

