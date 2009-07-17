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
 * Make a move that causes the user and target to simultaneously adopt each
 * other's stat levels for the specified stats.
 */
function makeStatLevelSwapMove(move, stats) {
    move.use = function(field, user, target, targets) {
        stats.forEach(function(stat) {
            var delta = target.getStatLevel(stat) - user.getStatLevel(stat);
            // target takes -delta, user takes +delta
            if (delta == 0)
                return;
            var effect = new StatChangeEffect(stat, -delta);
            effect.silent = true;
            target.applyStatus(user, effect);

            effect = new StatChangeEffect(stat, delta);
            effect.silent = true;
            user.applyStatus(user, effect);
        });
        // TODO: Perhaps this message should specify the target.
        field.print(Text.battle_messages_unique(6));
    };
}

/**
 * Make a move that heals a different amount depending on the weather.
 *
 * TODO: WARNING! The mechanics of this move were MADE UP by me! It is unknown
 *       which weather takes precedence when multiple weathers are present. I
 *       assume that sun takes precedence over any other weather, and that
 *       any combination of weathers takes precedence over no weather.
 */
function makeWeatherBasedHealingMove(move) {
    move.use = function(field, user, target, targets) {
        var max = user.getStat(Stat.HP);
        if (user.hp == max) {
            field.print(Text.battle_messages(0));
            return;
        }
        var flags = getGlobalController(user).flags;
        var delta = 0;
        if (flags[GlobalEffect.SUN]) {
            delta = Math.floor(max * 2 / 3);
        } else if (flags[GlobalEffect.RAIN] || flags[GlobalEffect.HAIL] ||
                flags[GlobalEffect.GRAVITY] || flags[GlobalEffect.FOG] ||
                flags[GlobalEffect.SAND] || flags[GlobalEffect.TRICK_ROOM]) {
            delta = Math.floor(max / 4);
        } else {
            delta = Math.floor(max / 2);
        }
        user.hp += delta;
    };
}

/**
 * Make a move that summons weather.
 */
function makeWeatherMove(move, idx) {
    move.prepareSelf = function(field, user) {
        var effect = getGlobalController(user);
        if (!effect.applyWeather(user, idx)) {
            field.print(Text.battle_messages(0));
        }
    };
    move.use = function() {
        // Does nothing.
    };
}

/**
 * Make a move that weakens attacks of a particular type while the user of the
 * move remains active.
 */
function makeTypeWeakeningMove(move, type) {
    var id = "TypeWeakeningEffect" + type;
    move.prepareSelf = function(field, user) {
        if (user.getStatus(id)) {
            field.print(Text.battle_messages(0));
            return;
        }
        var effect = new StatusEffect(id);
        effect.modifier = function(field, user, target, move, critical) {
            if (move.type != type)
                return null;
            return [0, 0.5, 3];
        };
        user.applyStatus(user, effect);
        field.print(Text.battle_messages_unique(35, Text.types(type)));
    };
    move.use = function() {
        // Does nothing.
    };
}

/**
 * Make a move that cures every target of special status effects and confusion.
 */
function makeStatusCureMove(move) {
    move.use = function(field, user, target, targets) {
        var effect = target.getStatus(StatusEffect.SPECIAL_EFFECT);
        if (effect) {
            field.print(Text.battle_messages_unique(1, target, effect));
            target.removeStatus(effect);
        }
        effect = target.getStatus("ConfusionEffect");
        if (effect) {
            field.print(Text.battle_messages_unique(1, target, effect));
            target.removeStatus(effect);
        }
    };
}

/**
 * Make a move that doubles in power if the target was the last non-user to
 * hit to the user with an attack (other than Pain Split).
 */
function makeRevengeMove(move) {
    var power_ = move.power;
    move.use = function(field, user, target, targets) {
        this.power = power_;
        var recent = null;
        while ((recent = user.popRecentDamage()) != null) {
            if (recent[1].name == "Pain Split")
                continue;
            if (recent[0] == target) {
                this.power *= 2;
            }
            break;
        }
        target.hp -= field.calculate(this, user, target, targets);
    };
}

/**
 * Make a move that causes the target's evasiveness stat to become zero until
 * it is replaced, as well as making the target vulnerable to the given types.
 */
