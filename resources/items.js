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
HoldItem.prototype.condition = function() {
    return false;
};
HoldItem.prototype.informFinishedExecution = function() {
    if (this.condition()) {
        this.use(this.subject);
    }
};
HoldItem.prototype.tier = 10;
HoldItem.prototype.tick = HoldItem.prototype.informFinishedExecution;

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
        condition : function() {
            for (var i in ids) {
                var id = ids[i];
                if (this.subject.getStatus(id)) {
                    return true;
                }
            }
            return false;
        }
    });
}

function makePinchBerry(item, effect) {
    makeItem({
        name : item,
        berry_ : true,
        use : effect,
        condition : function() {
            var d = this.subject.sendMessage("informPinchBerry");
            if (!d) d = 4;
            var threshold = Math.floor(this.subject.getStat(Stat.HP) / d);
            return (this.subject.hp <= threshold);
        }
    });
}

function makeFlavourHealingBerry(item, stat) {
    makeItem({
        name : item,
        berry_ : true,
        condition : function() {
            var threshold = Math.floor(this.subject.getStat(Stat.HP) / 2);
            return (this.subject.hp <= threshold);
        },
        use : function(user) {
            user.field.print(Text.item_messages(0, user, this));
            var delta = Math.floor(user.getStat(Stat.HP) / 8);
            user.hp += delta;
            if (user.getNatureEffect(stat) < 1.0) {
                user.field.print(Text.item_messages(4, user));
                user.applyStatus(user, new ConfusionEffect());
            }
            this.consume();
        }
    })
}

function makeStatBoostBerry(item, stat) {
    makePinchBerry(item, function(user) {
        if (user.getStatLevel(stat) == 6)
            return;
        var effect = new StatChangeEffect(stat, 1);
        effect.silent = true;
        if (!user.applyStatus(user, effect))
            return;
        user.field.print(Text.item_messages(3, user, this,
                Text.stats_long(stat)));
        this.consume();
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

function makeChoiceItem(item, func) {
    makeItem({
        name : item,
        choiceItem_ : true,
        statModifier : func,
        informFinishedSubjectExecution : function() {
            var move_ = this.subject.lastMove;
            if (!move_ || (move_.name == "Mimic") || (move_.name == "Sketch")
                    || (move_.name == "Transform")
                    || (this.subject.getPp(move_) == -1)
                    || this.subject.getStatus("ChoiceLockEffect"))
                return;
            var effect = new StatusEffect("ChoiceLockEffect");
            effect.vetoSelection = function(user, move) {
                if (user != this.subject)
                    return false;
                var item_ = user.item;
                if (!item_ || !item_.choiceItem_
                        || (item_.getState() != StatusEffect.STATE_ACTIVE)) {
                    this.subject.removeStatus(this);
                    return false;
                }
                return (move.name != move_.name);
            };
            this.subject.applyStatus(this.subject, effect);
        }
    });
}

makeChoiceItem("Choice Band", function(field, stat, subject) {
    if (subject != this.subject)
        return null;
    if (stat != Stat.ATTACK)
        return null;
    return [1.5, 3];
});

makeChoiceItem("Choice Specs", function(field, stat, subject) {
    if (subject != this.subject)
        return null;
    if (stat != Stat.SPATTACK)
        return null;
    return [1.5, 3];
});

makeChoiceItem("Choice Scarf", function(field, stat, subject) {
    if (subject != this.subject)
        return null;
    if (stat != Stat.SPEED)
        return null;
    return [1.5, 2];
});

makeItem({
    name : "Quick Claw",
    value_ : -1,
    switchIn : function() {
        this.value_ = -1;
    },
    switchOut : function() {
        this.value_ = -1;
        return false;
    },
    applyEffect : function() {
        this.value_ = -1;
        return true;
    },
    inherentPriority : function() {
        if (this.value_ == -1) {
            this.value_ = this.subject.field.random(0.2) ? 3 : 0;
        }
        return this.value_;
    },
    informBeginExecution : function() {
        if (this.value_ > 0) {
            this.subject.field.print(Text.item_messages(2, this.subject));
        }
    },
    informFinishedSubjectExecution : function() {
        this.value_ = -1;
    }
});

makeStatBoostBerry("Liechi Berry", Stat.ATTACK);
makeStatBoostBerry("Ganlon Berry", Stat.DEFENCE);
makeStatBoostBerry("Salac Berry", Stat.SPEED);
makeStatBoostBerry("Petaya Berry", Stat.SPATTACK);
makeStatBoostBerry("Apicot Berry", Stat.SPDEFENCE);

makeFlavourHealingBerry("Figy Berry", Stat.ATTACK);
makeFlavourHealingBerry("Wiki Berry", Stat.SPATTACK);
makeFlavourHealingBerry("Mago Berry", Stat.SPEED);
makeFlavourHealingBerry("Aguav Berry", Stat.SPDEFENCE);
makeFlavourHealingBerry("Iapapa Berry", Stat.DEFENCE);

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
        this.subject.field.print(Text.item_messages(0, this, this.subject));
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

