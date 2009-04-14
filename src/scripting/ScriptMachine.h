/* 
 * File:   ScriptMachine.h
 * Author: Catherine
 *
 * Created on March 31, 2009, 6:03 PM
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

#ifndef _SCRIPT_MACHINE_H_
#define _SCRIPT_MACHINE_H_

#include <vector>
#include <string>
#include <set>
#include <iostream>

#include "ObjectWrapper.h"
#include "../moves/PokemonMove.h"
#include "../mechanics/stat.h"

namespace shoddybattle {

class ScriptMachineImpl;

class ScriptMachineException {
    
};

class ScriptMachine;
class ScriptContext;
class Pokemon;
class BattleField;

class ScriptObject {
public:
    ScriptObject(void *p) {
        m_p = p;
        m_root = false;
    }
    void *getObject() const {
        return m_p;
    }
    void **getObjectRef() {
        if (!m_p)
            return NULL;
        return &m_p;
    }
    bool isNull() const {
        return (m_p == NULL);
    }
    virtual ~ScriptObject() { }
    ScriptObject(const ScriptObject &);
    ScriptObject &operator=(const ScriptObject &);
protected:
    void *m_p;
private:
    friend class ScriptContext;
    bool m_root;
};

class ScriptValue;

class ScriptArray : public ScriptObject {
public:
    ScriptArray(void *p, ScriptContext *cx): ScriptObject(p), m_cx(cx) { }

    ScriptValue operator[](const int i);
private:
    ScriptContext *m_cx;
};

class ScriptValue {
public:
    explicit ScriptValue(void *val = NULL): m_val(val), m_fail(false) { }
    ScriptValue(int i);
    ScriptValue(bool b);
    ScriptValue(const ScriptObject *object);
    ScriptValue(ObjectWrapper *obj) {
        m_fail = false;
        m_val = NULL;
        *this = obj;
    }
    ScriptObject getObject() const;
    int getInt() const;
    bool getBool() const;
    double getDouble(ScriptContext *cx) const;
    void *getValue() const {
        return m_val;
    }
    ScriptValue &operator=(ObjectWrapper *obj) {
        if (obj) {
            ScriptObject *sobj = obj->getObject();
            if (sobj) {
                m_val = sobj->getObject();
            }
        }
        return *this;
    }
    void setFailure() { m_fail = true; }
    bool failed() const { return m_fail; }
private:
    void *m_val;
    bool m_fail;
};

class PokemonObject : public ScriptObject {
public:
    PokemonObject(void *p = NULL): ScriptObject(p) { }
};

class FieldObject : public ScriptObject {
public:
    FieldObject(void *p): ScriptObject(p) { }
};

struct MODIFIER {
    int position;
    double value;
    int priority;
};

class MoveObject : public ScriptObject {
public:
    MoveObject(void *p = NULL): ScriptObject(p) { }

    std::string getName(ScriptContext *) const;
    MOVE_CLASS getMoveClass(ScriptContext *) const;
    TARGET getTargetClass(ScriptContext *) const;
    unsigned int getPower(ScriptContext *) const;
    int getPriority(ScriptContext *) const;
    void beginTurn(ScriptContext *, BattleField *, Pokemon *, Pokemon *);
    void use(ScriptContext *, BattleField *, Pokemon *, Pokemon *, const int);
    bool attemptHit(ScriptContext *, BattleField *, Pokemon *, Pokemon *);
    unsigned int getPp(ScriptContext *) const;
    double getAccuracy(ScriptContext *) const;
    const PokemonType *getType(ScriptContext *) const;
    bool getFlag(ScriptContext *, const MOVE_FLAG flag) const;
};

class ScriptFunction : public ScriptObject {
public:
    ScriptFunction(void *p = NULL): ScriptObject(p) { }
};

class StatusObject : public ScriptObject {
public:
    static const int STATE_ACTIVE = 0;
    static const int STATE_DEACTIVATED = 1;
    static const int STATE_REMOVABLE = 2;
    
    StatusObject(void *p): ScriptObject(p) { }

    StatusObject *cloneAndRoot(ScriptContext *);

    // State.
    int getState(ScriptContext *) const;
    bool isActive(ScriptContext *cx) const {
        return getState(cx) == STATE_ACTIVE;
    }
    bool isRemovable(ScriptContext *cx) const {
        return getState(cx) == STATE_REMOVABLE;
    }
    
    // Basic properties.
    std::string getId(ScriptContext *) const;
    Pokemon *getInducer(ScriptContext *) const;
    void setInducer(ScriptContext *, Pokemon *);
    Pokemon *getSubject(ScriptContext *) const;
    void setSubject(ScriptContext *, Pokemon *);
    int getLock(ScriptContext *) const;
    std::string getName(ScriptContext *) const;
    bool isPassable(ScriptContext *) const;
    int getTier(ScriptContext *) const;
    bool isSingleton(ScriptContext *) const;
    ScriptFunction *getOverride(ScriptContext *, std::string, std::string) const;
    int getInherentPriority(ScriptContext *);
    int getCriticalModifier(ScriptContext *);

    // Methods.
    bool applyEffect(ScriptContext *);
    void unapplyEffect(ScriptContext *);
    void switchIn(ScriptContext *);
    bool switchOut(ScriptContext *);
    bool immobilises(ScriptContext *);
    void tick(ScriptContext *);

    // Transformers.
    bool getModifier(ScriptContext *, BattleField *,
            Pokemon *, Pokemon *, MoveObject *, const bool, MODIFIER &);
    bool getStatModifier(ScriptContext *, BattleField *,
            STAT, Pokemon *, MODIFIER &);
    bool vetoExecution(ScriptContext *, BattleField *,
            Pokemon *, Pokemon *, MoveObject *);
    bool vetoSelection(ScriptContext *, Pokemon *, MoveObject *);
    bool transformStatus(ScriptContext *, Pokemon *, StatusObject **);
    bool vetoSwitch(ScriptContext *, Pokemon *);
    bool transformEffectiveness(ScriptContext *, int, int, Pokemon *, double *);
    bool transformHealthChange(ScriptContext *, int, bool, int *);
    // TODO: transformMultiplier

};

class ScriptContext {
public:
    ScriptFunction *compileFunction(
            const std::vector<std::string> args,
            const std::string body,
            const std::string file,
            const int line);

    /**
     * Create a new PokemonObject from the given Pokemon.
     */
    PokemonObject *newPokemonObject(Pokemon *p);

    /**
     * Create a new MoveObject from the given MoveTemplate.
     */
    MoveObject *newMoveObject(const MoveTemplate *p);

    /**
     * Create a new FieldObject for the given BattleField.
     */
    FieldObject *newFieldObject(BattleField *field);

    void addRoot(ScriptObject *obj);
    void removeRoot(ScriptObject *obj);

    StatusObject getAbility(const std::string) const;
    
    ScriptValue callFunction(ScriptObject *obj,
            const ScriptFunction *func,
            const int argc, ScriptValue *argv);

    ScriptValue callFunctionByName(ScriptObject *obj,
            const std::string name,
            const int argc, ScriptValue *argv);

    bool hasProperty(ScriptObject *obj, const std::string name) const;

    void runFile(const std::string file);

    ScriptMachine *getMachine() { return m_machine; }

    void gc();
    void maybeGc();

    bool isBusy() const { return m_busy; }