function makeForesightMove(move, types) {
    move.use = function(field, user, target, targets) {
        field.print(Text.battle_messages_unique(84, user, target));
        if (!target.getStatus("ForesightEffect")) {
            var effect = new StatusEffect("ForesightEffect");
            effect.transformStatLevel = function(user, target, stat, level) {
                if (target != this.subject)
                    return null;
                if (stat != Stat.EVASION)
                    return null;
                if (level <= 0)
                    return null;
                return [0, true];
            };
            target.applyStatus(user, effect);
        }
        types.forEach(function(type) {
            var id = "VulnerabilityEffect" + type;
            if (target.getStatus(id))
                return;
            var effect = new StatusEffect(id);
            effect.vulnerability = function(user, target) {
                if (target != this.subject)
                    return -1;
                return type;
            };
            target.applyStatus(user, effect);
        });
    };
}

/**
 * Make a move that makes the next move to target the target of this move
 * always hit. The effect lasts for two turns, including the present turn,
 * or until the target leaves the field.
 *
 * TODO: Major research is required here. Does Lock On make the next move to
 *       target the subject hit; the next move to target the subject on the
 *       next turn; all moves targeting the subject on the turn turn; or some
 *       other possibility?
 */
function makeLockOnMove(move) {
    move.use = function(field, user, target, targets) {
        if (target.getStatus("LockOnEffect")) {
            field.print(Text.battle_messages(0));
            return;
        }
        var effect = new StatusEffect("LockOnEffect");
        effect.tier = 0;
        effect.turns = 2;
        effect.tick = function() {
            if (--this.turns <= 0) {
                this.subject.removeStatus(this);
            }
        };
        effect.statModifier = function(field, stat, subject, target) {
            if (stat != Stat.ACCURACY)
                return null;
            if (target != this.subject)
                return null;
            this.subject.removeStatus(this);
            return [0, 13];
        };
        target.applyStatus(user, effect);
        field.print(Text.battle_messages_unique(141, user, target));
    };
}

/**
 * Make a move that causes the user to switch items with the target.
 */
function makeItemSwitchMove(move) {
    move.use = function(field, user, target, targets) {
        if (!user.item && !target.item) {
            field.print(Text.battle_messages(0));
            return;
        }
        if (user.sendMessage("informRemoveItem"))
            return;
        if (target.sendMessage("informRemoveItem"))
            return;
        [user.item, target.item] = [target.item, user.item];
        if (user.item) {
            field.print(Text.battle_messages_unique(13, user, user.item));
        }
        if (target.item) {
            field.print(Text.battle_messages_unique(13, target, target.item));
        }
    };
}

/**
 * Make a move that steals the target's item.
 */
function makeThiefMove(move) {
    move.use = function(field, user, target, targets) {
        var damage = field.calculate(this, user, target, targets);
        if (!damage)
            return;
        target.hp -= damage;
        if (!user.item && target.item
                && !target.sendMessage("informRemoveItem")) {
            field.print(Text.battle_messages_unique(10, user, target,
                    target.item));
            user.item = target.item;
            target.item = null;
        }
    };
}

/**
 * Make a move that has a chance of boosting all stats.
 */
function makeAllStatBoostMove(move) {
    var effect = new StatusEffect("AllStatBoostEffect");
    effect.applyEffect = function() {
        for (var i = 1; i < 6; ++i) {
            this.subject.applyStatus(this.subject, new StatChangeEffect(i, 1));
        }
        return false;
    };
    makeStatusMove(move, [[effect, 0.1, true]]);
}

/**
 * Make a move into a mass-based move.
 */
function makeMassBasedMove(move) {
    move.use = function(field, user, target, targets) {
        var mass = target.mass;
        if (mass <= 10) {
            this.power = 20;
        } else if (mass <= 25) {
            this.power = 40;
        } else if (mass <= 50) {
            this.power = 60;
        } else if (mass <= 100) {
            this.power = 80;
        } else if (mass <= 200) {
            this.power = 100;
        } else {
            this.power = 120;
        }
        target.hp -= field.calculate(this, user, target, targets);
    };
}

/**
 * Make a move into a delayed attack move. A delayed move deals damage two
 * turns after the turn it was used.
 */
