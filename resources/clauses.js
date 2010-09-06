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
    this.idx = Clause.count_;
    Clause.count_ += 1;
    Clause[name] = this;
}

Clause.count_ = 0;
Clause.prototype = new StatusEffect();
Clause.prototype.type = StatusEffect.TYPE_CLAUSE;

/**
 * Allows for a nicer syntax for making a clause.
 */
function makeClause(obj) {
    var clause = new Clause(obj.name);
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
        description : description,
        transformStatus : function(subject, status) {
            if (status.id != id)
                return status;
            var party = subject.party;
            if (status.inducer && (status.inducer.party == party))
                return status;
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
        name : clause,
        description : description,
        validateTeam : function(team) {
            for (var i in team) {
                var p = team[i];
                for (var j = 0; j < p.moveCount; j++) {
                    var move = p.getMove(j);
                    if (move && move[param])
                        return false;
                }
            }
            return true;
        }
    });
}

makeIllegalMoveClause("OHKO Clause", "isOneHitKill_",
    "One hit knock out moves are not allowed");

makeIllegalMoveClause("Evasion Clause", "isEvasionMove_",
    "Moves that specifically raise evasion are not allowed");

makeClause({
    name : "Strict Damage Clause",
    description : "Reported damage is never greater than the " +
                    "target's remaining health",
    informReportDamage : function(delta, hp, max) {
        if (delta > 0)
            return (delta > hp) ? hp : delta;
        return ((hp - delta) > max) ? (hp - max) : delta;
    }
});

makeClause({
    name : "Species Clause",
    description : "All Pokemon on a team must be of different species",
    validateTeam : function(team) {
        var used = [];
        for (var i in team) {
            var species = team[i].species;
            if (used.indexOf(species) != -1)
                return false;
            used.push(species);
        }
        return true;
    }
});

makeClause({
    name : "Item Clause",
    description : "All Pokemon on a team must have different items",
    validateTeam : function(team) {
        var used = [];
        for (var i in team) {
            var item = team[i].itemName;
            if (used.indexOf(item) != -1)
                return false;
            used.push(item);
        }
        return true;
    }
});

makeClause({
    name : "Level Balance",
    description : "Each Pokemon's level is adjusted to be in the " +
            "range [60,100] based on its base stat rating",
    s : function(spd) {
        //returns a 4th order approximation to the percentile of 
        //speeds that a certain speed falls into.
        var p1 = 7.2915e-09;
        var p2 = -3.1887e-06;
        var p3 = 4.1592e-04;
        var p4 = -0.0089004;
        var p5 = 0.052822;
        return p1 * Math.pow(spd, 4) + 
               p2 * Math.pow(spd, 3) + 
               p3 * Math.pow(spd, 2) + 
               p4 * spd + p5;
    },
    bsr : function(hp, atk, def, spd, spatk, spdef) {
        //this clause uses X-Act's Base Stat Ratings v.2
        //as the measure to balance a pokemon's level
        //http://www.smogon.com/forums/showthread.php?t=64133
        var shp = hp * 2 + 141;
        var satk = atk * 2 + 36;
        var sdef = def * 2 + 36;
        var sspa = spatk * 2 + 36;
        var sspd = spdef * 2 + 36;
        var sspe = this.s(spd);
        
        var rpt = shp * sdef
        var rst = shp * sspd
        var rps = satk * (satk * sspe + 415) / (satk * (1 - sspe) + 415);
        var rss = sspa * (sspa * sspe + 415) / (sspa * (1 - sspe) + 415);
        
        var pt = rpt / 417.5187 - 18.9256;
        var st = rst / 434.8833 - 13.9044;
        var ps = rps / 1.855522 - 4.36533;
        var ss = rss / 1.947004 + 4.36062;
        
        return (pt + st + ps + ss) / 1.525794 - 62.1586;
    },
    getLevel : function(rating) {
        //return an appropriate level given a BSR
        //we will use this function (for now) which is
        //level = 180.5 - 19.5 * log_e(bsr), capped at 100
        var level = 180.5 - 19.5 * Math.log(rating);
        return (level > 100) ? 100 : Math.floor(level);
    },
    transformTeam : function(team) {
        for (var i in team) {
            var p = team[i];
            var bases = p.base;
            var rating = this.bsr(bases[Stat.HP], bases[Stat.ATTACK], 
                    bases[Stat.DEFENCE], bases[Stat.SPEED], 
                    bases[Stat.SPATTACK], bases[Stat.SPDEFENCE]);
            var level = this.getLevel(rating);
            p.level = level;
        }
    }
});
