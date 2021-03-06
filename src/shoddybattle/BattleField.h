/* 
 * File:   BattleField.h
 * Author: Catherine
 *
 * Created on April 2, 2009, 10:10 PM
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

#ifndef _BATTLE_FIELD_H_
#define _BATTLE_FIELD_H_

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <vector>
#include <set>
#include "Pokemon.h"
#include "../matchmaking/MetagameList.h"
#include "../scripting/ObjectWrapper.h"

namespace shoddybattle {

const int TEAM_COUNT = 2;

class BattleFieldImpl;
class PokemonSlotImpl;

class BattleMechanics;

class ScriptMachine;
class ScriptContext;
class MoveObject;

class ScriptObject;

class BattleFieldException {
    
};

class TextMessage {
public:
    TextMessage(const int category, const int msg,
            const std::vector<std::string> &args = std::vector<std::string>()):
            m_category(category),
            m_msg(msg),
            m_args(args) { }
    int getCategory() const {
        return m_category;
    }
    int getMessage() const {
        return m_msg;
    }
    const std::vector<std::string> &getArgs() const {
        return m_args;
    }
private:
    const int m_category;
    const int m_msg;
    const std::vector<std::string> m_args;
};

enum TURN_TYPE {
    TT_MOVE = 0,
    TT_SWITCH = 1,
    TT_NOP = 2  // "No-op" turn: does nothing when executed.
};

struct PokemonTurn {
    static PokemonTurn NOP;
    
    TURN_TYPE type;
    int id;         // either id of move or the pokemon to which to switch
    int target;  // target of the move

    PokemonTurn(const TURN_TYPE tt, const int idx, const int ta = -1):
            type(tt),
            id(idx),
            target(ta) {

    }
    PokemonTurn(): type(TT_MOVE), id(-1), target(-1) { } // forced turn
};

struct PokemonParty {
public:
    PokemonParty(const int size, const std::string &name):
            m_size(size), m_name(name) {
        m_party = boost::shared_array<Pokemon::PTR>(new Pokemon::PTR[size]);
    }
    Pokemon::PTR &operator[](const int i) {
        return m_party[i];
    }
    const STATUSES &getEffects() const {
        return m_effects;
    }
    std::string getName() const {
        return m_name;
    }
    int getSize() const {
        return m_size;
    }
    ScriptValue sendMessage(const std::string &, int, ScriptValue *);
private:
    const int m_size;
    const std::string m_name;
    boost::shared_array<Pokemon::PTR> m_party;
    STATUSES m_effects;     // party-specific status effects
};

/**
 * Encapsulate the idea of a battle, including turn processing, move execution,
 * field effects, and more.
 *
 * BattleField objects cannot be copied or assigned to.
 */
class BattleField : public ObjectWrapper {
public:
    BattleField();
    virtual ~BattleField() { }

    /**
     * Initialise the BattleField using the provided BattleMechanics and
     * pokemon teams. The activeParty parameter specifices the size of each
     * team's active party. For singles, this would be one. For doubles, it
     * would be two. Shoddy Battle actually accepts arbitrarily high values
     * of this number with mechanics "extrapolating" from the game. However,
     * activeParty > 2 is not strictly a pokemon simulation, since there is
     * no equilvalent mode in the real game.
     */
    void initialise(const BattleMechanics *mech,
            Generation *,
            ScriptMachine *machine,
            Pokemon::ARRAY teams[TEAM_COUNT],
            const std::string trainer[TEAM_COUNT],
            const int activeParty,
            std::vector<StatusObject> &clauses);

    /**
     * Get the party size.
     */
    int getPartySize() const;

    /**
     * Get the host.
     */
    int getHost() const;

    /**
     * Begin the battle for a particular party.
     */
    void beginBattle(const int party);

    /**
     * Process a turn.
     */
    void processTurn(std::vector<PokemonTurn> &turn);

    /**
     * Execute pokemon's pending action.
     */
    bool executePendingAction(Pokemon *p);

    /**
     * Get the turn pending for a particular slot.
     */
    PokemonTurn *getTurn(const int i, const int j);

    /**
     * Execute a pokemon's pending's move action.
     */
    void executePendingMoveAction(Pokemon *);

    /**
     * Execute a switch action.
     */
    void executeSwitchAction(Pokemon *, const int);

    /**
     * Process a set of replacements.
     */
    void processReplacements(const std::vector<PokemonTurn> &turn);

    /**
     * Get the modifiers in play for a particular hit.
     */
    void getModifiers(Pokemon &, Pokemon &, MoveObject &,
            const bool, const int, MODIFIERS &);

    /**
     * Get the modifiers in play for a particular stat.
     */
    void getStatModifiers(STAT, Pokemon *, Pokemon *, PRIORITY_MAP &);

    /**
     * Transform a status effect.
     */
    void transformStatus(Pokemon *, boost::shared_ptr<StatusObject> *);

    /**
     * Sort a set of pokemon by speed.
     */
    template <class T>
    void sortBySpeed(T &pokemon);

