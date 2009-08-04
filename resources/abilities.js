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
Ability.prototype.switchIn = function() {
    if (!this.informActivate)
        return;
    var id_ = "Ability" + this.id + "ActivatedEffect";
    if (this.subject.getStatus(id_))
        return;
    this.informActivate();
    var effect = new StatusEffect(id_);
    this.subject.applyStatus(this.subject, effect);
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

/*******************
 * Levitate
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
})

/*******************
 * Intimidate
 *******************/
makeAbility({
    name : "Intimidate",
    informActivate : function() {
        var field = this.subject.field;
        var party_ = 1 - this.subject.party;
        for (var i = 0; i < field.partySize; ++i) {
            var p = field.getActivePokemon(party_, i);
            if (p) {
                field.print(Text.ability_messages(23, this.subject, p));
                p.applyStatus(null, new StatChangeEffect(Stat.ATTACK, -1));
            }
        }
    },
})

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
makeStatusImmuneAbility("Insomnia", "SleepEffect");

/*******************
 * Vital Spirit
 *******************/
makeStatusImmuneAbility("Vital Spirit", "SleepEffect");

/*******************
 * Water Veil
 *******************/
makeStatusImmuneAbility("Water Veil", "BurnEffect");

/*******************
 * Magma Armor
 *******************/
makeStatusImmuneAbility("Magma Armor", "FreezeEffect");

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
