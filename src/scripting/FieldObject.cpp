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
#include "../mechanics/PokemonType.h"

#include <iostream>

using namespace std;
using namespace boost;

namespace shoddybattle {

namespace {
    
enum FIELD_TINYID {
    FTI_GENERATION,
    FTI_LAST_MOVE,
    FTI_PARTY_SIZE,
    FTI_NARRATION,
    FTI_HOST,
    FTI_EXECUTION
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
 * field.getEffectiveness(type, pokemon)
 *
 * Get the effectiveness of a particular type against an arbitrary pokemon.
 */
JSBool getEffectiveness(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    if (!JSVAL_IS_INT(argv[0]) || !JSVAL_IS_OBJECT(argv[1])) {
        return JS_FALSE;
    }
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    const BattleMechanics *mech = p->getMechanics();
    const int type = JSVAL_TO_INT(argv[0]);
    JSObject *objPokemon = JSVAL_TO_OBJECT(argv[1]);
    Pokemon *defender = (Pokemon *)JS_GetPrivate(cx, objPokemon);
    const double effectiveness = mech->getEffectiveness(*p,
            PokemonType::getByValue(type), NULL, defender, NULL);
    JS_NewNumberValue(cx, effectiveness, ret);
    return JS_TRUE;
}

/**
 * field.getTypeEffectiveness(attackingType, defendingType)
 *
 * Get the effectiveness of a particular type against an arbitrary type.
 */
JSBool getTypeEffectiveness(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    if (!JSVAL_IS_INT(argv[0]) || !JSVAL_IS_INT(argv[1])) {
        return JS_FALSE;
    }
    const PokemonType *type0 = PokemonType::getByValue(JSVAL_TO_INT(argv[0]));
    const PokemonType *type1 = PokemonType::getByValue(JSVAL_TO_INT(argv[1]));
    if (type0 && type1) {
        const double effectiveness = type0->getMultiplier(*type1);
        JS_NewNumberValue(cx, effectiveness, ret);
    } else {
        *ret = JSVAL_NULL;
    }
    return JS_TRUE;
}

/**
 * field.getMoveCount()
 *
 * Return the number of moves that exist.
 */
JSBool getMoveCount(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    const int count = scx->getMachine()->getMoveDatabase()->getMoveCount();
    *ret = INT_TO_JSVAL(count);
    return JS_TRUE;
}

/**
 * field.getMove(name)
 * field.getMove(idx)
 */
JSBool getMove(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    jsval v = argv[0];

    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    MoveDatabase *moves = p->getScriptMachine()->getMoveDatabase();

    string str;
    if (JSVAL_IS_STRING(v)) {
        str = JS_GetStringBytes(JSVAL_TO_STRING(v));
    } else if (JSVAL_IS_INT(v)) {
        str = moves->getMove(JSVAL_TO_INT(v));
    } else {
        return JS_FALSE;
    }

    const MoveTemplate *tpl = moves->getMove(str);
    if (tpl) {
        MoveObjectPtr move = scx->newMoveObject(tpl);
        *ret = OBJECT_TO_JSVAL(move->getObject());
    } else {
        *ret = JSVAL_NULL;
    }

    return JS_TRUE;
}

/**
 * field.applyStatus(effect)
 */
JSBool applyStatus(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    jsval v = argv[0];
    if (!JSVAL_IS_OBJECT(v)) {
        return JS_FALSE;
    }

    BattleField *field = (BattleField *)JS_GetPrivate(cx, obj);
    StatusObject effect(JSVAL_TO_OBJECT(v));
    StatusObjectPtr ptr = field->applyStatus(&effect);
    if (ptr) {
        *ret = OBJECT_TO_JSVAL(ptr->getObject());
    } else {
        *ret = JSVAL_NULL;
    }

    return JS_TRUE;
}

/**
 * field.removeStatus(effect)
 */
JSBool removeStatus(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    jsval v = argv[0];
    if (!JSVAL_IS_OBJECT(v)) {
        return JS_FALSE;
    }

    BattleField *field = (BattleField *)JS_GetPrivate(cx, obj);
    StatusObject effect(JSVAL_TO_OBJECT(v));
    field->removeStatus(&effect);

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
 * field.isCriticalHit(move, user, target)
 */
JSBool isCriticalHit(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    if (!JSVAL_IS_OBJECT(argv[0]))
        return JS_FALSE;
    if (!JSVAL_IS_OBJECT(argv[1]))
        return JS_FALSE;
    if (!JSVAL_IS_OBJECT(argv[2]))
        return JS_FALSE;

    JSObject *mobj = JSVAL_TO_OBJECT(argv[0]);
    MoveObject move(mobj);

    Pokemon *user = (Pokemon *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[1]));
    Pokemon *target = (Pokemon *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[2]));

