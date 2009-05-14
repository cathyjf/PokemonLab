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
using namespace boost;

namespace shoddybattle {

namespace {
    
enum FIELD_TINYID {
    FTI_GENERATION
};

/**
 *      field.random(lower, upper)
 *          Generate a random integer in the range [lower, upper], distributed
 *          according to a uniform distribution.
 *
 *      field.random(chance)
 *          Return a boolean with the given chance of being true.
 *
 */
JSBool random(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    const BattleMechanics *mech = p->getMechanics();

    if (argc >= 2) {
        int lower = 0, upper = 0;
        JS_ConvertArguments(cx, 2, argv, "ii", &lower, &upper);
        *ret = INT_TO_JSVAL(mech->getRandomInt(lower, upper));
    } else if (argc == 1) {
        jsdouble p = 0.0;
        JS_ConvertArguments(cx, 1, argv, "d", &p);
        if (p < 0.00) {
            p = 0.00;
        } else if (p > 1.00) {
            p = 1.00;
        }
        *ret = BOOLEAN_TO_JSVAL(mech->getCoinFlip(p));
    }

    return JS_TRUE;
}

JSBool getActivePokemon(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    BattleField *field = (BattleField *)JS_GetPrivate(cx, obj);
    int party = 0, idx = 0;
    JS_ConvertArguments(cx, 2, argv, "ii", &party, &idx);
    if ((party != 0) && (party != 1)) {
        JS_ReportError(cx, "getActivePokemon: party must be 0 or 1");
        return JS_FALSE;
    }
    if (idx < 0) {
        JS_ReportError(cx, "getActivePokemon: position must be > 0");
        return JS_FALSE;
    }
    shared_ptr<PokemonParty> p = field->getActivePokemon()[party];
    const int size = p->getSize();
    if (idx >= size) {
        *ret = JSVAL_NULL;
        return JS_TRUE;
    }
    Pokemon::PTR pokemon = (*p)[idx].pokemon;
    if (!pokemon || pokemon->isFainted()) {
        *ret = JSVAL_NULL;
    } else {
        *ret = OBJECT_TO_JSVAL(pokemon->getObject()->getObject());
    }
    return JS_TRUE;
}

/**
 * getMove(name)
 */
JSBool getMove(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    jsval v = argv[0];
    if (!JSVAL_IS_STRING(v)) {
        return JS_FALSE;
    }
    char *str = JS_GetStringBytes(JSVAL_TO_STRING(v));

    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    MoveDatabase *moves = p->getScriptMachine()->getMoveDatabase();
    const MoveTemplate *tpl = moves->getMove(str);
    if (tpl) {
        MoveObject *move = scx->newMoveObject(tpl);
        *ret = OBJECT_TO_JSVAL(move->getObject());
        scx->removeRoot(move);
    } else {
        *ret = JSVAL_NULL;
    }

    return JS_TRUE;
}

JSBool print(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    jsval v = argv[0];
    assert(JSVAL_IS_OBJECT(v));
    JSObject *arr = JSVAL_TO_OBJECT(v);
    jsuint length = 0;
    JS_GetArrayLength(cx, arr, &length);
    jsval msg0, msg1;
    JS_GetElement(cx, arr, 0, &msg0);
    JS_GetElement(cx, arr, 1, &msg1);
    int category = 0, msg = 0;
    category = JSVAL_TO_INT(msg0);
    msg = JSVAL_TO_INT(msg1);
    vector<string> args;
    args.reserve(length - 2);
    for (int i = 2; i < length; ++i) {
        jsval val;
        JS_GetElement(cx, arr, i, &val);
        JSString *str = JS_ValueToString(cx, val);
        char *pstr = JS_GetStringBytes(str);
        args.push_back(pstr);
    }

    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    p->print(TextMessage(category, msg, args));

    return JS_TRUE;
}

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

/**
 * field.calculate(move, user, target, targets)
 */
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

JSBool fieldGet(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    int tid = JSVAL_TO_INT(id);
    switch (tid) {
        case FTI_GENERATION: {
            *vp = INT_TO_JSVAL(p->getGeneration());
        } break;
    }
    return JS_TRUE;
}

JSPropertySpec fieldProperties[] = {
    { "damage", 0, JSPROP_PERMANENT, NULL, NULL },
    { "generation", FTI_GENERATION, JSPROP_PERMANENT | JSPROP_SHARED, fieldGet, NULL },
    { 0, 0, 0, 0, 0 }
};

JSFunctionSpec fieldFunctions[] = {
    JS_FS("calculate", calculate, 4, 0, 0),
    JS_FS("attemptHit", attemptHit, 3, 0, 0),
    JS_FS("random", random, 1, 0, 0),
    JS_FS("getMove", getMove, 1, 0, 0),
    JS_FS("print", print, 1, 0, 0),
    JS_FS("getActivePokemon", getActivePokemon, 2, 0, 0),
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

