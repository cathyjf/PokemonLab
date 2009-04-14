/* 
 * File:   FieldObject.cpp
 * Author: Catherine
 *
 * Created on April 14, 2009, 1:23 AM
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
#include "../shoddybattle/BattleField.h"
#include "../mechanics/BattleMechanics.h"

#include <iostream>

using namespace std;

namespace shoddybattle {

namespace {

JSBool attemptHit(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);

    // make sure arguments are of the correct type
    assert(JSVAL_IS_OBJECT(argv[0]));   // move
    assert(JSVAL_IS_OBJECT(argv[1]));   // user
    assert(JSVAL_IS_OBJECT(argv[2]));   // target

    JSObject *mobj = JSVAL_TO_OBJECT(argv[0]);
    MoveObject move(mobj);

    Pokemon *user = (Pokemon *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[1]));
    Pokemon *target = (Pokemon *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[2]));

    const BattleMechanics *mech = p->getMechanics();
    const bool hit = mech->attemptHit(*p, move, *user, *target);

    *ret = BOOLEAN_TO_JSVAL(hit);
    return JS_TRUE;
}

JSBool calculate(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);

    // make sure arguments are of the correct type
    assert(JSVAL_IS_OBJECT(argv[0]));   // move
    assert(JSVAL_IS_OBJECT(argv[1]));   // user
    assert(JSVAL_IS_OBJECT(argv[2]));   // target
    assert(JSVAL_IS_INT(argv[3]));      // targets

    JSObject *mobj = JSVAL_TO_OBJECT(argv[0]);
    MoveObject move(mobj);
    
    Pokemon *user = (Pokemon *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[1]));
    Pokemon *target = (Pokemon *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[2]));
    const int targets = JSVAL_TO_INT(argv[3]);

    const BattleMechanics *mech = p->getMechanics();
    const int damage = mech->calculateDamage(*p, move, *user, *target, targets);

    *ret = INT_TO_JSVAL(damage);
    return JS_TRUE;
}

JSPropertySpec fieldProperties[] = {
    { "damage", 0, JSPROP_PERMANENT, NULL, NULL },
    { 0, 0, 0, 0, 0 }
};

JSFunctionSpec fieldFunctions[] = {
    JS_FS("calculate", calculate, 4, 0, 0),
    JS_FS("attemptHit", attemptHit, 3, 0, 0),
    JS_FS_END
};

} // anonymous namespace

FieldObject *ScriptContext::newFieldObject(BattleField *p) {
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
    FieldObject *ret = new FieldObject(obj);
    addRoot(ret);
    JS_DefineProperties(cx, obj, fieldProperties);
    JS_DefineFunctions(cx, obj, fieldFunctions);
    JS_SetPrivate(cx, obj, p);
    JS_EndRequest(cx);
    return ret;
}

}

