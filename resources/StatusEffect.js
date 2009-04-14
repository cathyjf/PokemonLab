/*
 * File:   StatusEffect.js
 * Author: Catherine
 *
 * Created on April 3 2009, 8:39 PM
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
 * StatusEffect constructor.
 *
 * The main use of this function will probably be when creating a new type of
 * status effect, say AwesomeEffect, where it would be used as follows:
 *
 *        function AwesomeEffect() { // AwesomeEffect constructor
 *            this.turns = 4;
 *        }
 *        // inherit the StatusEffect prototype
 *        AwesomeEffect.prototype = new StatusEffect("AwesomeEffect");
 *        AwesomeEffect.prototype.tick = function() {
 *            subject.field.print(subject.name + " was struck down by God!");
 *            subject.hp -= 1000;
 *        }
 *
 *        // .. somewhere in a move implementation
 *        target.addStatus(new AwesomeEffect());
 *
 */
function StatusEffect(id) {
    this.id = id;
}

/**
 * Some StatusEffect constants.
 */

// Special categories of status effects.
StatusEffect.SPECIAL_EFFECT = 1;
StatusEffect.WEATHER_EFFECT = 2;

// Flags used for the "garbage collection" system.
StatusEffect.STATE_ACTIVE = 0;
StatusEffect.STATE_DEACTIVATED = 1;
StatusEffect.STATE_REMOVABLE = 2;

/**
 * StatusEffect prototype.
 */
StatusEffect.prototype = {

    // Important internal properties
    id : "StatusEffect",            // Internal name of this StatusEffect.
    inducer : null,                 // Pokemon who induced this status effect.
    subject : null,                 // Subject (i.e. victim) of this status effect.
    lock : 0,                       // Special category of status effects, which cannot coexist.
	state : StatusEffect.STATE_ACTIVE,
    singleton : true,

    // Properties specific to each type of status effect
    name : "A Status Effect",       // English name of the effect
    passable : true,                // Can be Baton Passed?
    tier : -1,                      // End of turn tier for this effect.
    
    /**
     * Array of Array(id, method, func). "id" is the "id" of a StatusEffect, or another special
     * reserved keyword. "method" is the name of a typical status effect method. When the Pokemon
     * class is about to call into any status effect method, it first looks in "overrides" on every
     * status effect on every pokemon. If an appropriate Function object is found, it calls that
     * function instead, with an additional final parameter for the object whose "overrides" Array
     * produced a Function object.
     *
     * Returns an Array(overrode, ret) where overrode is whether we wanted to override the method.
     * If overrode is false, the call falls through and the original method is called as well.
     *
     * Example: Hypothetical partial implementation of Poison Heal
     *
     * PoisonHeal.prototype.overrides = new Array(
     *     new Array("PoisonEffect", "tick", function(overrider) {
     *             if (this.subject != overrider.subject)
     *                 return new Array(false, 0); // call the usual tick() instead
     *
     *             this.subject.hp += this.subject.stat[HP] / 8;
     *             return new Array(true, 0);
     *         });
     * );
     *
     */
    overrides : null,

    /**
     * Methods with a default implementation.
     */

    // Make a copy of this StatusEffect.
    copy : function() {
        var ret = { };
        for (var property in this) {
            ret[property] = this[property];
        }
        return ret;
    },

    // Initialise the effect on the subject, and also return whether to apply it.
    applyEffect : function() {
        return true;
    },

	// Handle the pokemon with the effect switching in.
	switchIn : function() { },

	// Unapply the effect.
	unapplyEffect : function() { },

    // Does this StatusEffect immobilise the subject?
    immobilises : function() {
        return false;
    },

    // Called once each round if tier != -1.
    tick : function() { },

    // Called when the subject switches out. Return whether to remove the effect.
    switchOut : function() {
        return true;
    },

	// A term in the subject's inherent priority.
	inherentPriority : function() {
		return 0;
	},

	// A term in the subject's critical hit sum.
	criticalModifier : function() {
		return 0;
	},

    /**
     * Methods that may be implemented to achieve a desired function, but have no
     * default implementation.
     */

    /**
     * Return a modifier to the damage formula.
     *
     * Return a [position, value, priority].
     *        position is the position in the damage formula
     *        value is the value of the multiplier (e.g. 0.5)
     *        priority is how this multiplier should be sorted with others
     *        move is the move being used
     *        critical is whether it is a critical hit
     * Return null for no modifier.
     *
     * Example:
     *
     *    function(field, user, target, move, critical) {
     *        return null;
     *    }
     */
    modifier : null,

    /**
     * Return a modifier to a stat.
     * 
     * Return [value, priority]
     *      stat is the id of the stat
     *      value is the value of the multiplier
     *      priority is where to place this multiplier in order
     * Return null for no modifier.
     *
     * Example:
     *
     *      // Hypothetical implementation of Pure Power.
     *      function(field, stat, subject) {
     *          if (subject != this.subject)
     *              return null;
     *          if (stat == S_ATTACK)
     *              return [2, 1];  // priority == 1 for abilities
     *          return null;
     *      }
     */
    statModifier : null,

    /**
     * Veto a move right before it is executed on a particular target. This is
     * also called before the move is executed in general, with target = null.
     * If the status effect completely prevents the execution of the move, it
     * should return true to the initial call.
     *
     * Example:
     *
     *    function(field, user, target, move) {
     *        return true; // veto all moves
     *    }
     */
    vetoExecution : null,

    /**
     * Prevent a move from being _selected_. This does not prevent the move
     * from being used. This is called for all pokemon, not just the subject.
     *
     * Example:
     *
     *    function(user, move) {
     *        // Prevent U-Turn from being selected by only the subject.
     *        if (user != this.subject)
     *            return false;
     *
     *        return (move.name == "U-turn");
     *    }
     */
    vetoSelection : null,

    /**
     * Transform a status effect right before it is applied. If the new
     * status effect is null then it is not applied.
     *
     * Example:
     *
     *    function(subject, status) {
     *        // protect the subject of this status effect from AwesomeEffect.
     *        if (subject != this.subject)
     *          return status;    // Unchanged.
     *        if (status.id == "AwesomeEffect")
     *          return null;
     *      return status;
     *    }
     *
     */
    transformStatus : null,

    /**
     * Potentially prevent a switch. Called for _all_ pokemon, not just the subject.
     *
     * Example:
     *
     *    function(subject) {
     *          // Prevent all enemy pokemon from switching.
     *          return (subject.party != this.subject.party);
     *      }
     */
    vetoSwitch : null,

	/**
	 * Modify the effectiveness of a move of a particular type being used on a
     * particular pokemon. Only handle one type at a time, not all the types
     * of the target. The second parameter is the particular type to handle, but
     * it is not always needed.
	 *
	 * Example:
	 *
	 *    function(moveType, type, target) {
     *        if ((moveType == GROUND) && (target == this.subject)) {
     *            // immune to ground attacks
	 *            return 0;
     *        }
	 *	      // return the usual effectiveness
     *        return getEffectiveness(moveType, type);
     *    }
	 *
	 */
	transformEffectiveness : null,
    
	/**
	 */
	transformMultiplier : null,

	/**
	 */
	transformHealthChange : null,

	speedComparator : null,

	informDamaged : null,

	informLostItem : null,

	/** bunch of other informs **/

};

