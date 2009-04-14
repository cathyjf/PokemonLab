/* 
 * File:   StatusObject.cpp
 * Author: Catherine
 *
 * Created on April 5, 2009, 2:19 PM
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

#include <stdlib.h>
#include <nspr/nspr.h>
#include <jsapi.h>

#include "ScriptMachine.h"
#include "../shoddybattle/Pokemon.h"
#include "../shoddybattle/BattleField.h"

#include <iostream>

using namespace std;

/**
// Basic properties.
ScriptFunction *getOverride(ScriptContext *, std::string, std::string) const;

// Transformers.
bool getModifier(ScriptContext *,
        BattleField *, Pokemon *, Pokemon *, MODIFIER &);
bool transformMove(ScriptContext *, Pokemon *, MoveObject **);
bool vetoMove(ScriptContext *, Pokemon *, MoveObject *);
bool transformStatus(ScriptContext *, Pokemon *, StatusObject **);
bool vetoSwitch(ScriptContext *, Pokemon *);
bool transformEffectiveness(int, int, Pokemon *, double *);
bool transformHealthChange(int, int *);
// TODO: transformMultiplier
 **/

namespace shoddybattle {

bool StatusObject::getModifier(ScriptContext *scx, BattleField *field,
        Pokemon *user, Pokemon *target, MoveObject *mobj, const bool critical,
        MODIFIER &mod) {
    if (!scx->hasProperty(this, "modifier"))
        return false;
    
    ScriptValue argv[5] = { field, user, target, mobj, critical };

    // need request to avoid the gc freeing the return value of the call
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    ScriptValue ret = scx->callFunctionByName(this, "modifier", 5, argv);
    bool b = false;
    if (!ret.failed()) {
        ScriptArray arr(ret.getObject().getObject(), scx);
        if (!arr.isNull()) {
            b = true;
            mod.position = arr[0].getInt();
            mod.value = arr[1].getDouble(scx);
            mod.priority = arr[2].getInt();
        }
    }
    JS_EndRequest(cx);
    return b;
}

bool StatusObject::getStatModifier(ScriptContext *scx, BattleField *field,
        STAT stat, Pokemon *subject, MODIFIER &mod) {
    if (!scx->hasProperty(this, "statModifier"))
        return false;

    ScriptValue argv[3] = { field, stat, subject };

    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    ScriptValue ret = scx->callFunctionByName(this, "statModifier", 3, argv);
    bool b = false;
    if (!ret.failed()) {
        ScriptArray arr(ret.getObject().getObject(), scx);
        if (!arr.isNull()) {
            b = true;
            mod.position = -1;  // unused here
            mod.value = arr[0].getDouble(scx);
            mod.priority = arr[1].getInt();
        }
    }
    JS_EndRequest(cx);
    return b;
}

bool StatusObject::vetoExecution(ScriptContext *scx, BattleField *field,
        Pokemon *user, Pokemon *target, MoveObject *move) {
    if (!scx->hasProperty(this, "vetoExecution"))
        return false;
    ScriptValue argv[] = { field, user, target, move };
    ScriptValue v = scx->callFunctionByName(this, "vetoExecution", 4, argv);
    return v.getBool();
}

int StatusObject::getInherentPriority(ScriptContext *cx) {
    ScriptValue v = cx->callFunctionByName(this, "inherentPriority", 0, NULL);
    return v.getInt();
}

int StatusObject::getCriticalModifier(ScriptContext *cx) {
    ScriptValue v = cx->callFunctionByName(this, "criticalModifier", 0, NULL);
    return v.getInt();
}

void StatusObject::tick(ScriptContext *cx) {
    cx->callFunctionByName(this, "tick", 0, NULL);
}

bool StatusObject::immobilises(ScriptContext *cx) {
    ScriptValue v = cx->callFunctionByName(this, "immobilises", 0, NULL);
    return v.getBool();
}

void StatusObject::switchIn(ScriptContext *cx) {
    cx->callFunctionByName(this, "switchIn", 0, NULL);
}

bool StatusObject::switchOut(ScriptContext *cx) {
    ScriptValue v = cx->callFunctionByName(this, "switchOut", 0, NULL);
    return v.getBool();
}
    
void StatusObject::unapplyEffect(ScriptContext *cx) {
    cx->callFunctionByName(this, "unapplyEffect", 0, NULL);
}

bool StatusObject::applyEffect(ScriptContext *scx) {
    ScriptValue v = scx->callFunctionByName(this, "applyEffect", 0, NULL);
    return v.getBool();
}

StatusObject *StatusObject::cloneAndRoot(ScriptContext *scx) {
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    ScriptValue val = scx->callFunctionByName(this, "copy", 0, NULL);
    StatusObject *ret = new StatusObject(val.getObject().getObject());
    scx->addRoot(ret);
    JS_EndRequest(cx);
    return ret;
}

string StatusObject::getId(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "id", &val);
    assert(JSVAL_IS_STRING(val));
    string ret = JS_GetStringBytes(JSVAL_TO_STRING(val));
    JS_EndRequest(cx);
    return ret;
}

