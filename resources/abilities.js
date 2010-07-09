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
Ability.prototype.type = StatusEffect.TYPE_ABILITY;
Ability.prototype.switchIn = function() {
    if (!this.informActivate)
        return;
    var id_ = this.id + "ActivatedEffect";
    if (this.subject.getStatus(id_))
        return;
    var effect = new StatusEffect(id_);
    this.subject.applyStatus(this.subject, effect);
    this.informActivate();
};
Ability.prototype.getState = function() {
    if (!this.subject || (this.state != StatusEffect.STATE_ACTIVE))
        return this.state;
    if (this.subject.getStatus("GastroAcidEffect"))
        return StatusEffect.STATE_DEACTIVATED;
    if ((this.id == "Klutz") || (this.id == "Mold Breaker"))
        return StatusEffect.STATE_ACTIVE;
    var p = this.subject.field.executionUser;
    if (p && (p != this.subject) && p.hasAbility("Mold Breaker"))
        return StatusEffect.STATE_DEACTIVATED;
    return StatusEffect.STATE_ACTIVE;
};

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
            for (i in immune) {
                if (immune[i] == status.id)
                    return null;
            }
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

function makeCriticalTypeAbility(ability, type) {
    makeAbility({
        name : ability,
        modifier : function(field, user, target, move, critical) {
            if (user != this.subject)
                return null;
            if (move.type != type)
                return null;
            if (user.hp > Math.floor(user.getStat(Stat.HP) / 3))
                return null;
            // 150% base power mod; 5th (user ability) position.
            return [0, 1.5, 5];
        }
    });
}

function makeStatLevelProtectionAbility(ability, predicate) {
    makeAbility({
        name : ability,
        transformStatus : function(subject, status) {
            if (subject != this.subject)
                return status;
            if (status.inducer == this.subject)
                return status;
            if (status.id != "StatChangeEffect")
                return status;
            if (status.delta_ > 0)
                return status;
            if (!predicate(status.stat))
                return status;
            return null;
        }
    });
}

function makeFilterAbility(ability) {
    makeAbility({
        name : ability,
        modifier : function(field, user, target, move, critical) {
            if (target != this.subject)
                return null;
            if (field.getEffectiveness(move.type, target) < 2.0)
                return null;
            return [3, 0.75, 0];
        }
    });
}

function makeWeatherAbility(ability, idx, text) {
    makeAbility({
        name : ability,
        informActivate : function() {
            this.subject.field.print(text(this.subject));
            var controller = getGlobalController(this.subject);
            var effect = controller.applyWeather(this.subject, idx);
            if (!effect) {
                effect = this.subject.getStatus(GlobalEffect.EFFECTS[idx]);
            }
            if (effect) {
                effect.turns_ = -1;
            }
        }
    });
}

function makeAbsorbMove(ability, type) {
    makeAbility({
        name : ability,
        vetoExecution : function(field, user, target, move) {
            if (user == this.subject)
                return false;
            if (target != this.subject)
                return false;
            if (move.type != type)
                return false;
            var max = target.getStat(Stat.HP);
            if (target.hp != max) {
                field.print(Text.ability_messages(12, target, this));
                var delta = Math.floor(max / 4);
                target.hp += delta;
            }
            return true;
        }
    });
}

/*******************
 * Volt Absorb
 *******************/
makeAbsorbMove("Volt Absorb", Type.ELECTRIC);

/*******************
 * Water Absorb
 *******************/
makeAbsorbMove("Water Absorb", Type.WATER);

/*******************
 * Motor Drive
 *******************/
makeAbility({
    name : "Motor Drive",
    vetoExecution : function(field, user, target, move) {
        if (user == this.subject)
            return false;
        if (target != this.subject)
            return false;
        if (move.type != Type.ELECTRIC)
            return false;
        if (target.isType(Type.GROUND))
            return false;
        var effect = new StatChangeEffect(Stat.SPEED, 1);
        effect.silent = true;
        if (target.applyStatus(target, effect)) {
            field.print(Text.ability_messages(29, target));
        }
        return true;
    }
});

/*******************
 * Flash Fire
 *******************/
