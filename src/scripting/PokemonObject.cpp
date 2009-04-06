/* 
 * File:   PokemonObject.cpp
 * Author: Catherine
 *
 * Created on April 4, 2009, 1:55 AM
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
 * This file implements the JavaScript interface to a Pokemon object.
 */

#include <stdlib.h>
#include <nspr/nspr.h>
#include <jsapi.h>

#include "ScriptMachine.h"
#include "../shoddybattle/Pokemon.h"

#include <iostream>

using namespace std;

namespace shoddybattle {

enum POKEMON_TINYID {
    PTI_SPECIES,
    PTI_NAME,
    PTI_BASE,
    PTI_IV,
    PTI_EV,
    PTI_STAT,
    PTI_LEVEL,
    PTI_NATURE,
    PTI_HP,     // modifiable
    PTI_TYPES,
    PTI_PPUPS,
    PTI_GENDER,
};

JSBool applyStatus(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    *ret = JSVAL_NULL;
    Pokemon *p = (Pokemon *)JS_GetPrivate(cx, obj);
    if (JSVAL_IS_OBJECT(argv[1])) {
        StatusObject status(JSVAL_TO_OBJECT(argv[1]));
        ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
        Pokemon *inducer = NULL;
        if (JSVAL_IS_OBJECT(argv[0])) {
            inducer = (Pokemon *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
        }
        StatusObject *objret = p->applyStatus(scx, inducer, &status);
        if (objret) {
            *ret = OBJECT_TO_JSVAL(objret->getObject());
        }
    }
    return JS_TRUE;
}

JSBool PokemonSet(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    Pokemon *p = (Pokemon *)JS_GetPrivate(cx, obj);
    int tid = JSVAL_TO_INT(id);
    switch (tid) {
        case PTI_HP: {
            p->setHp(JSVAL_TO_INT(*vp));
        } break;
    }
    return JS_TRUE;
}

JSBool PokemonGet(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    Pokemon *p = (Pokemon *)JS_GetPrivate(cx, obj);
    int tid = JSVAL_TO_INT(id);
    switch (tid) {
        case PTI_SPECIES: {
            string name = p->getSpeciesName();
            char *pstr = JS_strdup(cx, name.c_str());
            JSString *str = JS_NewString(cx, pstr, name.length());
            *vp = STRING_TO_JSVAL(str);
        } break;

        case PTI_NAME: {
            string name = p->getName();
            char *pstr = JS_strdup(cx, name.c_str());
            JSString *str = JS_NewString(cx, pstr, name.length());
            *vp = STRING_TO_JSVAL(str);
        } break;

        case PTI_BASE: {
            jsval arr[STAT_COUNT];
            for (int i = 0; i < STAT_COUNT; ++i) {
                arr[i] = INT_TO_JSVAL(p->getBaseStat((STAT)i));
            }
            JSObject *objArr = JS_NewArrayObject(cx, STAT_COUNT, arr);
            *vp = OBJECT_TO_JSVAL(objArr);
        } break;

        case PTI_IV: {
            jsval arr[STAT_COUNT];
            for (int i = 0; i < STAT_COUNT; ++i) {
                arr[i] = INT_TO_JSVAL(p->getIv((STAT)i));
            }
            JSObject *objArr = JS_NewArrayObject(cx, STAT_COUNT, arr);
            *vp = OBJECT_TO_JSVAL(objArr);
        } break;

        case PTI_EV: {
            jsval arr[STAT_COUNT];
            for (int i = 0; i < STAT_COUNT; ++i) {
                arr[i] = INT_TO_JSVAL(p->getEv((STAT)i));
            }
            JSObject *objArr = JS_NewArrayObject(cx, STAT_COUNT, arr);
            *vp = OBJECT_TO_JSVAL(objArr);
        } break;

        case PTI_STAT: {
            jsval arr[STAT_COUNT];
            for (int i = 0; i < STAT_COUNT; ++i) {
                arr[i] = INT_TO_JSVAL(p->getStat((STAT)i));
            }
            JSObject *objArr = JS_NewArrayObject(cx, STAT_COUNT, arr);
            *vp = OBJECT_TO_JSVAL(objArr);
        } break;

        case PTI_LEVEL: {
            *vp = INT_TO_JSVAL(p->getLevel());
        } break;

        case PTI_GENDER: {
            *vp = INT_TO_JSVAL(p->getGender());
        } break;

        case PTI_HP: {
            *vp = INT_TO_JSVAL(p->getHp());
        } break;
    }
    return JS_TRUE;
}

static JSPropertySpec pokemonProperties[] = {
    { "addStatus", 0, JSPROP_PERMANENT, NULL, NULL },
    { "species", PTI_SPECIES, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, NULL },
    { "name", PTI_NAME, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, NULL },
    { "base", PTI_BASE, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, NULL },
    { "iv", PTI_IV, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, NULL },
    { "ev", PTI_EV, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, NULL },
    { "stat", PTI_STAT, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, NULL },
    { "level", PTI_LEVEL, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, NULL },
    { "nature", PTI_NATURE, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, NULL },
    { "hp", PTI_HP, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, PokemonSet },
    { "types", PTI_TYPES, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, NULL },
    { "gender", PTI_GENDER, JSPROP_PERMANENT | JSPROP_SHARED, PokemonGet, NULL },
    { 0, 0, 0, 0, 0 }
};

static JSFunctionSpec pokemonFunctions[] = {
    JS_FS("applyStatus", applyStatus, 2, 0, 0),
    JS_FS_END
};

PokemonObject *ScriptContext::newPokemonObject(Pokemon *p) {
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
    PokemonObject *ret = new PokemonObject(obj);
    addRoot(ret);
    JS_DefineProperties(cx, obj, pokemonProperties);
    JS_DefineFunctions(cx, obj, pokemonFunctions);
    JS_SetPrivate(cx, obj, p);
    JS_EndRequest(cx);
    return ret;
}

}

