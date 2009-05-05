/*
 * File:   moves.js
 * Author: Catherine
 *
 * Created on April 16 2009, 4:10 PM
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
 * Make a move into a counter move.
 */
function makeCounterMove(move, cls, ratio) {
    move.use = function(field, user) {
        var recent = null;
        while ((recent = user.popRecentDamage()) != null) {
            var move = recent[1];
            if ((cls == undefined) || (move.moveClass == cls)) {
                var target = recent[0];
                if (target.party == user.party) {
                    break;
                }
                // todo: is fainted?
                if (target.isImmune(move)) {
                    field.print(Text.battle_messages(1, target));
                } else {
                    target.hp -= recent[2] * ratio;
                }
                return;
            }
        }

        // if we get here, the move failed
        field.print(Text.battle_messages(0));
    };
}

/**
 * Make a move object into a StatusMove with an array of elements of the form
 * [effect, chance, user].
 */
function makeStatusMove(move, effects, immunities) {
    if (immunities == undefined) {
        immunities = false;
    }
    move.use = function(field, user, target, targets) {
        if (this.power != 0) {
            var damage = field.calculate(this, user, target, targets);
            if (damage == 0) {
                return;
            }
            target.hp -= damage;
        } else if (immunities) {
            if (target.isImmune(this)) {
                field.print(Text.battle_messages(1, target));
                return;
            }
        }
        var serene = user.hasAbility("Serene Grace");
        var immune = (target.hasAbility("Shield Dust") && (this.power != 0));
        for (var i = 0; i < effects.length; ++i) {
            var effect = effects[i];
            if (effect.length == 1) {
                effect[1] = 1.00;
                effect[2] = false;
            } else if (effect.length == 2) {
                effect[2] = false;
            }
            if (!effect[2] && immune) {
                continue;
            }
            var chance = effect[1];
            if (serene) {
                chance *= 2;
            }
            if (field.random(chance)) {
                var affected = effect[2] ? user : target;
                var e = effect[0];
                var obj = (typeof(e) == "function") ? new e() : e;
                if (affected.applyStatus(user, obj) == null) {
                    if (this.power == 0) {
                        // inform failure
                        obj.informFailure(affected);
                    }
                }
            }
        }
    };
}