makeAbility({
    name : "Flash Fire",
    vetoExecution : function(field, user, target, move) {
        if (user == this.subject)
            return false;
        if (target != this.subject)
            return false;
        if (move.type != Type.FIRE)
            return false;
        if ((move.power == 0) && (move.name != "Will-o-wisp"))
            return false;
        if (target.getStatus("FreezeEffect"))
            return false;
        field.print(Text.ability_messages(15, target));
        if (!target.getStatus("FlashFireEffect")) {
            var effect = new StatusEffect("FlashFireEffect");
            effect.modifier = function(field, user, target, move, critical) {
                if (user != this.subject)
                    return null;
                if (move.type != Type.FIRE)
                    return null;
                return [1, 1.5, 5];
            };
            target.applyStatus(target, effect);
        }
        return true;
    }
});

/*******************
 * Drizzle
 *******************/
makeWeatherAbility("Drizzle",
        GlobalEffect.RAIN, Text.ability_messages.wrap(8));

/*******************
 * Drought
 *******************/
makeWeatherAbility("Drought",
        GlobalEffect.SUN, Text.ability_messages.wrap(9));

/*******************
 * Snow Warning
 *******************/
makeWeatherAbility("Snow Warning",
        GlobalEffect.HAIL, Text.ability_messages.wrap(41));

/*******************
 * Sand Stream
 *******************/
makeWeatherAbility("Sand Stream",
        GlobalEffect.SAND, Text.ability_messages.wrap(38));

/*******************
 * Tinted Lens
 *******************/
makeAbility({
    name : "Tinted Lens",
    modifier : function(field, user, target, move, critical) {
        if (user != this.subject)
            return null;
        if (field.getEffectiveness(move.type, target) >= 1.0)
            return null;
        return [3, 2, 2];
    }
});

/*******************
 * Battle Armor
 *******************/
makeAbility({
    name : "Battle Armor",
    informAttemptCritical : function() {
        return true;
    }
});

/*******************
 * Shell Armor
 *******************/
makeAbility({
    name : "Shell Armor",
    informAttemptCritical : function() {
        return true;
    }
});

/*******************
 * Filter
 *******************/
makeFilterAbility("Filter");

/*******************
 * Solid Rock
 *******************/
makeFilterAbility("Solid Rock");

/*******************
 * Overgrow
 *******************/
makeCriticalTypeAbility("Overgrow", Type.GRASS);

/*******************
 * Blaze
 *******************/
makeCriticalTypeAbility("Blaze", Type.FIRE);

/*******************
 * Torrent
 *******************/
makeCriticalTypeAbility("Torrent", Type.WATER);

/*******************
 * Swarm
 *******************/
makeCriticalTypeAbility("Swarm", Type.BUG);

/*******************
 * Levitate
 *******************/
makeAbility({
    name : "Levitate",
    immunity : function(user, target) {
        if (target != this.subject)
            return -1;
        return Type.GROUND;
    }
});

/*******************
 * Synchronize
 *******************/
makeAbility({
    name : "Synchronize",
    informEffectApplied : function(effect) {
        var subject = effect.inducer;
        if (subject == this.subject)
            return;
        if (effect.lock != StatusEffect.SPECIAL_EFFECT)
            return;
        if ((effect.id != "PoisonEffect") && (effect.id != "ToxicEffect")
                && (effect.id != "BurnEffect")
                && (effect.id != "ParalysisEffect"))
            return;
        if (effect.id == "ToxicEffect") {
            effect = new PoisonEffect();
        }
        if (subject.applyStatus(subject, effect)) {
            subject.field.print(Text.ability_messages(48, this.subject));
        }
    }
});

/*******************
 * Slow Start
 *******************/
makeAbility({
    name : "Slow Start",
    tier : 0,
    informActivate : function() {
        if (this.message_)
            return;
        this.message_ = true;
        this.subject.field.print(Text.ability_messages(40, this.subject));
        this.turns_ = 5;
    },
    applyEffect : function() {
        this.message_ = false;
        if (this.subject.position != -1) {
            this.informActivate();
        }
    },
    tick : function() {
        if (--this.turns_ > 0)
            return;
        this.subject.field.print(Text.ability_messages(58, this.subject));
    },
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (this.turns_ <= 0)
            return null;
        if ((stat != Stat.ATTACK) && (stat != Stat.SPEED))
            return null;
        return [0.5, 1];
    }
});

/*******************
 * Pressure
 *******************/
makeAbility({
    name : "Pressure",
    informActivate : function() {
        this.subject.field.print(Text.ability_messages(34, this.subject));
    },
    informTargeted : function(user, move) {
        if (user == this.subject)
            return;
        var pp = user.getPp(move);
        if (pp > 0) {
            user.setPp(move, pp - 1);
        }
    }
});

/*******************
 * Gluttony
 *******************/