function makeDelayedAttackMove(move) {
    move.attemptHit = function() {
        return true;
    };
    move.use = function(field, user, target, targets) {
        if (target.getStatus("DelayedAttack")) {
            field.print(Text.battle_messages(0));
            return;
        }

        // Calculate the damage that the attack will do.
        // TODO: Make sure that this is not affected by Life Orb.
        var type_ = this.type;
        this.type = Type.TYPELESS;
        var damage = field.calculate(this, user, target, targets);
        this.type = type_;

        // Create an effect to deal the damage.
        var effect = new StatusEffect("DelayedAttack");
        effect.turns = 3;
        effect.tier = 7;
        effect.subtier = 0;
        effect.party_ = target.party;
        effect.position_ = target.position;
        effect.applyEffect = function() {
            if (this.subject.party != this.party_)
                return false;
            if (this.subject.position != this.position_)
                return false;
            return true;
        };
        effect.beginTick = function() {
            if (--this.turns < 0) {
                field.removeStatus(this);
            }
        };
        effect.tick = function() {
            if (this.turns != 0)
                return;
            field.print(Text.battle_messages_unique(95, this.subject));
            if (field.attemptHit(move, user, this.subject)) {
                this.subject.hp -= damage;
            } else {
                field.print(Text.battle_messages(2, user, this.subject));
            }
            field.removeStatus(this);
        };
        field.applyStatus(effect);
        field.print(Text.battle_messages_unique(94, user));
    };
}

/**
 * Return whether two pokemon have opposite genders.
 */
function isOppositeGender(user, target) {
    if (target.sendMessage("informGenderBased"))
        return false;
    if (user.gender == Gender.NONE)
        return false;
    if (target.gender == Gender.NONE)
        return false;
    return (user.gender != target.gender);
}

/**
 * Make a move into a move that depends on the user's health being low.
 */
function makeUserLowHealthMove(move) {
    move.use = function(field, user, target, targets) {
        var n = Math.floor(user.hp * 64 / user.getStat(Stat.HP));
        if (n < 2) {
            this.power = 200;
        } else if (n < 6) {
            this.power = 150;
        } else if (n < 13) {
            this.power = 100;
        } else if (n < 22) {
            this.power = 80;
        } else if (n < 43) {
            this.power = 40;
        } else {
            this.power = 20;
        }
        target.hp -= field.calculate(this, user, target, targets);
    };
}

/**
 * Make a move into a move that depends on the user's health being high.
 */
function makeUserHealthMove(move) {
    move.use = function(field, user, target, targets) {
        this.power = Math.floor(user.hp * 150 / user.getStat(Stat.HP));
        if (this.power == 0) {
            this.power = 1;
        } else if (this.power > 150) {
            this.power = 150;
        }
        target.hp -= field.calculate(this, user, target, targets);
    };
}

/**
 * Make a move into a move that depends on the target's health.
 */
function makeTargetHealthMove(move) {
    move.use = function(field, user, target, targets) {
        this.power = Math.floor(target.hp * 120 / target.getStat(Stat.HP)) + 1;
        target.hp -= field.calculate(this, user, target, targets);
    };
}

/**
 * Make a move into an explosion move. For the purpose of an explosion move,
 * the target's defence stat is temporarily halved.
 */
function makeExplosionMove(move) {
    move.prepareSelf = function(field, user) {
        if (field.sendMessage("informExplosion")) {
            return;
        }
        user.faint();
    };
    move.use = function(field, user, target, targets) {
        var effect = new StatusEffect("ExplosionEffect");
        effect.statModifier = function(field, stat, subject) {
            if (subject != this.subject)
                return null;
            if (stat != Stat.DEFENCE)
                return null;
            return [0.5, 4];
        };
        effect = target.applyStatus(user, effect);
        if (field.partySize > 1) {
            targets = 2;
        }
        target.hp -= field.calculate(this, user, target, targets);
        target.removeStatus(effect);
    };
}

/**
 * Make a move into a rampage move.
 */
