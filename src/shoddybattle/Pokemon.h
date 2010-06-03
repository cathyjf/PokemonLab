/* 
 * File:   Pokemon.h
 * Author: Catherine
 *
 * Created on March 28, 2009, 12:13 AM
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

#ifndef _POKEMON_H_
#define _POKEMON_H_

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <list>
#include <map>
#include <stack>
#include <set>
#include "../mechanics/stat.h"
#include "../scripting/ObjectWrapper.h"

namespace shoddybattle {

class BattleField;
class PokemonNature;
class PokemonType;
class PokemonSpecies;
class MoveTemplate;
class PokemonTurn;

class PokemonObject;
class MoveObject;
class StatusObject;

class ScriptMachine;
class ScriptContext;
class ScriptValue;

class Target;

typedef std::vector<const PokemonType *> TYPE_ARRAY;
typedef std::list<boost::shared_ptr<StatusObject> > STATUSES;

typedef std::map<int, double> PRIORITY_MAP;
// map<position, map<priority, value>>
typedef std::map<int, PRIORITY_MAP> MODIFIERS;

/**
 * The pokemon class contains all of the information about an arbitrary
 * Pokemon, including its stats, the moves it can use, its most recent actions,
 * its species, a battle field to which it may be attached, and so forth.
 *
 * Pokemon objects are mutable, but they cannot be copied or assigned to.
 */
class Pokemon : public ObjectWrapper {
public:
    typedef boost::shared_ptr<Pokemon> PTR;
    typedef std::vector<PTR> ARRAY;
    
    Pokemon(const PokemonSpecies *species,
            const std::string &nickname,
            const PokemonNature *nature,
            const std::string &ability,
            const std::string &item,
            const int *iv,
            const int *ev,
            const int level,
            const int gender,
            const unsigned char happiness,
            const bool shiny,
            const std::vector<std::string> &moves,
            const std::vector<int> &ppUps);

    void initialise(BattleField *field, boost::shared_ptr<ScriptContext>,
            const int i, const int j);

    ScriptValue sendMessage(const std::string &, int, ScriptValue *);

    void setTurn(PokemonTurn *turn) { m_turn = turn; }
    PokemonTurn *getTurn() { return m_turn; }

    int getInherentPriority() const;
    int getCriticalModifier() const;

    bool executeMove(boost::shared_ptr<MoveObject>,
            Pokemon *target, bool inform = true);
    bool useMove(MoveObject *, Pokemon *, const int);
    bool vetoExecution(Pokemon *, Pokemon *, MoveObject *);
    bool vetoSelection(Pokemon *, MoveObject *);

    void informTargeted(Pokemon *, boost::shared_ptr<MoveObject>);
    void informDamaged(Pokemon *, boost::shared_ptr<MoveObject>, int);
    boost::shared_ptr<MoveObject> getMemory() const;
    Pokemon *getMemoryPokemon() const;
    void removeMemory(Pokemon *);
    void clearMemory();
    
    const STATUSES &getEffects() const { return m_effects; }
    boost::shared_ptr<StatusObject> applyStatus(Pokemon *, StatusObject *);
    void removeStatus(StatusObject *);
    boost::shared_ptr<StatusObject> getStatus(const std::string &);
    boost::shared_ptr<StatusObject> getStatus(const int lock);
    void getModifiers(Pokemon *, Pokemon *,
            MoveObject *, const bool, const int, MODIFIERS &);
    void getStatModifiers(STAT, Pokemon *, Pokemon *, PRIORITY_MAP &);
    bool getTransformedStatLevel(Pokemon *, Pokemon *, STAT, int *);
    template <class T>
    static void removeStatuses(STATUSES &, T predicate);
    void removeStatuses();
    bool hasAbility(const std::string &);
    bool transformStatus(Pokemon *, boost::shared_ptr<StatusObject> *);

    int transformHealthChange(int, Pokemon *, bool) const;
    
    int getHp() const { return m_hp; }
    void setHp(int hp);

    std::string getSpeciesName() const;
    int getSpeciesId() const;
    std::string getName() const { return m_nickname; }
    std::string getToken() const;
    double getMass() const;

    unsigned int getBaseStat(const STAT i) const;
    unsigned int getIv(const STAT i) const { return m_iv[i]; }
    unsigned int getEv(const STAT i) const { return m_ev[i]; }
    unsigned int getStat(const STAT i);
    unsigned int getRawStat(const STAT i) const { return m_stat[i]; }
    void setRawStat(const STAT i, const unsigned int v) {
        m_stat[i] = v;
    }
    int getStatLevel(const STAT i) const { return m_statLevel[i]; }
    void setStatLevel(const STAT i, int level) {
        if (level < -6) {
            level = -6;
        } else if (level > 6) {
            level = 6;
        }
        m_statLevel[i] = level;
    }

    const TYPE_ARRAY &getTypes() const { return m_types; }
    void setTypes(TYPE_ARRAY &types) {
        m_types = types;
    }
    bool isType(const PokemonType *) const;

    unsigned int getLevel() const { return m_level; }
    const PokemonNature *getNature() const { return m_nature; }
    const std::vector<int> &getPpUps() const { return m_ppUps; }
    unsigned int getGender() const { return m_gender; }
    bool isShiny() const { return m_shiny; }

    ScriptObject *getObject() { return (ScriptObject *)m_object.get(); }
    BattleField *getField() { return m_field; }

    boost::shared_ptr<MoveObject> getMove(const int i);