makeAbility({
    name : "Gluttony",
    informPinchBerry : function() {
        return 2;
    }
});

/*******************
 * Intimidate
 *******************/
makeAbility({
    name : "Intimidate",
    informActivate : function() {
        var field = this.subject.field;
        var party_ = 1 - this.subject.party;
        var effect = new StatChangeEffect(Stat.ATTACK, -1);
        effect.silent = true;
        for (var i = 0; i < field.partySize; ++i) {
            var p = field.getActivePokemon(party_, i);
            if (p && p.applyStatus(null, effect)) {
                field.print(Text.ability_messages(23, this.subject, p));
            }
        }
    },
});

/*******************
 * Frisk
 *******************/
 makeAbility({
     name : "Frisk",
     informActivate : function() {
        var field = this.subject.field;
        var party_ = 1 - this.subject.party;
        var choices = [];
        for (var i = 0; i < field.partySize; ++i) {
            var p = field.getActivePokemon(party_, i);
            if (p && p.item) {
                choices.push(p);
            }
        }
        var length = choices.length;
        if (length == 0)
            return;
        var p = choices[field.random(0, length - 1)];
        field.print(Text.ability_messages(18, this.subject, p, p.item));
     }
 });

/*******************
 * Trace
 *******************/
 makeAbility({
     name : "Trace",
     informActivate : function() {
        var field = this.subject.field;
        var party_ = 1 - this.subject.party;
        var choices = [];
        for (var i = 0; i < field.partySize; ++i) {
            var p = field.getActivePokemon(party_, i);
            if (p && p.ability && (p.ability.id != "Multitype")) {
                choices.push(p);
            }
        }
        var length = choices.length;
        if (length == 0)
            return;
        var p = choices[field.random(0, length - 1)];
        field.print(Text.ability_messages(50, this.subject, p, p.ability));
        this.subject.ability = p.ability;
        this.subject.ability.switchIn();
     }
 });

/*******************
 * Aftermath
 *******************/
makeAbility({
    name : "Aftermath",
    informDamaged : function(user, move, damage) {
        var subject = this.subject;
        if ((subject.hp <= 0) && (damage > 0)
                && move.flags[Flag.CONTACT]) {
            subject.field.print(Text.ability_messages(0, user, subject));
            var delta = Math.floor(user.getStat(Stat.HP) / 4);
            if (delta < 1) delta = 1;
            user.hp -= delta;
        }
    }
});

/*******************
 * Rough Skin
 *******************/
makeAbility({
    name : "Rough Skin",
    informDamaged : function(user, move, damage) {
        var subject = this.subject;
        if ((damage > 0) && move.flags[Flag.CONTACT]) {
            subject.field.print(Text.ability_messages(37, user, subject));
            var delta = Math.floor(user.getStat(Stat.HP) / 8);
            if (delta < 1) delta = 1;
            user.hp -= delta;
        }
    }
});

/*******************
 * Flame Body
 *******************/
makeAbility({
    name : "Flame Body",
    informDamaged : function(user, move, damage) {
        var subject = this.subject;
        if ((damage > 0) && move.flags[Flag.CONTACT]
                && subject.field.random(0.3)) {
            if (user.applyStatus(user, new BurnEffect())) {
                subject.field.print(Text.ability_messages(14, subject, user));
            }
        }
    }
});

/*******************
 * Static
 *******************/
makeAbility({
    name : "Static",
    informDamaged : function(user, move, damage) {
        var subject = this.subject;
        if ((damage > 0) && move.flags[Flag.CONTACT]
                && subject.field.random(0.3)) {
            if (user.applyStatus(user, new ParalysisEffect())) {
                subject.field.print(Text.ability_messages(44, subject, user));
            }
        }
    }
});

/*******************
 * Poison Point
 *******************/
makeAbility({
    name : "Poison Point",
    informDamaged : function(user, move, damage) {
        var subject = this.subject;
        if ((damage > 0) && move.flags[Flag.CONTACT]
                && subject.field.random(0.3)) {
            if (user.applyStatus(user, new PoisonEffect())) {
                subject.field.print(Text.ability_messages(33, subject, user));
            }
        }
    }
});

/*******************
 * Cute Charm
 *******************/
makeAbility({
    name : "Cute Charm",
    informDamaged : function(user, move, damage) {
        var subject = this.subject;
        if ((damage > 0) && move.flags[Flag.CONTACT]
                && subject.field.random(0.3)
                && isOppositeGender(subject, user)) {
            if (user.applyStatus(user, new AttractEffect())) {
                subject.field.print(Text.ability_messages(5, subject, user));
            }
        }
    }
});