function makeRampageMove(move) {
    move.prepareSelf = function(field, user) {
        if (user.getStatus("SleepEffect"))
            return;
        
        user.setForcedMove(this, null);

        if (user.getStatus("RampageEffect"))
            return;

        var effect = new StatusEffect("RampageEffect");
        effect.turns = field.random(2, 3);
        effect.vetoSwitch = function(subject) {
            return (subject == this.subject);
        };
        effect.unapplyEffect = function() {
            this.subject.clearForcedMove();
        };
        effect.informFreeze = effect.informSleep = function() {
            this.subject.removeStatus(this);
        };
        effect.informFinishedExecution = function() {
            if (!this.subject.lastMove) {
                // Subject was prevented from using the move, so the effect
                // does not continue.
                this.subject.removeStatus(this);
            } else if (--this.turns == 0) {
                var user = this.subject;
                if (!user.getStatus("ConfusionEffect")) {
                    field.print(Text.battle_messages_unique(97, user));
                    user.applyStatus(user, new ConfusionEffect());
                }
                this.subject.removeStatus(this);
            }
        };
        user.applyStatus(user, effect);
    };
    move.use = function(field, user, target, targets) {
        var damage = field.calculate(this, user, target, targets);
        if (damage == 0) {
            var effect = user.getStatus("RampageEffect");
            if (effect) {
                user.removeStatus(effect);
            }
            return;
        }
        target.hp -= damage;
    };
}

/**
 * Make a move into a party buff move. A party buff move decreases the
 * effectiveness of attacks against the party, either physical attacks or
 * special attacks, for five turns. Critical hits ignore this boost.
 */
function makePartyBuffMove(move, moveClass) {
    var id = "BuffEffect" + moveClass;
    move.prepareSelf = function(field, user) {
        if (user.getStatus(id)) {
            field.print(Text.battle_messages(0));
            return;
        }
        var effect = new StatusEffect(id);
        var turns = user.sendMessage("informPartyBuffTurns");
        effect.turns = turns ? turns : 5;
        effect.party_ = user.party;
        effect.applyEffect = function() {
            return (this.subject.party == this.party_);
        };
        effect.beginTick = function() {
            if (--this.turns != 0)
                return;
            // TODO: Fix this message to reference the trainer's name.
            field.print(Text.battle_messages_unique(80, move));
            this.subject.field.removeStatus(this);
        };
        effect.modifier = function(field, user, target, move, critical) {
            if (target.party != this.party_)
                return null;
            if (move.moveClass != moveClass)
                return null;
            if (critical)
                return null;
            // Determine the number of active pokemon on the buffed team.
            var active = 0;
            for (var i = 0; i < field.partySize; ++i) {
                if (field.getActivePokemon(this.party_, i)) {
                    ++active;
                }
            }
            var m = 1 - 1.0 / (active + 1);
            // Multiplier in Mod1; priority of 1 (i.e. second position).
            return [1, m, 1];
        };
        field.print(Text.battle_messages_unique(79));
        field.applyStatus(effect);
    };
    move.use = function() {
        // Does nothing.
    };
}

/**
 * Make a move into a OHKO move. A OHKO move deals damage equal to the target's
 * current HP. Whether the target has a substitute has no effect no the damage
 * that a OHKO move deals.
 */
function makeOneHitKillMove(move) {
    move.attemptHit = function(field, user, target) {
        if (user.level < target.level)
            return true;
        var p = (user.level - target.level + 30) / 100;
        return field.random(p);
    };
    move.use = function(field, user, target, targets) {
        if (user.level < target.level) {
            field.print(Text.battle_messages(0));
            return;
        }
        if (target.isImmune(this)) {
            field.print(Text.battle_messages(1, target));
            return;
        }
        target.hp = 0;
        field.print(Text.battle_messages_unique(48));
    };
}

/**
 * Make a move into a (full) trapping move.
 */
function makeTrappingMove(move) {
    move.use = function(field, user, target, targets) {
        if (target.getStatus("TrappingEffect")) {
            field.print(Text.battle_messages(0));
            return;
        }
        var effect = new StatusEffect("TrappingEffect");
        effect.vetoSwitch = function(subject) {
            if (subject != this.subject)
                return false;
            if (subject.sendMessage("informBlockSwitch"))
                return false;
            return true;
        };
        effect.informWithdraw = function(subject) {
            if (subject == this.inducer) {
                this.subject.removeStatus(this);
            }
        };
        field.print(Text.battle_messages_unique(76, target));
        target.applyStatus(user, effect);
    };
}

