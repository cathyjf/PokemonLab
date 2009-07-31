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
    this.use();
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

function makeStatusCureItem(item, ids) {
    makeItem({
        name : item,
        use : function() {
            ids.forEach(function(id) {
                var effect = this.subject.getStatus(id);
                if (effect) {
                    this.subject.field.print(
                            Text.item_messages(1, this.subject, this, effect));
                    this.subject.removeStatus(effect);
                }
            }, this);
        },
        applyEffect : function() {
            for (var i in ids) {
                var id = ids[i];
                if (this.subject.getStatus(id)) {
                    this.consume();
                    break;
                }
            }
        },
        informEffectApplied : function(effect) {
            if (ids.indexOf(effect.id) != -1) {
                this.consume();
            }
        }
    });
}

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