/*******************
 * Color Change
 *******************/
makeAbility({
    name : "Color Change",
    informDamaged : function(user, move, damage) {
        var subject = this.subject;
        if ((damage > 0) && (move.name != "Pain Split")
                && !subject.isType(move.type)) {
            subject.setTypes([move.type]);
            subject.field.print(Text.battle_messages_unique(9, subject,
                    Text.types(move.type)));
        }
    }
});

/*******************
 * Sturdy
 *******************/
makeAbility({
    name : "Sturdy",
    informOHKO : function() {
        this.subject.field.print(Text.ability_messages(47, this.subject));
        return true;
    }
});

/*******************
 * Arena Trap
 *******************/
makeAbility({
    name : "Arena Trap",
    vetoSwitch : function(subject) {
        if (subject.party == this.subject.party)
            return false;
        if (subject.sendMessage("informBlockSwitch"))
            return false;
        var factor = subject.field.getEffectiveness(Type.GROUND, subject);
        return (factor > 0);
    }
});

/*******************
 * Shadow Tag
 *******************/
makeAbility({
    name : "Shadow Tag",
    vetoSwitch : function(subject) {
        if (subject.party == this.subject.party)
            return false;
        if (subject.sendMessage("informBlockSwitch"))
            return false;
        return !subject.hasAbility(this.id);
    }
});

/*******************
 * Magnet Pull
 *******************/
makeAbility({
    name : "Magnet Pull",
    vetoSwitch : function(subject) {
        if (subject.party == this.subject.party)
            return false;
        if (subject.sendMessage("informBlockSwitch"))
            return false;
        return subject.isType(Type.STEEL);
    }
});

/*******************
 * Suction Cups
 *******************/
makeAbility({
    name : "Suction Cups",
    informRandomSwitch : function() {
        return true;
    }
});

/*******************
 * Bad Dreams
 *******************/
makeAbility({
    name : "Bad Dreams",
    informSleepTick : function(effect) {
        // TODO: Research question: Does Bad Dreams affect allies? For now,
        //       I assume it does.
        if (effect.subject == this.subject)
            return;
        this.subject.field.print(Text.ability_messages(3,
                effect.subject, this.subject));
        var delta = Math.floor(effect.subject.getStat(Stat.HP) / 8);
        if (delta < 1) delta = 1;
        effect.subject.hp -= delta;
    }
});

/*******************
 * Air Lock
 *******************/
makeAbility({
    name : "Air Lock",
    informWeatherEffects : function() {
        return true;
    }
});

/*******************
 * Cloud Nine
 *******************/
makeAbility({
    name : "Cloud Nine",
    informWeatherEffects : function() {
        return true;
    }
});

/*******************
 * Super Luck
 *******************/
makeAbility({
    name : "Super Luck",
    criticalModifier : function() {
        return 1;
    }
});

/*******************
 * Insomnia
 *******************/
makeStatusImmuneAbility("Insomnia", ["SleepEffect"]);

/*******************
 * Vital Spirit
 *******************/
makeStatusImmuneAbility("Vital Spirit", ["SleepEffect"]);

/*******************
 * Water Veil
 *******************/
makeStatusImmuneAbility("Water Veil", ["BurnEffect"]);

/*******************
 * Magma Armor
 *******************/
makeStatusImmuneAbility("Magma Armor", ["FreezeEffect"]);

/*******************
 * Limber
 *******************/
makeStatusImmuneAbility("Limber", ["ParalysisEffect"]);

/*******************
 * Own Tempo
 *******************/
makeStatusImmuneAbility("Own Tempo", ["ConfusionEffect"]);

/*******************
 * Immunity
 *******************/
makeStatusImmuneAbility("Immunity", ["PoisonEffect", "ToxicEffect"]);


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
 * Liquid Ooze
 *******************/
makeAbility({
    name : "Liquid Ooze",
    informDrainHealth : function(user, damage) {
        this.subject.field.print(Text.ability_messages(55, user));
        user.hp -= damage;
        return true;
    }
});

/*******************
 * Solar Power
 *******************/
makeAbility({
    name : "Solar Power",
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.SPATTACK)
            return null;
        if (!isWeatherActive(subject, GlobalEffect.SUN))
            return null;
        return [1.5, 1];
    },
    informSunDamage : function() {
        return true;
    }
});

/*******************
 * Plus
 *******************/
