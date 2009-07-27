/*
 * File:   hazards.js
 * Author: Catherine
 *
 * Created on July 23, 2009, 4:54 PM
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

const EntryHazard = {
    TOXIC_SPIKES : 0,
    SPIKES : 1,
    STEALTH_ROCK : 2
};

EntryHazard[EntryHazard.SPIKES] = {
    name : Text.field_effects_spikes(9),
    factors_ : [0.125, 0.1875, 0.25],
    addLayer : function(layers, field) {
        if (layers == 3)
            return false;
        field.print(Text.field_effects_spikes(0));
        return true;
    },
    executeEffect : function(layers, field, subject) {
        if (field.getEffectiveness(Type.GROUND, subject) == 0.0)
            return;
        var damage = Math.floor(subject.getStat(Stat.HP)
                * this.factors_[layers - 1]);
        if (damage < 1) damage = 1;
        field.print(Text.field_effects_spikes(3, subject));
        subject.hp -= damage;
    }
};

EntryHazard[EntryHazard.TOXIC_SPIKES] = {
    name : Text.field_effects_spikes(8),
    addLayer : function(layers, field) {
        if (layers == 2)
            return false;
        field.print(Text.field_effects_spikes(2));
        return true;
    },
    executeEffect : function(layers, field, subject) {
        if ((field.getEffectiveness(Type.GROUND, subject) == 0.0)
                || subject.isType(Type.STEEL)
                || subject.getStatus("SubstituteEffect")) {
            return;
        }
        if (subject.isType(Type.POISON)) {
            var effect = getHazardController(subject);
            effect.unapplyHazard(EntryHazard.TOXIC_SPIKES, subject.party);
            field.print(Text.field_effects_spikes(4, subject));
            return;
        }
        if (layers == 1) {
            if (subject.applyStatus(null, new PoisonEffect())) {
                field.print(Text.field_effects_spikes(5, subject));
            }
        } else {
            if (subject.applyStatus(null, new ToxicEffect())) {
                field.print(Text.field_effects_spikes(6, subject));
            }
        }
    }
};

EntryHazard[EntryHazard.STEALTH_ROCK] = {
    name : Text.field_effects_spikes(10),
    addLayer : function(layers, field) {
        if (layers == 1)
            return false;
        field.print(Text.field_effects_spikes(1));
        return true;
    },
    executeEffect : function(layers, field, subject) {
        var factor = field.getEffectiveness(Type.ROCK, subject);
        var damage = Math.floor(subject.getStat(Stat.HP) * factor / 8);
        if (damage < 1) damage = 1;
        field.print(Text.field_effects_spikes(7, subject));
        subject.hp -= damage;
    }
};

makeEffect(StatusEffect, {
    id : "EntryHazardController",
    effects_ : [[], []],
    applyHazard : function(field, hazard, party) {
        var layers = this.effects_[party][hazard] || 0;
        if (!EntryHazard[hazard].addLayer(layers, field))
            return false;
        this.effects_[party][hazard] = layers + 1;
        return true;
    },
    unapplyHazard : function(hazard, party) {
        this.effects_[party][hazard] = 0;
    },
    clearHazards : function(party) {
        this.effects_[party] = [];
    },
    switchIn : function() {
        var effects = this.effects_[this.subject.party];
        for (var i in effects) {
            var layers = effects[i];
            if (layers > 0) {
                EntryHazard[i].executeEffect(layers,
                        this.subject.field, this.subject);
            }
        }
    }
});

function getHazardController(user) {
    var effect = user.getStatus("EntryHazardController");
    if (effect)
        return effect;
    return user.field.applyStatus(new EntryHazardController());
}