/**
 * Make a move into a temporary trapping move.
 */
function makeTemporaryTrappingMove(move, text) {
    move.use = function(field, user, target, targets) {
        var damage = field.calculate(this, user, target, targets);
        if (damage == 0)
            return;
        target.hp -= damage;
        if (target.getStatus("TemporaryTrappingEffect"))
            return;
        var effect = new StatusEffect("TemporaryTrappingEffect");
        effect.tier = 6;
        effect.subtier = 7;
        var turns = user.sendMessage("informTemporaryTrapping");
        if (!turns) {
            turns = field.random(3, 6);
        }
        effect.turns = turns;
        effect.tick = function() {
            if (--this.turns == 0) {
                field.print(Text.battle_messages_unique(
                        53, this.subject, move));
                this.subject.removeStatus(this);
                return;
            }
            field.print(Text.battle_messages_unique(54, this.subject, move));
            var damage = Math.floor(this.subject.getStat(Stat.HP) / 16);
            if (damage < 1) damage = 1;
            this.subject.hp -= damage;
        };
        effect.vetoSwitch = function(subject) {
            if (subject != this.subject)
                return false;
            if (subject.sendMessage("informBlockSwitch"))
                return false;
            return true;
        };
        effect.informWithdraw = function(subject) {
            if (subject == this.inducer) {
                this.subject.removeStatus(this);
            }
        };
        field.print(text(target));
        target.applyStatus(user, effect);
    };
}

/**
 * Make a move into a recharge move.
 */
function makeRechargeMove(move) {
    var execute = getParentUse(move);
    move.use = function(field, user, target, targets) {
        execute.call(this, field, user, target, targets);
        user.setForcedMove(this, target);
        var effect = new StatusEffect("RechargeMoveEffect");
        effect.vetoTier = -5;
        effect.turns = 2;
        effect.informFinishedExecution = function() {
            if (--this.turns == 0) {
                this.subject.removeStatus(this);
            }
        };
        effect.vetoSwitch = function(subject) {
            return (subject == this.subject);
        };
        effect.vetoExecution = function(field, user, target, move) {
            if (target != null)
                return false;
            if (user != this.subject)
                return false;
            field.print(Text.battle_messages_unique(57, user));
            return true;
        };
        user.applyStatus(user, effect);
    };
}

/**
 * Make a move into a two hit move.
 */
function makeTwoHitMove(move) {
    var execute = getParentUse(move);
    move.use = function(field, user, target, targets) {
        var hits = 0;
        for (var i = 0; i < 2; ++i) {
            if (target.fainted)
                break;
            ++hits;
            execute.call(this, field, user, target, targets);
        }
        field.print(Text.battle_messages_unique(0, hits));
    };
}

/**
 * Make a move into a multiple hit move, which hits between 2 and 5 times,
 * with a nonuniform distribution. In particular, there is a 37.5% chance of
 * 2 hits, a 37.5% chance of 3 hits, a 12.5% chance of 4 hits, and a 12.5%
 * chance of 5 hits.
 */
function makeMultipleHitMove(move) {
    move.use = function(field, user, target, targets) {
        var hits = user.sendMessage("informMultipleHitMove");
        if (!hits) {
            var rand = field.random(0, 1000);
            if (rand < 375) {
                hits = 2;
            } else if (rand < 750) {
                hits = 3;
            } else if (rand < 875) {
                hits = 4;
            } else {
                hits = 5;
            }
        }
        var i = 0;
        for (; i < hits; ++i) {
            if (target.fainted)
                break;
            target.hp -= field.calculate(this, user, target, targets);
        }
        field.print(Text.battle_messages_unique(0, i));
    };
}

/**
 * Make a move into a recovery move.
 */
function makeRecoveryMove(move, ratio) {
    if (ratio == undefined) {
        ratio = 0.5;
    }
    move.use = function(field, user) {
        var max = user.getStat(Stat.HP);
        if (max == user.hp) {
            field.print(Text.battle_messages(0));
            return;
        }
        user.hp += Math.floor(max * ratio);
    };
}

