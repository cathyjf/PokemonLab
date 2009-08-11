/*
 * File:   items.js
 * Author: Catherine
 *
 * Created on June 2, 2009, 12:18 AM
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

function HoldItem(name) {
    this.name = this.id = name;
    HoldItem[name] = this;
}

HoldItem.prototype = new StatusEffect();
HoldItem.prototype.switchOut = function() {
    return false;
};
HoldItem.prototype.use = function() { };
HoldItem.prototype.consume = function() {
    if (this.subject.fainted)
        return;
    var effect = this.subject.getStatus("ConsumedItemEffect");
    if (!effect) {
        effect = new StatusEffect("ConsumedItemEffect");
        var party_ = this.subject.party;
        var position_ = this.subject.position;
        effect.applyEffect = function() {
            if (this.subject.party != party_)
                return false;
            if (this.subject.position != position_)
                return false;
            return true;
        };
        effect = this.subject.field.applyStatus(effect);
    }
    effect.item_ = this.id;
    this.subject.removeStatus(this);
};
HoldItem.prototype.getState = function() {
    if (!this.subject || (this.state != StatusEffect.STATE_ACTIVE))
        return this.state;
    if (this.subject.getStatus("EmbargoEffect")
            || this.subject.hasAbility("Klutz"))
        return StatusEffect.STATE_DEACTIVATED;
    return StatusEffect.STATE_ACTIVE;
};

function makeItem(obj) {
    var item = new HoldItem(obj.name);
    for (var p in obj) {
        item[p] = obj[p];
    }
}

function makeEvadeItem(item) {
    makeItem({
        name : item,
        statModifier : function(field, stat, subject, target) {
            if (stat != Stat.ACCURACY)
                return null;
            if (target != this.subject)
                return null;
            return [0.9, 8];
        }
    });
}

// TODO: Also remove statuses in tick().
function makeStatusCureItem(item, ids) {
    makeItem({
        name : item,
        berry_ : true,
        use : function(user) {
            ids.forEach(function(id) {
                var effect = user.getStatus(id);
                if (effect) {
                    user.field.print(Text.item_messages(1, user, this, effect));
                    user.removeStatus(effect);
                }
            }, this);
            this.consume();
        },
        applyEffect : function() {
            for (var i in ids) {
                var id = ids[i];
                if (this.subject.getStatus(id)) {
                    this.use(this.subject);
                    break;
                }
            }
        },
        informEffectApplied : function(effect) {
            if (ids.indexOf(effect.id) != -1) {
                this.use(this.subject);
            }
        }
    });
}

function makeTypeBoostingItem(item, type) {
    makeItem({
        name : item,
        type_ : type,
        modifier : function(field, user, target, move, critical) {
            if (user != this.subject)
                return null;
            if (move.type != type)
                return null;
            return [0, 1.2, 1];
        }
    });
}

function makePlateItem(item, type) {
    makeTypeBoostingItem(item, type);
    HoldItem[item].plate_ = true;
}

makeTypeBoostingItem("SilverPowder", Type.BUG);
makeTypeBoostingItem("Metal Coat", Type.STEEL);
makeTypeBoostingItem("Soft Sand", Type.GROUND);
makeTypeBoostingItem("Hard Stone", Type.ROCK);
makeTypeBoostingItem("Miracle Seed", Type.GRASS);
makeTypeBoostingItem("BlackGlasses", Type.DARK);
makeTypeBoostingItem("Black Belt", Type.FIGHTING);
makeTypeBoostingItem("Magnet", Type.ELECTRIC);
makeTypeBoostingItem("Mystic Water", Type.WATER);
makeTypeBoostingItem("Sharp Beak", Type.FLYING);
makeTypeBoostingItem("Poison Barb", Type.POISON);
makeTypeBoostingItem("NeverMeltIce", Type.ICE);
makeTypeBoostingItem("Spell Tag", Type.GHOST);
makeTypeBoostingItem("TwistedSpoon", Type.PSYCHIC);
makeTypeBoostingItem("Charcoal", Type.FIRE);
makeTypeBoostingItem("Dragon Fang", Type.DRAGON);
makeTypeBoostingItem("Silk Scarf", Type.NORMAL);

makePlateItem("Flame Plate", Type.FIRE);
makePlateItem("Splash Plate", Type.WATER);
makePlateItem("Zap Plate", Type.ELECTRIC);
makePlateItem("Meadow Plate", Type.GRASS);
makePlateItem("Icicle Plate", Type.ICE);
makePlateItem("Fist Plate", Type.FIGHTING);
makePlateItem("Toxic Plate", Type.POISON);
makePlateItem("Earth Plate", Type.GROUND);
makePlateItem("Sky Plate", Type.FLYING);
makePlateItem("Mind Plate", Type.PSYCHIC);
makePlateItem("Insect Plate", Type.BUG);
makePlateItem("Stone Plate", Type.ROCK);
makePlateItem("Spooky Plate", Type.GHOST);
makePlateItem("Draco Plate", Type.DRAGON);
makePlateItem("Dread Plate", Type.DARK);
makePlateItem("Iron Plate", Type.STEEL);

makeStatusCureItem("Cheri Berry", ["ParalysisEffect"]);
makeStatusCureItem("Chesto Berry", ["SleepEffect"]);
makeStatusCureItem("Pecha Berryy", ["PoisonEffect", "ToxicEffect"]);
makeStatusCureItem("Rawst Berry", ["BurnEffect"]);
makeStatusCureItem("Aspear Berry", ["FreezeEffect"]);
makeStatusCureItem("Persim Berry", ["ConfusionEffect"]);
makeStatusCureItem("Lum Berry", ["ParalysisEffect", "SleepEffect",
        "PoisonEffect", "ToxicEffect", "BurnEffect", "FreezeEffect",
        "ConfusionEffect"]);

makeEvadeItem("Brightpowder");
makeEvadeItem("Lax Incense");

makeItem({
    name : "Light Clay",
    informPartyBuffTurns : function() {
        return 8;
    }
});

makeItem({
    name : "Shed Shell",
    informBlockSwitch : function() {
        return true;
    }
});

makeItem({
    name : "Grip Claw",
    informTemporaryTrapping : function() {
        return 6;
    }
});

makeItem({
    name : "Leftovers",
    tier : 6,
    subtier : 2,
    tick : function() {
        var max = this.subject.getStat(Stat.HP);
        if (this.subject.hp == max)
            return;
        this.subject.field.print(Text.item_messages(0, this.subject));
        var delta = Math.floor(max / 16);
        this.subject.hp += delta;
    }
});

makeItem({
    name : "Lagging Tail",
    inherentPriority : function() {
        return -2;
    }
});

