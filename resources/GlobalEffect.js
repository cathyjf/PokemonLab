/*
 * File:   GlobalEffect.js
 * Author: Catherine
 *
 * Created on April 13 2009, 2:30 AM
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

makeEffect(StatusEffect, {
    id : "GlobalEffect",
    ticked_ : false,
    beginTick : function() {
        this.ticked_ = false;
    },
    tick : function() {
        if (!this.ticked_) {
            this.ticked_ = true;
            if (this.tickField(this.subject.field))
                return;
        }
        this.tickPokemon();
    },
    tickField : function() { },
    tickPokemon : function() { }
});

makeEffect(GlobalEffect, {
    id : "WeatherEffect",
    turns_ : 5,
    tier : 3,
    tickField : function(field) {
        if ((this.turns_ != -1) && (--this.turns_ == 0)) {
            var effect = getGlobalController(this.subject);
            effect.removeGlobalEffect(this.subject, this.subtier);
            return true;
        }
        this.tickWeather(field);
        return false;
    },
    informApplied : function(user) {
        user.field.print(this.text_(1));
        var turns = user.sendMessage("informApplyWeather", this.subtier);
        if (turns) {
            this.turns_ = turns;
        }
    },
    tickWeather : function(field) {
        field.print(this.text_(2));
    },
    informFinished : function(field) {
        field.print(this.text_(4));
    },
    tickPokemon : function() {
        var flags = getGlobalController(this.subject).flags;
        var subject = this.subject;
        if (subject.sendMessage("informWeatherHealing", flags))
            return;
        if (flags[GlobalEffect.SAND] ||
                flags[GlobalEffect.HAIL] ||
                        (flags[GlobalEffect.SUN] &&
                        (subject.hasAbility("Dry Skin") ||
                                subject.hasAbility("Solarpower")))) {
            var field = subject.field;
            if (flags[GlobalEffect.SUN]) {
                // damaged by ability
                field.print(Text.weather_sun(3, subject, subject.ability));
            } else if (flags[GlobalEffect.SAND]) {
                if (subject.sendMessage("informSandDamage"))
                    return;
                if (subject.isType(Type.GROUND) ||
                        subject.isType(Type.ROCK) ||
                        subject.isType(Type.STEEL))
                    return;
                field.print(Text.weather_sandstorm(3, subject));
            } else {
                if (subject.sendMessage("informHailDamage"))
                    return;
                if (subject.isType(Type.ICE))
                    return;
                field.print(Text.weather_hail(3, subject));
            }

            subject.hp -= Math.floor(subject.getStat(Stat.HP) / 16);
        }
    }
});

GlobalEffect.EFFECTS = ["RainEffect", "SandEffect", "SunEffect", "HailEffect",
                        "FogEffect", "UproarEffect", "GravityEffect",
                        "TrickRoomEffect"];

GlobalEffect.RAIN = 0;
GlobalEffect.SAND = 1;
GlobalEffect.SUN = 2;
GlobalEffect.HAIL = 3;
GlobalEffect.FOG = 4;
GlobalEffect.UPROAR = 5;
GlobalEffect.GRAVITY = 6;
GlobalEffect.TRICK_ROOM = 7;

makeEffect(WeatherEffect, {
    id : "RainEffect",
    name : Text.weather_rain(0),
    text_ : Text.weather_rain,
    subtier : GlobalEffect.RAIN,
    tickPokemon : function() {
        this.subject.sendMessage("informRainHealing");
        WeatherEffect.prototype.tickPokemon.call(this);
    },
    modifier : function(field, user, target, move, critical) {
        var type = move.type;
        if (type == Type.FIRE)
            return [1, 0.5, 3];
        if (type == Type.WATER)
            return [1, 1.5, 3];
        return null;
    }
});

makeEffect(WeatherEffect, {
    id : "SandEffect",
    name : Text.weather_sandstorm(0),
    text_ : Text.weather_sandstorm,
    subtier : GlobalEffect.SAND,
    statModifier : function(field, stat, subject) {
        if (stat != Stat.SPDEFENCE)
            return null;
        if (!subject.isType(Type.ROCK))
            return null;
        return [1.5, 3];
    }
});

makeEffect(WeatherEffect, {
    id : "SunEffect",
    name : Text.weather_sun(0),
    text_ : Text.weather_sun,
    subtier : GlobalEffect.SUN,
    modifier : function(field, user, target, move, critical) {
        var type = move.type;
        if (type == Type.FIRE)
            return [1, 1.5, 4];
        if (type == Type.WATER)
            return [1, 0.5, 4];
        return null;
    }
});

makeEffect(WeatherEffect, {
    id : "HailEffect",
    name : Text.weather_hail(0),
    text_ : Text.weather_hail,
    subtier : GlobalEffect.HAIL,
});

makeEffect(StatusEffect, {
    id : "GlobalEffectController",
    flags : [false, false, false, false, false, false, false, false],
    removeGlobalEffect : function(user, idx) {
        if (!this.flags[idx])
            return;
        this.flags[idx] = false;
        var effect = user.getStatus(GlobalEffect.EFFECTS[idx]);
        if (effect) {
            effect.informFinished(user.field);
            user.field.removeStatus(effect);
        }
    },
    applyGlobalEffect : function(user, idx) {
        this.flags[idx] = true;
        var f = getGlobalEffect(idx);
        var effect = user.field.applyStatus(new f());
        return effect;
    },
    applyWeather : function(user, idx) {
        var fail = [];
        for (var i = GlobalEffect.RAIN; i <= GlobalEffect.FOG; ++i) {
            if (this.flags[i]) {
                fail.push(i);
            }
        }
        if (fail.length > 1) {
            fail.pop();
        }
        if (fail.indexOf(idx) != -1) {
            return null;
        }
        for (var i = GlobalEffect.RAIN; i <= GlobalEffect.FOG; ++i) {
            this.removeGlobalEffect(user, i);
        }
        var effect = this.applyGlobalEffect(user, idx);
        effect.informApplied(user);
        return effect;
    },
    simulateBufferOverflow : function() {
        var idx = this.flags.indexOf(true);
        if (idx == -1)
            return;
        for (var i = 0; i < idx; ++i) {
            var effect = this.applyGlobalEffect(this.subject, i);
            effect.turns_ = -1; // indefinite duration
        }
    }
});

function getGlobalEffect(idx) {
    return this[GlobalEffect.EFFECTS[idx]];
}

function getGlobalController(user) {
    var effect = user.getStatus("GlobalEffectController");
    if (effect)
        return effect;
    return user.field.applyStatus(new GlobalEffectController());
}