makeAbility({
    name : "Plus",
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.SPATTACK)
            return null;
        if (!field.sendMessage("informMinus"))
            return null;
        return [1.5, 1];
    },
    informPlus : function() {
        return true;
    }
});

/*******************
 * Minus
 *******************/
makeAbility({
    name : "Minus",
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.SPATTACK)
            return null;
        if (!field.sendMessage("informPlus"))
            return null;
        return [1.5, 1];
    },
    informMinus : function() {
        return true;
    }
});

/*******************
 * Huge Power
 *******************/
makeAbility({
    name : "Huge Power",
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.ATTACK)
            return null;
        return [2, 1];
    }
});

/*******************
 * Pure Power
 *******************/
makeAbility({
    name : "Pure Power",
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.ATTACK)
            return null;
        return [2, 1];
    }
});

/*******************
 * Leaf Guard
 *******************/
makeAbility({
    name : "Leaf Guard",
    transformStatus : function(subject, status) {
        if (subject != this.subject)
            return status;
        if (status.inducer == this.subject)
            return status;
        if (status.lock != StatusEffect.SPECIAL_EFFECT)
            return status;
        if (!isWeatherActive(subject, GlobalEffect.SUN))
            return status;
        return null;
    }
});

/*******************
 * Chlorophyll
 *******************/
makeAbility({
    name : "Chlorophyll",
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.SPEED)
            return null;
        if (!isWeatherActive(subject, GlobalEffect.SUN))
            return null;
        return [2, 1];
    }
});

/*******************
 * Swift Swim
 *******************/
makeAbility({
    name : "Swift Swim",
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.SPEED)
            return null;
        if (!isWeatherActive(subject, GlobalEffect.RAIN))
            return null;
        return [2, 1];
    }
});

/*******************
 * Sand Veil
 *******************/
makeAbility({
    name : "Sand Veil",
    statModifier : function(field, stat, subject, target) {
        if (stat != Stat.ACCURACY)
            return null;
        if (target != this.subject)
            return null;
        if (!isWeatherActive(subject, GlobalEffect.SAND))
            return null;
        return [0.8, 3];
    },
    informSandDamage : function() {
        return true;
    }
});

/*******************
 * Snow Cloak
 *******************/
makeAbility({
    name : "Snow Cloak",
    statModifier : function(field, stat, subject, target) {
        if (stat != Stat.ACCURACY)
            return null;
        if (target != this.subject)
            return null;
        if (!isWeatherActive(subject, GlobalEffect.HAIL))
            return null;
        return [0.8, 4];
    },
    informHailDamage : function() {
        return true;
    }
});

/*******************
 * Compoundeyes
 *******************/
makeAbility({
    name : "Compoundeyes",
    statModifier : function(field, stat, subject, target) {
        if (stat != Stat.ACCURACY)
            return null;
        if (subject != this.subject)
            return null;
        return [1.3, 2];
    },
});

/*******************
 * Tangled Feet
 *******************/
makeAbility({
    name : "Tangled Feet",
    statModifier : function(field, stat, subject, target) {
        if (stat != Stat.ACCURACY)
            return null;
        if (target != this.subject)
            return null;
        if (!target.getStatus("ConfusionEffect"))
            return null;
        return [0.5, 7];
    },
});

/*******************
 * Hustle
 *******************/
makeAbility({
    name : "Hustle",
    statModifier : function(field, stat, subject, target) {
        if (subject != this.subject)
            return null;
        if (stat == Stat.ACCURACY) {
            if (!field.execution)
                return null;
            if (field.execution.moveClass != MoveClass.PHYSICAL)
                return null;
            return [0.8, 6];
        }
        if (stat != Stat.ATTACK)
            return null;
        return [1.5, 1];
    }
})

/*******************
 * Rain Dish
 *******************/
makeAbility({
    name : "Rain Dish",
    informRainHealing : function() {
        var max = this.subject.getStat(Stat.HP);
        if (this.subject.hp == max)
            return;
        this.subject.field.print(Text.ability_messages(36, this.subject));
        var delta = Math.floor(max / 16);
        this.subject.hp += delta;
    }
})

/*******************
 * Ice Body
 *******************/
makeAbility({
    name : "Ice Body",
    informHailDamage : function() {
        return true;
    },
    informWeatherHealing : function(flags) {
        if (!flags[GlobalEffect.HAIL])
            return false;
        var max = this.subject.getStat(Stat.HP);
        if (this.subject.hp == max)
            return false;
        this.subject.field.print(Text.ability_messages(56, this.subject));
        var delta = Math.floor(max / 16);
        this.subject.hp += delta;
        return true;
    }
});