    int getMove(const std::string &) const;

    bool isMoveUsed(const int i) const { return m_moveUsed[i]; }
    int getPp(const int i) const { return m_pp[i]; }
    int getMaxPp(const int i) const { return m_maxPp[i]; }
    void setPp(const int i, const int pp);
    void deductPp(const int i);
    void deductPp(boost::shared_ptr<MoveObject>);

    void setMove(const int, const std::string &, const int, const int);
    void setAbility(StatusObject *);
    void setAbility(const std::string &);
    void setItem(StatusObject *);
    void setItem(const std::string &);

    int getMoveCount() const { return m_moves.size(); }

    int getParty() const { return m_party; }
    int getPosition() const { return m_position; }
    int getSlot() const { return m_slot; }
    void setSlot(const int slot) { m_slot = slot; }
    bool isActive() const { return m_slot != -1; }

    void faint();
    bool isFainted() const { return m_fainted; }

    void switchIn();
    void switchOut();
    void determineLegalActions();
    void clearForcedTurn() {
        m_forcedTurn.reset();
    }
    void setForcedTurn(const PokemonTurn &turn);
    boost::shared_ptr<MoveObject> setForcedTurn(
            const MoveTemplate *, Pokemon *);
    PokemonTurn *getForcedTurn() const {
        if (!m_forcedTurn)
            return NULL;
        return m_forcedTurn.get();
    }
    bool isMoveLegal(const int i) const { return m_legalMove[i]; }
    bool isSwitchLegal() const { return m_legalSwitch; }

    struct RECENT_DAMAGE {
        Pokemon *user;
        boost::shared_ptr<MoveObject> move;
        int damage;
    };

    void clearRecentDamage() {
        while (!m_recent.empty()) m_recent.pop();
    }
    bool hasRecentDamage() const {
        return !m_recent.empty();
    }
    RECENT_DAMAGE popRecentDamage() {
        RECENT_DAMAGE ret = m_recent.top();
        m_recent.pop();
        return ret;
    }

    void setLastMove(boost::shared_ptr<MoveObject> move) {
        m_lastMove = move;
    }

    boost::shared_ptr<MoveObject> getLastMove() const {
        return m_lastMove;
    }
    
    bool hasActed() const {
        return m_acted;
    }

    boost::shared_ptr<StatusObject> getItem() const;
    boost::shared_ptr<StatusObject> getAbility() const;

    std::string getItemName() const;
    std::string getAbilityName() const;

    void getImmunities(Pokemon *user, Pokemon *target,
            std::set<const PokemonType *> &immunities,
            std::set<const PokemonType *> &vulnerabilities);
            
    bool getTransformedEffectiveness(const PokemonType *moveType,
            const PokemonType *type, Pokemon *target, double &effectiveness);

    unsigned char getHappiness() const {
        return m_happiness;
    }

    bool isDamaged() const {
        return m_damaged;
    }

    void clearDamagedFlag() {
        m_damaged = false;
    }
    
    void informStatusChange(StatusObject *status, const bool applied);

    struct RECENT_MOVE {
        Pokemon *user;
        boost::shared_ptr<MoveObject> move;
        bool operator==(const RECENT_MOVE &rhs) const {
            return user == rhs.user;
        }
    };

private:
    void setMove(const int, boost::shared_ptr<MoveObject>,
            const int, const int);

    const PokemonSpecies *m_species;
    unsigned int m_level;
    int m_hp; // The remaining health of the pokemon.
    bool m_fainted;
    unsigned int m_stat[STAT_COUNT];
    unsigned int m_iv[STAT_COUNT];
    unsigned int m_ev[STAT_COUNT];
    int m_statLevel[TOTAL_STAT_COUNT];  // Level of stat boost.
    unsigned int m_gender;    // This pokemon's gender.
    unsigned char m_happiness;
    bool m_shiny;
    const PokemonNature *m_nature;
    TYPE_ARRAY m_types; // Current effective type.
    std::vector<int> m_ppUps;
    std::vector<const MoveTemplate *> m_moveProto;
    std::vector<boost::shared_ptr<MoveObject> > m_moves;
    std::vector<int> m_pp;
    std::vector<int> m_maxPp;
    std::vector<bool> m_moveUsed;
    std::vector<bool> m_legalMove;
    bool m_legalSwitch;
    std::string m_nickname;
    std::string m_itemName;
    std::string m_abilityName;

    typedef RECENT_MOVE MEMORY;
    std::list<MEMORY> m_memory;

    boost::shared_ptr<MoveObject> m_lastMove;

    bool m_acted;   // Has this pokemon acted since it became active?
    bool m_damaged; // Has this pokemon been damaged this turn?

    std::stack<RECENT_DAMAGE> m_recent;

    boost::shared_ptr<StatusObject> m_item;
    boost::shared_ptr<StatusObject> m_ability;

    ScriptMachine *m_machine;
    ScriptContext *m_cx;
    boost::shared_ptr<ScriptContext> m_scx;
    BattleField *m_field;
    int m_party, m_position, m_slot;

    STATUSES m_effects;

    PokemonTurn *m_turn;
    boost::shared_ptr<PokemonTurn> m_forcedTurn;
    boost::shared_ptr<MoveObject> m_forcedMove;

    boost::shared_ptr<PokemonObject> m_object;

    Pokemon(const Pokemon &);
    Pokemon &operator=(const Pokemon &);
};

}

#endif