Pokemon *StatusObject::getInducer(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "inducer", &val);
    JS_EndRequest(cx);
    if (JSVAL_IS_NULL(val))
        return NULL;
    assert(JSVAL_IS_OBJECT(val));
    JSObject *obj = JSVAL_TO_OBJECT(val);
    Pokemon *p = (Pokemon *)JS_GetPrivate(cx, obj);
    return p;
}

void StatusObject::setInducer(ScriptContext *scx, Pokemon *p) {
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    JSObject *obj = (JSObject *)p->getObject()->getObject();
    jsval val = OBJECT_TO_JSVAL(obj);
    JS_SetProperty(cx, (JSObject *)m_p, "inducer", &val);
    JS_EndRequest(cx);
}

Pokemon *StatusObject::getSubject(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "subject", &val);
    JS_EndRequest(cx);
    if (JSVAL_IS_NULL(val))
        return NULL;
    assert(JSVAL_IS_OBJECT(val));
    JSObject *obj = JSVAL_TO_OBJECT(val);
    Pokemon *p = (Pokemon *)JS_GetPrivate(cx, obj);
    return p;
}

void StatusObject::setSubject(ScriptContext *scx, Pokemon *p) {
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    JSObject *obj = (JSObject *)p->getObject()->getObject();
    jsval val = OBJECT_TO_JSVAL(obj);
    JS_SetProperty(cx, (JSObject *)m_p, "subject", &val);
    JS_EndRequest(cx);
}

int StatusObject::getLock(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "lock", &val);
    JS_EndRequest(cx);
    assert(JSVAL_IS_INT(val));
    int ret = JSVAL_TO_INT(val);
    return ret;
}

bool StatusObject::isSingleton(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "singleton", &val);
    JS_EndRequest(cx);
    assert(JSVAL_IS_BOOLEAN(val));
    bool ret = JSVAL_TO_BOOLEAN(val);
    return ret;
}

int StatusObject::getState(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "state", &val);
    JS_EndRequest(cx);
    assert(JSVAL_IS_INT(val));
    int ret = JSVAL_TO_INT(val);
    return ret;
}

string StatusObject::getName(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "name", &val);
    assert(JSVAL_IS_STRING(val));
    string ret = JS_GetStringBytes(JSVAL_TO_STRING(val));
    JS_EndRequest(cx);
    return ret;
}

bool StatusObject::isPassable(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "passable", &val);
    JS_EndRequest(cx);
    assert(JSVAL_IS_BOOLEAN(val));
    return JSVAL_TO_BOOLEAN(val);
}

int StatusObject::getTier(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "tier", &val);
    JS_EndRequest(cx);
    assert(JSVAL_IS_INT(val));
    int ret = JSVAL_TO_INT(val);
    return ret;
}

}