/*******************
 * Shed Skin
 *******************/
makeAbility({
    name : "Shed Skin",
    tier : 6,
    subtier : 0,
    tick : function() {
        var effect = this.subject.getStatus(StatusEffect.SPECIAL_EFFECT);
        if (effect && this.subject.field.random(0.3)) {
            this.subject.field.print(Text.ability_messages(39, this.subject));
            this.subject.removeStatus(effect);
        }
    }
});

/*******************
 * Natural Cure
 *******************/
makeAbility({
    name : "Natural Cure",
    switchOut : function() {
        var effect = this.subject.getStatus(StatusEffect.SPECIAL_EFFECT);
        if (effect) {
            this.subject.removeStatus(effect);
        }
        return true;
    }
});

/*******************
 * White Smoke
 *******************/
makeStatLevelProtectionAbility("White Smoke", function() { return true; });

/*******************
 * Clear Body
 *******************/
makeStatLevelProtectionAbility("Clear Body", function() { return true; });

/*******************
 * Hyper Cutter
 *******************/
makeStatLevelProtectionAbility("Hyper Cutter", function(stat) {
    return (stat == Stat.ATTACK);
});

/*******************
 * Keen Eye
 *******************/
makeStatLevelProtectionAbility("Keen Eye", function(stat) {
    return (stat == Stat.ACCURACY);
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
 * Guts
 *******************/
makeAbility({
    name : "Guts",
    informBurnMod : function() {
        // nullify the burn attack mod
        return true;
    },
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.ATTACK)
            return null;
        if (subject.getStatus(StatusEffect.SPECIAL_EFFECT) == null)
            return null;

        // 50% attack increase; priority of 1 (ability modifier)
        return [1.5, 1];
    }
});

/*******************
 * Marvel Scale
 *******************/