    /**
     * Get a list of pokemon that can be switched to.
     */
    void getLegalSwitches(Pokemon *, std::vector<bool> &);

    /**
     * Get the active pokemon.
     */
    void getActivePokemon(Pokemon::ARRAY &, Pokemon::ARRAY * = NULL);
    boost::shared_ptr<PokemonParty> *getActivePokemon();
    Pokemon::PTR getActivePokemon(int i, int j) { // convenience method
        return (*getActivePokemon()[i])[j];
    }

    /**
     * Get the fainted pokemon.
     */
    void getFaintedPokemon(Pokemon::ARRAY &);

    /**
     * Get the number of living pokemon on a team.
     */
    int getAliveCount(const int party, const bool reserve = false) const;

    /**
     * Get a pokemon team.
     */
    const Pokemon::ARRAY &getTeam(const int i) const;

    /**
     * Get a random single target from a particular party.
     */
    Pokemon *getRandomTarget(const int party) const;

    /**
     * Get a list of targets for a move.
     */
    void getTargetList(TARGET, std::vector<Pokemon *> &,
            Pokemon *, Pokemon *);

    /**
     * Determine whether the execution of a move should be vetoed.
     */
    bool vetoExecution(Pokemon *, Pokemon *, MoveObject *);

    /**
     * Determine whether the selection of a move should be vetoed.
     */
    bool vetoSelection(Pokemon *, MoveObject *);

    /**
     * Determiner whether a switch by the subject should be vetoed.
     */
    bool vetoSwitch(Pokemon *);
    
    /**
     * Determine whether a particular turn is legal for a given pokemon.
     */
    bool isTurnLegal(Pokemon *, const PokemonTurn *, const bool) const;
    
    /**
     * Obtain the BattleMechanics in use on this BattleField.
     */
    const BattleMechanics *getMechanics() const;

    /**
     * Withdraw a pokemon.
     */
    void withdrawPokemon(Pokemon *);

    /**
     * Send out a new pokemon.
     */
    void sendOutPokemon(const int, const int, const int);

    /**
     * Check whether one party has won the battle.
     */
    bool determineVictory();

    /**
     * Process end of turn effects.
     */
    void tickEffects();

    /**
     * Get the last move that was executed by any pokemon.
     */
    boost::shared_ptr<MoveObject> getLastMove() const;

    /**
     * Apply a status effect to the whole field.
     */
    boost::shared_ptr<StatusObject> applyStatus(StatusObject *);

    /**
     * Find a status effect applied to the field by ID.
     */
    boost::shared_ptr<StatusObject> getStatus(const std::string &);

    /**
     * Remove a status effect from the field.
     */
    void removeStatus(StatusObject *);

    /**
     * Remove removable statuses from the field.
     */
    void removeStatuses();

    /**
     * Get a list of special types to which a pokemon is immune or vulnerable.
     */
    void getImmunities(Pokemon *user, Pokemon *target,
            std::set<const PokemonType *> &immunities,
            std::set<const PokemonType *> &vulnerabilities);
    
    /**
     * Get's the transformed effectiveness of a move type on a target
     */
    bool getTransformedEffectiveness(const PokemonType *moveType,
            const PokemonType *type, Pokemon *target, double &effectiveness);
    
    /**
     * Send a message to the whole field.
     */
    ScriptValue sendMessage(const std::string &, int, ScriptValue *);

    void setNarrationEnabled(const bool);
    bool isNarrationEnabled() const;

    Pokemon *getRandomInactivePokemon(Pokemon *);

    typedef Pokemon::RECENT_MOVE EXECUTION;
    const EXECUTION *topExecution() const;
    void pushExecution(const EXECUTION &exec);
    void popExecution();

    ScriptMachine *getScriptMachine();

    ScriptContext *getContext();

    ScriptObject *getObject();

    Generation *getGeneration() const;
    
    /**
     * Begin the battle.
     */
    virtual void beginBattle();

    /**
     * Print a message to the BattleField.
     */
    virtual void print(const TextMessage &msg);

    /**
     * Request a player to choose an inactive pokemon immediately. The default
     * implementation of this method selects a random inactive pokemon.
     */
    virtual Pokemon *requestInactivePokemon(Pokemon *pokemon) {
        return getRandomInactivePokemon(pokemon);
    }

    virtual void informVictory(const int);
    virtual void informUseMove(Pokemon *, MoveObject *);
    virtual void informWithdraw(Pokemon *);
    virtual void informSendOut(Pokemon *);
    virtual void informHealthChange(Pokemon *, const int);
    virtual void informSetPp(Pokemon *, const int, const int) { }
    virtual void informSetMove(Pokemon *, const int, const int,
            const int, const int) { }
    virtual void informFainted(Pokemon *);
    virtual void informStatusChange(Pokemon *, StatusObject *, const bool);

    /**
     * Frees all of the objects held by this BattleField. Do not use any
     * methods of the BattleField after calling this!
     */
    virtual void terminate();

private:
    boost::shared_ptr<BattleFieldImpl> m_impl;
    BattleField(const BattleField &);
    BattleField &operator=(const BattleField &);
};

}

#endif