/**
 * Make a move into a recoil move with an arbitrary ratio of recoil. Note that
 * absorb moves are a special case of recoil moves where the ratio is a
 * negative number.
 */
function makeRecoilMove(move, divisor) {
    var execute = getParentUse(move);
    move.use = function(field, user, target, targets) {
        var effective = target.sendMessage("getEffectiveHp");
        var present = effective ? effective[0] : target.hp;
        execute.call(this, field, user, target, targets);

        // Calculate the relevant health change (delta).
        var delta = 0;
        if (effective) {
            var max = effective[1];
            effective = target.sendMessage("getEffectiveHp");
            var after = effective ? effective[0] : target.hp;
            delta = present - after;
            if (delta > effective[1]) {
                delta = effective[1];
            }
        } else {
            delta = present - target.hp;
            var max = target.getStat(Stat.HP);
            if (delta > max) {
                delta = max;
            }
        }
        
        var sgn = (divisor > 0) ? 1 : -1;
        var recoil = sgn * Math.floor(Math.abs(delta / divisor));
        if (user.sendMessage("informRecoilDamage", recoil, move))
            return;

        if (recoil > 0) {
            field.print(Text.battle_messages_unique(58, user));
        } else if (recoil < 0) {
            field.print(Text.battle_messages_unique(59, user));
        }
        user.hp -= recoil;
    };
}

/**
 * Make a move into a charge move. A charge move is selected once and executes
 * twice; the second time is a forced move. The position to attack is fixed
 * when the move is selected. If the final parameter is not undefined then the
 * move also makes the user invulnerable to all moves except those named in
 * the array passed as the final parameter.
 *
 * NOTE (TODO): It should not be possible for the charge turn to be stopped
 *              by an immunity veto (target != null).
 */
function makeChargeMove(move, text, vulnerable) {
    var execute = getParentUse(move);
    var accuracy_ = move.accuracy;
    move.accuracy = 0;
    move.charge_ = true;
    move.use = function(field, user, target, targets) {
        var effect = user.getStatus("ChargeMoveEffect");
        if (effect) {
            if (effect.turns == 2) {
                // We get here if the move has more than one target and this
                // is the charge turn; hence, we don't need to do anything.
                return;
            }
            // The charge turn has already been completed, so we proceed to
            // execute the move.
            execute.call(this, field, user, target, targets);
            return;
        }

        // Otherwise, the user spends his turn charging.
        field.print(text(user));
        if (this.additional) {
            this.additional(user);
        }
        var move_ = user.setForcedMove(this, target);
        move_.accuracy = accuracy_;
        effect = new StatusEffect("ChargeMoveEffect");
        effect.move = this;
        effect.turns = 2;
        effect.informFinishedExecution = function() {
            if (--this.turns == 0) {
                this.subject.removeStatus(this);
            }
        };
        effect.vetoSwitch = function(subject) {
            return (subject == this.subject);
        };
        if (vulnerable != undefined) {
            // TODO: vetoTier?
            // This effect also makes the user immune to most moves.
            effect.vetoExecution = function(field, user, target, move) {
                if (target != this.subject)
                    return false;
                if (vulnerable.indexOf(move.name) != -1)
                    return false;
                // TODO: Replace this by something wherein the charge turn
                //       does not get a chance to be vetoed.
                if (move.charge_ && (move.accuracy == 0))
                    return false;
                var tc = move.targetClass;
                if ((tc == Target.ALL) || (tc == Target.USER_OR_ALLY))
                    return false;
                field.print(Text.battle_messages(2, user, target));
                return true;
            };
        }
        user.applyStatus(user, effect);
    };
}

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
                if (target.fainted)
                    return;
                if (target.isImmune(this)) {
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
            if (this.prepare) {
                this.prepare(field, user, target, targets);
            }
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
        var immune = (target &&
            target.hasAbility("Shield Dust") && (this.power != 0));
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
                if (affected.fainted) {
                    continue;
                }
                var e = effect[0];
                if (e.delayed) {
                    e = e(field);
                }
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

/**
 * Get the use function of an arbitrary move, or a basic use function if the
 * move has no use function.
 */
function getParentUse(move) {
    return move.use || function(field, user, target, targets) {
            // Just do a basic move use.
            target.hp -= field.calculate(this, user, target, targets);
        };
}