makeAbility({
    name : "Marvel Scale",
    statModifier : function(field, stat, subject) {
        if (subject != this.subject)
            return null;
        if (stat != Stat.DEFENCE)
            return null;
        if (subject.getStatus(StatusEffect.SPECIAL_EFFECT) == null)
            return null;
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
 * Dry Skin
 *******************/
makeAbility({
    name : "Dry Skin",
    informWeatherHealing : function(flags) {
        if (!flags[GlobalEffect.RAIN])
            return false;
        var max = this.subject.getStat(Stat.HP);
        if (this.subject.hp == max)
            return false;
        this.subject.field.print(Text.ability_messages(57, this.subject));
        var delta = Math.floor(max / 8);
        this.subject.hp += delta;
        return true;
    },
    informSunDamage : function() {
        return true;
    },
    modifier : function(field, user, target, move, critical) {
        if (target != this.subject)
            return null;
        if (move.type != Type.FIRE)
            return null;
        return [0, 1.25, 6];
    },
    vetoExecution : function(field, user, target, move) {
        if (user == this.subject)
            return false;
        if (target != this.subject)
            return false;
        if (move.type != Type.WATER)
            return false;
        var max = target.getStat(Stat.HP);
        if (target.hp != max) {
            field.print(Text.ability_messages(12, target, this));
            var delta = Math.floor(max / 4);
            target.hp += delta;
        }
        return true;
    }
});

/*******************
 * Thick Fat
 *******************/
makeAbility({
    name : "Thick Fat",
    modifier : function(field, user, target, move, critical) {
        if (target != this.subject)
            return null;
        if ((move.type != Type.FIRE) && (move.type != Type.ICE))
            return null;
        // 50% base power mod; 6th (target ability) position.
        return [0, 0.5, 6];
    }
});

/*******************
 * Technician
 *******************/
makeAbility({
    name : "Technician",
    modifier : function(field, user, target, move, critical) {
        if (user != this.subject)
            return null;
        if (move.power > 60)
            return null;
        if (move.name == "Struggle")
            return null;
        return [0, 1.5, 5];
    }
});

/*******************
 * Iron Fist
 *******************/
makeAbility({
    name : "Iron Fist",
    moves_ : ["Ice Punch", "Fire Punch", "Thunderpunch", "Mach Punch",
              "Focus Punch", "Dizzy Punch", "Dynamicpunch", "Hammer Arm",
              "Mega Punch", "Comet Punch", "Meteor Mash", "Shadow Punch",
              "Drain Punch", "Bullet Punch", "Sky Uppercut"],
    modifier : function(field, user, target, move, critical) {
        if (user != this.subject)
            return null;
        if (this.moves_.indexOf(move.name) == -1)
            return null;
        return [0, 1.2, 5];
    }
});

/*******************
 * Reckless
 *******************/
makeAbility({
    name : "Reckless",
    moves_ : ["Brave Bird", "Double-edge", "Flare Blitz", "Head Smash",
              "Submission", "Take Down", "Volt Tackle", "Wood Hammer",
              "Jump Kick", "Hi Jump Kick"],
    modifier : function(field, user, target, move, critical) {
        if (user != this.subject)
            return null;
        if (this.moves_.indexOf(move.name) == -1)
            return null;
        return [0, 1.2, 5];
    }
});

/*******************
 * Rivalry
 *******************/
makeAbility({
    name : "Rivalry",
    modifier : function(field, user, target, move, critical) {
        if (user != this.subject)
            return null;
        if ((user.gender == Gender.NONE) || (target.gender == Gender.NONE))
            return null;
        if (user.gender == target.gender)
            return [0, 1.25, 5];
        return [0, 0.75, 5];
    }
});

/*******************
 * Simple
 *******************/
makeAbility({
    name : "Simple",
    transform_ : function(level) {
        level *= 2;
        if (level > 6) {
            level = 6;
        } else if (level < -6) {
            level = -6;
        }
        return level;
    },
    transformStatLevel : function(user, target, stat, level) {
        if (user == this.subject) {
            if ((stat == Stat.ACCURACY)
                    || (stat == Stat.ATTACK)
                    || (stat == Stat.SPATTACK)
                    || (stat == Stat.SPEED)) {
                return [this.transform_(level), false];
            }
        }
        if (target != this.subject)
            return null;
        if ((stat != Stat.EVASION)
                && (stat != Stat.DEFENCE)
                && (stat != Stat.SPDEFENCE))
            return null;
        return [this.transform_(level), false];
    }
});

/*******************
 * Unaware
 *******************/
makeAbility({
    name : "Unaware",
    transformStatLevel : function(user, target, stat, level) {
        if (target == this.subject) {
            if ((stat == Stat.ACCURACY)
                    || (stat == Stat.ATTACK)
                    || (stat == Stat.SPATTACK)) {
                return [0, true];
            }
        }
        if (user != this.subject)
            return null;
        if ((stat != Stat.EVASION)
                && (stat != Stat.DEFENCE)
                && (stat != Stat.SPDEFENCE))
            return null;
        return [0, true];
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
 * Inner Focus
 *******************/
makeAbility({
    name : "Inner Focus",
    informMaybeFlinch : function() {
        return true;
    }
});

/*******************
 * Hydration
 *******************/
makeAbility({
    name : "Hydration",
    tier : 4,
    subtier : 0,
    tick : function() {
        if (!isWeatherActive(this.subject, GlobalEffect.RAIN))
            return;
        var effect = this.subject.getStatus(StatusEffect.SPECIAL_EFFECT);
        if (effect) {
            this.subject.field.print(Text.ability_messages(19, this.subject));
            this.subject.removeStatus(effect);
        }
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
    },
    vetoExecution : function(field, user, target, move) {
        if (target == null)
            return false;
        return ((move.name == "Explosion") || (move.name == "Selfdestruct"));
    }
});

/*******************
 * Oblivious
 *******************/
makeAbility({
    name : "Oblivious",
    informGenderBased : function() {
        return true;
    }
});

/*******************
 * Sticky Hold
 *******************/
makeAbility({
    name : "Sticky Hold",
    informRemoveItem : function() {
        this.subject.field.print(Text.ability_messages(46, this.subject));
        return true;
    }
});

/*******************
 * Soundproof
 *******************/
makeAbility({
    name : "Soundproof",
    vetoExecution : function(field, user, target, move) {
        if (target != this.subject)
            return false;
        if (!move.flags[Flag.SOUND])
            return false;
        field.print(Text.ability_messages(42, this.subject, this, move));
        return true;
    }
});

/*******************
 * Serene Grace
 *******************/
makeAbility({
    name : "Serene Grace"
});

/*******************
 * Shield Dust
 *******************/
makeAbility({
    name : "Shield Dust",
    informSecondaryEffect : function() {
        return true;
    }
});

/*******************
 * Klutz
 *******************/
makeAbility({
    name : "Klutz"
});

/*******************
 * Mold Breaker
 *******************/
makeAbility({
    name : "Mold Breaker"
});

/*******************
 * Stench
 *******************/
makeAbility({
    name : "Stench"
    // Has no effect in battle.
});

/*******************
 * Honey Gather
 *******************/
makeAbility({
    name : "Honey Gather"
    // Has no effect in battle.
});

/*******************
 * Run Away
 *******************/
makeAbility({
    name : "Run Away"
    // Has no effect in battle.
});

/*******************
 * Pickup
 *******************/
makeAbility({
    name : "Pickup"
    // Has no effect in battle.
});

/*******************
 * Illuminate
 *******************/
makeAbility({
    name : "Illuminate"
    // Has no effect in battle.
});

/*******************
 * Download
 *******************/
makeAbility({
    name : "Download",
    informActivate: function() {
        var user = this.subject;
        var party = user.party;
        var opponent = user.field.getRandomTarget(1 - party);
        if (opponent.getRawStat(Stat.ATTACK) > opponent.getRawStat(Stat.SPATTACK))
            var stat = Stat.ATTACK;
        else
            var stat = Stat.SPATTACK;
        var effect = new StatChangeEffect(stat, 1);
        effect.silent = true;
        if (user.applyStatus(this.subject, effect)) {
            user.field.print(
                Text.ability_messages(59, user, user.ability, Text.stats_long(stat)));
        }
    }
});

/*******************
 * Anticipation
 *******************/
makeAbility({
    name : "Anticipation",
    informActivate: function() {
        var user = this.subject;
        var party = user.party;
        var found = false;
        for (var i = 0; i < user.field.partySize; i++) {
            if (found) break;
            var opponent = user.field.getActivePokemon(1 - party, i);
            for (var j = 0; j < 4; j++) {
                var move = opponent.getMove(j);
                if ((move == null) || move.power < 1) continue;
                if (user.field.getEffectiveness(move.type, user) > 1) {
                    user.field.print(Text.ability_messages(2, user));
                    found = true;
                    break;
                }
            }
        }
    }
});

/*******************
 * Normalize
 *******************/
makeAbility({
    name: "Normalize",
    transformEffectiveness: function(moveType, type, target) {
        if (target != this.subject)
            return target.field.getEffectiveness(moveType, type);
        else
            return target.field.getEffectiveness(Type.NORMAL, type);
    }
});

/*******************
 * Unburden
 *******************/
makeAbility({
    name: "Unburden",
    informLostItem: function(target) {
        if (target != this.subject)
            return;
        var effect = new StatusEffect("Unburden");
        effect.name = "Unburden";
        effect.statModifier = function(field, stat, subject) {
            if (subject != this.subject)
                return null;
            if (stat != Stat.SPEED)
                return null;
            return [2, 1];
        }
        this.subject.applyStatus(this.subject, effect);
    }
});

/*******************
 * Speed Boost
 *******************/
makeAbility({
    name: "Speed Boost",
    tier: 6,
    subtier: 2,
    tick: function() {
        var eff = new StatChangeEffect(Stat.SPEED, 1);
        eff.silent = true;
        this.subject.applyStatus(this.subject, eff);
        this.subject.field.print(Text.ability_messages(43, this.subject));
    }
});

/*******************
 * Forewarn
 *******************/
makeAbility({
    name: "Forewarn",
    informActivate: function() {
        var party = this.subject.party;
        var move = null;
        for (var i = 0; i < this.subject.field.partySize; i++) {
            var opponent = this.subject.field.getActivePokemon(1 - party, i);
            for (var j = 0; j < 4; j++) {
                var m = opponent.getMove(j);
                if ((move == null) || (m.power > move.power))
                    move = m;
            }
        }
        if (move != null) {
            this.subject.field.print(Text.ability_messages(17, this.subject, move));
        }
    }
});

/*******************
 * Scrappy
 *******************/
makeAbility({
    name: "Scrappy",
    transformEffectiveness: function(moveType, type, target) {
        var exceptions_ = [Type.NORMAL, Type.FIGHTING];
        if ((target != this.subject) && (type == Type.GHOST) && (moveType in exceptions_)) {
            return 1.0;
        }
        return target.field.getTypeEffectiveness(moveType, type);
    }
});

/*******************
 * Truant
 *******************/
makeAbility({
    name: "Truant",
    informActivate: function () {
        this.loaf_ = false;
    },
    vetoExecution: function(field, user, target, move) {
        if (user != this.subject)
            return false;
        if (this.loaf_)
            field.print(Text.ability_messages(51, user));

        return this.loaf_;
    },
    informFinishedSubjectExecution: function() {
        this.loaf_ = !this.loaf_;
    }
});