    const BattleMechanics *mech = p->getMechanics();
    const bool result = mech->isCriticalHit(*p, move, *user, *target);
    *ret = BOOLEAN_TO_JSVAL(result);
    return JS_TRUE;
}

/**
 * field.calculate(move, user, target, targets[, weight = true])
 */
JSBool calculate(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);

    // make sure arguments are of the correct type
    assert(JSVAL_IS_OBJECT(argv[0]));   // move
    assert(JSVAL_IS_OBJECT(argv[1]));   // user
    assert(JSVAL_IS_OBJECT(argv[2]));   // target
    assert(JSVAL_IS_INT(argv[3]));      // targets
    if (argc > 4) {
        assert(JSVAL_IS_BOOLEAN(argv[4]));
    }

    JSObject *mobj = JSVAL_TO_OBJECT(argv[0]);
    MoveObject move(mobj);
    
    Pokemon *user = (Pokemon *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[1]));
    Pokemon *target = (Pokemon *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[2]));
    const int targets = JSVAL_TO_INT(argv[3]);
    const bool weight = (argc > 4) ? JSVAL_TO_BOOLEAN(argv[4]) : true;

    const BattleMechanics *mech = p->getMechanics();
    const int damage = mech->calculateDamage(*p, move, *user, *target,
            targets, weight);

    *ret = INT_TO_JSVAL(damage);
    return JS_TRUE;
}

/**
 * field.getPartySize(party)
 *
 * Get the length of a particular party.
 */
JSBool getPartySize(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    const jsval v = argv[0];
    if (!JSVAL_IS_INT(v)) {
        return JS_FALSE;
    }
    const int party = JSVAL_TO_INT(v);
    if ((party != 0) && (party != 1)) {
        JS_ReportError(cx, "getPartySize: party must be 0 or 1");
        return JS_FALSE;
    }
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    *ret = INT_TO_JSVAL(p->getTeam(party).size());
    return JS_TRUE;
}

/**
 * field.getPokemon(party, idx)
 *
 * Get a particular pokemon.
 */
JSBool getPokemon(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    if (!JSVAL_IS_INT(argv[0]) || !JSVAL_IS_INT(argv[1])) {
        return JS_FALSE;
    }
    const int party = JSVAL_TO_INT(argv[0]);
    const int idx = JSVAL_TO_INT(argv[1]);
    if ((party != 0) && (party != 1)) {
        JS_ReportError(cx, "getPokemon: party must be 0 or 1");
        return JS_FALSE;
    }
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    void *sobj = p->getTeam(party)[idx]->getObject()->getObject();
    *ret = OBJECT_TO_JSVAL((JSObject *)sobj);
    return JS_TRUE;
}

/**
 * field.sendMessage(message, ...)
 *
 * TODO: This method duplicates a method in PokemonObject. The two methods
 *       should probably be consolidated.
 */
JSBool sendMessage(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    jsval v = argv[0];
    if (!JSVAL_IS_STRING(v)) {
        return JS_FALSE;
    }
    char *str = JS_GetStringBytes(JSVAL_TO_STRING(v));

    const int c = argc - 1;
    ScriptValue val[c];
    for (int i = 0; i < c; ++i) {
        val[i] = ScriptValue::fromValue((void *)argv[i + 1]);
    }

    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    ScriptValue vret = p->sendMessage(str, c, val);
    if (vret.failed()) {
        *ret = JSVAL_NULL;
    } else {
        *ret = (jsval)vret.getValue();
    }
    return JS_TRUE;
}

