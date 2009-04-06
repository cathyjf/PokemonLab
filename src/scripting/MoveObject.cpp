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
#include "../mechanics/PokemonType.h"

using namespace std;

namespace shoddybattle {

string MoveObject::getName(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "name", &val);
    char *str = JS_GetStringBytes(JSVAL_TO_STRING(val));
    JS_EndRequest(cx);
    return str;
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

unsigned int MoveObject::getPower(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    jsval val;
    JS_BeginRequest(cx);
    JS_GetProperty(cx, (JSObject *)m_p, "power", &val);
    JS_EndRequest(cx);
    return JSVAL_TO_INT(val);
}

/*
TARGET MoveObject::getTargetClass(ScriptContext *) const;
bool MoveObject::attemptHit(ScriptContext *, BattleField *, Pokemon *, Pokemon *);
unsigned int MoveObject::getPp(ScriptContext *) const;
double MoveObject::getAccuracy(ScriptContext *) const;
const PokemonType *MoveObject::getType(ScriptContext *) const;
bool MoveObject::getFlag(ScriptContext *, const MOVE_FLAG flag) const;**/

void MoveObject::use(ScriptContext *scx,
        BattleField *field, Pokemon *user, Pokemon *target) {
    ScriptValue val[3] = { field, user, target };
    scx->callFunctionByName(this, "use", 3, val);
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