private:
    friend class ScriptMachine;
    friend class ScriptMachineImpl;
    friend class ScriptValue;
    friend class MoveObject;
    friend class StatusObject;
    friend class ScriptArray;
    ScriptContext(void *);
    void *m_p;
    ScriptMachine *m_machine;
    bool m_busy;

    ScriptContext(const ScriptContext &);
    ScriptContext &operator=(const ScriptContext &);
};

class Text;
class SpeciesDatabase;
class MoveDatabase;
class TextLookup;

/**
 * A script machine is used to work with scripts without having to contend
 * with the JavaScript API.
 */
class ScriptMachine {
public:
    ScriptMachine() throw(ScriptMachineException);
    ~ScriptMachine();

    /** Obtain a new context for running scripts. **/
    ScriptContext *acquireContext();

    /** Release a context for running scripts. **/
    void releaseContext(ScriptContext *cx);

    /** Global program state. **/
    Text *getText() const;
    SpeciesDatabase *getSpeciesDatabase() const;
    MoveDatabase *getMoveDatabase() const;

    std::string getText(int i, int j, int argc, const char **argv);
    void loadText(const std::string file, TextLookup &func);
    void includeMoves(const std::string);
    void includeSpecies(const std::string);
    void populateMoveLists();
    
private:
    friend class ScriptContext;
    ScriptMachineImpl *m_impl;

    ScriptMachine(const ScriptMachine &);
    ScriptMachine &operator=(const ScriptMachine &);
};

}

#endif
