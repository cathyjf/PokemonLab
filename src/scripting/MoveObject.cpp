/* 
 * File:   MoveObject.cpp
 * Author: Catherine
 *
 * Created on April 1, 2009, 10:59 PM
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
 * This file implements the JavaScript interface to a Move object.
 */

#include <stdlib.h>
#include <time.h>
#include <nspr/nspr.h>
#include <jsapi.h>
#include <set>
#include <iostream>

#include "ScriptMachine.h"
#include "../shoddybattle/Pokemon.h"
#include "../shoddybattle/BattleField.h"
#include "../mechanics/PokemonType.h"
#include "../mechanics/BattleMechanics.h"

using namespace std;

namespace shoddybattle {

string MoveObject::getName(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "name", &val);
    string ret = JS_GetStringBytes(JSVAL_TO_STRING(val));
    JS_EndRequest(cx);
    return ret;
}


bool MoveObject::getFlag(ScriptContext *scx, const MOVE_FLAG flag) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "flags", &val);
    JSObject *obj = JSVAL_TO_OBJECT(val);
    JS_GetElement(cx, obj, flag, &val);
    JS_EndRequest(cx);
    return JSVAL_TO_BOOLEAN(val);
}

MOVE_CLASS MoveObject::getMoveClass(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "moveClass", &val);
    MOVE_CLASS mc = (MOVE_CLASS)JSVAL_TO_INT(val);
    JS_EndRequest(cx);
    return mc;
}

const PokemonType *MoveObject::getType(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "type", &val);
    int type = JSVAL_TO_INT(val);
    JS_EndRequest(cx);
    return PokemonType::getByValue(type);
}

unsigned int MoveObject::getPp(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "pp", &val);
    JS_EndRequest(cx);
    return JSVAL_TO_INT(val);
}

unsigned int MoveObject::getPower(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "power", &val);
    JS_EndRequest(cx);
    return JSVAL_TO_INT(val);
}

int MoveObject::getPriority(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "priority", &val);
    JS_EndRequest(cx);
    return JSVAL_TO_INT(val);
}

TARGET MoveObject::getTargetClass(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "targetClass", &val);
    JS_EndRequest(cx);
    return (TARGET)JSVAL_TO_INT(val);
}

/*
double MoveObject::getAccuracy(ScriptContext *) const;**/

bool MoveObject::attemptHit(ScriptContext *scx, BattleField *field,
        Pokemon *user, Pokemon *target) {
    if (scx->hasProperty(this, "attemptHit")) {
        ScriptValue val[] = { field, user, target };
        return scx->callFunctionByName(this, "attemptHit", 3, val).getBool();
    }
    return field->getMechanics()->attemptHit(*field, *this, *user, *target);
}

void MoveObject::beginTurn(ScriptContext *scx, BattleField *field,
        Pokemon *user, Pokemon *target) {
    if (!scx->hasProperty(this, "beginTurn"))
        return;
    ScriptValue val[] = { field, user, target };
    scx->callFunctionByName(this, "beginTurn", 3, val);
}

void MoveObject::use(ScriptContext *scx, BattleField *field,
        Pokemon *user, Pokemon *target, const int targets) {
    if (scx->hasProperty(this, "use")) {
        ScriptValue val[4] = { field, user, target, targets };
        scx->callFunctionByName(this, "use", 4, val);
    } else {
        // just do a basic move use
        int damage = field->getMechanics()->calculateDamage(*field,
                *this, *user, *target, targets);
        target->setHp(target->getHp() - damage);
    }
}

MoveObject *ScriptContext::newMoveObject(const MoveTemplate *p) {
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
    MoveObject *ret = new MoveObject(obj);
    addRoot(ret);
    
    string name = p->getName();
    char *pstr = JS_strdup(cx, name.c_str());
    JSString *str = JS_NewString(cx, pstr, name.length());
    jsval val = STRING_TO_JSVAL(str);
    JS_SetProperty(cx, obj, "name", &val);

    val = INT_TO_JSVAL(((int)p->getMoveClass()));
    JS_SetProperty(cx, obj, "moveClass", &val);

    val = INT_TO_JSVAL(p->getTargetClass());
    JS_SetProperty(cx, obj, "targetClass", &val);

    val = INT_TO_JSVAL(p->getPower());
    JS_SetProperty(cx, obj, "power", &val);

    val = INT_TO_JSVAL(p->getPp());
    JS_SetProperty(cx, obj, "pp", &val);

    val = INT_TO_JSVAL(p->getPriority());
    JS_SetProperty(cx, obj, "priority", &val);

    JS_NewNumberValue(cx, p->getAccuracy(), &val);
    JS_SetProperty(cx, obj, "accuracy", &val);

    val = INT_TO_JSVAL(p->getType()->getTypeValue());
    JS_SetProperty(cx, obj, "type", &val);

    jsval flags[FLAG_COUNT];
    for (int i = 0; i < FLAG_COUNT; ++i) {
        flags[i] = BOOLEAN_TO_JSVAL(p->getFlag((MOVE_FLAG)i));
    }
    JSObject *arr = JS_NewArrayObject(cx, FLAG_COUNT, flags);
    val = OBJECT_TO_JSVAL(arr);
    JS_SetProperty(cx, obj, "flags", &val);

    const ScriptFunction *func = p->getInitFunction();
    if (func && !func->isNull()) {
        val = OBJECT_TO_JSVAL((JSObject *)func->getObject());
        JS_SetProperty(cx, obj, "init", &val);
    }

    if (func && !func->isNull()) { // init function
        callFunction(ret, func, 0, NULL);
    }
    
    const ScriptFunction *func2 = p->getUseFunction();
    if (func2 && !func2->isNull()) {
        val = OBJECT_TO_JSVAL((JSObject *)func2->getObject());
        JS_SetProperty(cx, obj, "use", &val);
    }

    const ScriptFunction *func3 = p->getAttemptHitFunction();
    if (func3 && !func3->isNull()) {
        val = OBJECT_TO_JSVAL((JSObject *)func3->getObject());
        JS_SetProperty(cx, obj, "attemptHit", &val);
    }

    JS_EndRequest(cx);

    return ret;
}

}