JSBool fieldSet(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    int tid = JSVAL_TO_INT(id);
    switch (tid) {
        case FTI_NARRATION: {
            const bool enabled = JSVAL_TO_BOOLEAN(*vp);
            p->setNarrationEnabled(enabled);
        } break;
    }
    return JS_TRUE;
}

JSBool fieldGet(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    BattleField *p = (BattleField *)JS_GetPrivate(cx, obj);
    int tid = JSVAL_TO_INT(id);
    switch (tid) {
        case FTI_GENERATION: {
            *vp = INT_TO_JSVAL(p->getGeneration());
        } break;
        case FTI_LAST_MOVE: {
            MoveObjectPtr move = p->getLastMove();
            if (move) {
                *vp = OBJECT_TO_JSVAL(move->getObject());
            } else {
                *vp = JSVAL_NULL;
            }
        } break;
        case FTI_PARTY_SIZE: {
            *vp = INT_TO_JSVAL(p->getPartySize());
        } break;
        case FTI_NARRATION: {
            *vp = BOOLEAN_TO_JSVAL(p->isNarrationEnabled());
        } break;
        case FTI_HOST: {
            *vp = INT_TO_JSVAL(p->getHost());
        } break;
        case FTI_EXECUTION: {
            const BattleField::EXECUTION *execution = p->topExecution();
            if (execution) {
                *vp = OBJECT_TO_JSVAL(execution->move->getObject());
            } else {
                *vp = JSVAL_NULL;
            }
        }
    }
    return JS_TRUE;
}

JSPropertySpec fieldProperties[] = {
    { "damage", 0, JSPROP_PERMANENT, NULL, NULL },
    { "generation", FTI_GENERATION, JSPROP_PERMANENT | JSPROP_SHARED, fieldGet, NULL },
    { "lastMove", FTI_LAST_MOVE, JSPROP_PERMANENT | JSPROP_SHARED, fieldGet, NULL },
    { "partySize", FTI_PARTY_SIZE, JSPROP_PERMANENT | JSPROP_SHARED, fieldGet, NULL },
    { "narration", FTI_NARRATION, JSPROP_PERMANENT | JSPROP_SHARED, fieldGet, fieldSet },
    { "host", FTI_HOST, JSPROP_PERMANENT | JSPROP_SHARED, fieldGet, NULL },
    { "execution", FTI_EXECUTION, JSPROP_PERMANENT | JSPROP_SHARED, fieldGet, NULL },
    { 0, 0, 0, 0, 0 }
};

JSFunctionSpec fieldFunctions[] = {
    JS_FS("calculate", calculate, 5, 0, 0),
    JS_FS("attemptHit", attemptHit, 3, 0, 0),
    JS_FS("random", random, 1, 0, 0),
    JS_FS("getMove", getMove, 1, 0, 0),
    JS_FS("print", print, 1, 0, 0),
    JS_FS("getActivePokemon", getActivePokemon, 2, 0, 0),
    JS_FS("applyStatus", applyStatus, 1, 0, 0),
    JS_FS("removeStatus", removeStatus, 1, 0, 0),
    JS_FS("sendMessage", sendMessage, 1, 0, 0),
    JS_FS("getPartySize", getPartySize, 1, 0, 0),
    JS_FS("getPokemon", getPokemon, 2, 0, 0),
    JS_FS("getEffectiveness", getEffectiveness, 2, 0, 0),
    JS_FS("getTypeEffectiveness", getTypeEffectiveness, 2, 0, 0),
    JS_FS("isCriticalHit", isCriticalHit, 3, 0, 0),
    JS_FS("getMoveCount", getMoveCount, 0, 0, 0),
    JS_FS_END
};

} // anonymous namespace

FieldObjectPtr ScriptContext::newFieldObject(BattleField *p) {
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
    FieldObjectPtr ptr = addRoot(new FieldObject(obj));
    JS_DefineProperties(cx, obj, fieldProperties);
    JS_DefineFunctions(cx, obj, fieldFunctions);
    JS_SetPrivate(cx, obj, p);
    JS_EndRequest(cx);
    return ptr;
}

}

