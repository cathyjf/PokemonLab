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
#include <js/jsapi.h>

#include "ScriptMachine.h"
#include "../shoddybattle/Pokemon.h"
#include "../shoddybattle/BattleField.h"
#include "../mechanics/PokemonType.h"

#include <iostream>

using namespace std;

namespace shoddybattle {

bool StatusObject::getModifier(ScriptContext *scx, BattleField *field,
        Pokemon *user, Pokemon *target, MoveObject *mobj, const bool critical,
        const int targets, MODIFIER &mod) {
    if (!scx->hasProperty(this, "modifier"))
        return false;
    
    ScriptValue argv[] = { field, user, target, mobj, critical, targets };

    // need request to avoid the gc freeing the return value of the call
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    ScriptValue ret = scx->callFunctionByName(this, "modifier", 6, argv);
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
        STAT stat, Pokemon *subject, Pokemon *target, MODIFIER &mod) {
    if (!scx->hasProperty(this, "statModifier"))
        return false;

    ScriptValue argv[] = { field, stat, subject, target };

    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    ScriptValue ret = scx->callFunctionByName(this, "statModifier", 4, argv);
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

bool StatusObject::transformStatus(ScriptContext *scx,
        Pokemon *subject, StatusObjectPtr *pStatus) {
    if (!scx->hasProperty(this, "transformStatus"))
        return false;

    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    StatusObjectPtr status = *pStatus;
    ScriptValue argv[] = { subject, status.get() };
    ScriptValue v = scx->callFunctionByName(this, "transformStatus", 2, argv);
    void *obj = v.getObject().getObject();
    if (obj != status->getObject()) {
        if (!obj) {
            pStatus->reset();
        } else {
            *pStatus = scx->addRoot(new StatusObject(obj));;
        }
    }
    JS_EndRequest(cx);
    return true;
}

bool StatusObject::transformStatLevel(ScriptContext *scx, Pokemon *user,
        Pokemon *target, STAT stat, int *level) {
    if (!scx->hasProperty(this, "transformStatLevel"))
        return false;

    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    ScriptValue argv[] = { user, target, (int)stat, *level };
    ScriptValue v = scx->callFunctionByName(this,
            "transformStatLevel", 4, argv);
    
    bool ret = false;

    ScriptArray arr(v.getObject().getObject(), scx);
    if (!arr.isNull()) {
        *level = arr[0].getInt();
        ret = arr[1].getBool();
    }
    
    JS_EndRequest(cx);
    return ret;
}

bool StatusObject::transformHealthChange(ScriptContext *scx, int hp,
        Pokemon *user, bool indirect, int *pHp) {
    if (!scx->hasProperty(this, "transformHealthChange"))
        return false;

    ScriptValue argv[] = { hp, user, indirect };
    ScriptValue v = scx->callFunctionByName(this,
            "transformHealthChange", 3, argv);
    *pHp = v.getInt();
    return true;
}

const PokemonType *StatusObject::getVulnerability(ScriptContext *scx,
        Pokemon *user, Pokemon *target) {
    if (!scx->hasProperty(this, "vulnerability"))
        return NULL;
    
    ScriptValue argv[] = { user, target };
    ScriptValue v = scx->callFunctionByName(this, "vulnerability", 2, argv);
    const int type = v.getInt();
    if (type == -1)
        return NULL;
    return PokemonType::getByValue(type);
}

const PokemonType *StatusObject::getImmunity(ScriptContext *scx,
        Pokemon *user, Pokemon *target) {
    if (!scx->hasProperty(this, "immunity"))
        return NULL;

    ScriptValue argv[] = { user, target };
    ScriptValue v = scx->callFunctionByName(this, "immunity", 2, argv);
    const int type = v.getInt();
    if (type == -1)
        return NULL;
    return PokemonType::getByValue(type);
}

bool StatusObject::transformEffectiveness(ScriptContext *scx,
        int moveType, int type, Pokemon *target, double *effectiveness) {
    if (!scx->hasProperty(this, "transformEffectiveness"))
        return false;
    
    ScriptValue argv[] = { moveType, type, target };
    ScriptValue v = scx->callFunctionByName(this, "transformEffectiveness", 3, argv);
    *effectiveness = v.getDouble(scx);
    return true;
}

bool StatusObject::vetoSelection(ScriptContext *scx,
        Pokemon *user, MoveObject *move) {
    if (!scx->hasProperty(this, "vetoSelection"))
        return false;
    ScriptValue argv[] = { user, move };
    ScriptValue v = scx->callFunctionByName(this, "vetoSelection", 2, argv);
    return v.getBool();
}

bool StatusObject::vetoExecution(ScriptContext *scx, BattleField *field,
        Pokemon *user, Pokemon *target, MoveObject *move) {
    if (!scx->hasProperty(this, "vetoExecution"))
        return false;
    ScriptValue argv[] = { field, user, target, move };
    ScriptValue v = scx->callFunctionByName(this, "vetoExecution", 4, argv);
    return v.getBool();
}

bool StatusObject::validateTeam(ScriptContext *scx, const Pokemon::ARRAY &team) {
    if (!scx->hasProperty(this, "validateTeam"))
        return true;
    ScriptArrayPtr teamPtr = ScriptArray::newTeamArray(team, scx);
    ScriptValue argv[] = { teamPtr.get() };
    ScriptValue v = scx->callFunctionByName(this, "validateTeam", 1, argv);
    return v.getBool();
}

void StatusObject::informTargeted(ScriptContext *cx,
        Pokemon *user, MoveObject *move) {
    if (!cx->hasProperty(this, "informTargeted"))
        return;
    ScriptValue argv[] = { user, move };
    cx->callFunctionByName(this, "informTargeted", 2, argv);
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

void StatusObject::switchIn(ScriptContext *cx) {
    cx->callFunctionByName(this, "switchIn", 0, NULL);
}

bool StatusObject::switchOut(ScriptContext *cx) {
    ScriptValue v = cx->callFunctionByName(this, "switchOut", 0, NULL);
    return v.getBool();
}
    
void StatusObject::unapplyEffect(ScriptContext *cx) {
    cx->callFunctionByName(this, "unapplyEffect", 0, NULL);
    Pokemon *subject = getSubject(cx);
    subject->informStatusChange(this, false);
}

bool StatusObject::applyEffect(ScriptContext *scx) {
    ScriptValue v = scx->callFunctionByName(this, "applyEffect", 0, NULL);
    return v.getBool();
}

StatusObjectPtr StatusObject::cloneAndRoot(ScriptContext *scx) {
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    ScriptValue val = scx->callFunctionByName(this, "copy", 0, NULL);
    StatusObjectPtr ret;
    void *obj = val.getObject().getObject();
    if (obj != m_p) {
        ret = scx->addRoot(new StatusObject(obj));
    } else {
        ret = shared_from_this();
    }
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

int StatusObject::getIdx(ScriptContext *scx) {
    JSContext *cx = (JSContext *)scx->m_p;
    if (scx->hasProperty(this, "idx")) {
        jsval val;
        JS_BeginRequest(cx);
        JS_GetProperty(cx, (JSObject *)m_p, "idx", &val);
        JS_EndRequest(cx);
        assert(JSVAL_IS_INT(val));
        return JSVAL_TO_INT(val);
    }
    
    return -1;
}

string StatusObject::toString(ScriptContext *scx) {
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    ScriptValue v = scx->callFunctionByName(this, "toString", 0, NULL);
    jsval val = (jsval)v.getValue();
    string ret = JS_GetStringBytes(JSVAL_TO_STRING(val));
    JS_EndRequest(cx);
    return ret;
}

string StatusObject::getDescription(ScriptContext *scx) {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    if (!scx->hasProperty(this, "description"))
        return string();
    JS_GetProperty(cx, (JSObject *)m_p, "description", &val);
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

void StatusObject::setState(ScriptContext *scx, const int state) {
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    jsval val = INT_TO_JSVAL(state);
    JS_SetProperty(cx, (JSObject *)m_p, "state", &val);
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

int StatusObject::getRadius(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "radius", &val);
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

int StatusObject::getState(ScriptContext *scx) {
    if (scx->hasProperty(this, "getState")) {
        ScriptValue v = scx->callFunctionByName(this, "getState", 0, NULL);
        return v.getInt();
    }
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "state", &val);
    JS_EndRequest(cx);
    assert(JSVAL_IS_INT(val));
    int ret = JSVAL_TO_INT(val);
    return ret;
}

int StatusObject::getType(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "type", &val);
    JS_EndRequest(cx);
    assert(JSVAL_IS_INT(val));
    int ret = JSVAL_TO_INT(val);
    return ret;
}

string StatusObject::getName(ScriptContext *scx) const {
    // todo: actually returns an array now!
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "name", &val);
    JSString *jsstr;
    if (!JSVAL_IS_STRING(val)) {
        jsstr = JS_ValueToString(cx, val);
    } else {
        jsstr = JSVAL_TO_STRING(val);
    }
    string ret = JS_GetStringBytes(jsstr);
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

double StatusObject::getTier(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "tier", &val);
    jsdouble d;
    JS_ValueToNumber(cx, val, &d);
    JS_EndRequest(cx);
    return d;
}

int StatusObject::getSubtier(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "subtier", &val);
    JS_EndRequest(cx);
    assert(JSVAL_IS_INT(val));
    int ret = JSVAL_TO_INT(val);
    return ret;
}

int StatusObject::getVetoTier(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "vetoTier", &val);
    JS_EndRequest(cx);
    assert(JSVAL_IS_INT(val));
    int ret = JSVAL_TO_INT(val);
    return ret;
}

namespace {

JSBool returnSelf(JSContext * /*cx*/,
        JSObject *obj, uintN /*argc*/, jsval * /*argv*/, jsval *ret) {
    *ret = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

} // anonymous namespace

void StatusObject::disableClone(ScriptContext *scx) {
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    JS_DefineFunction(cx, (JSObject *)m_p, "copy", returnSelf, 0, 0);
    JS_EndRequest(cx);
}

}

