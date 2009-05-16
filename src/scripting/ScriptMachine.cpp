/* 
 * File:   ScriptMachine.cpp
 * Author: Catherine
 *
 * Created on March 31, 2009, 6:06 PM
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
#include <time.h>
#include <nspr/nspr.h>
#include <jsapi.h>
#include <set>
#include <fstream>
#include <iostream>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#include "ScriptMachine.h"
#include "../text/Text.h"
#include "../shoddybattle/PokemonSpecies.h"
#include "../moves/PokemonMove.h"

using namespace std;
using namespace boost;

namespace shoddybattle {

/* The class of the global object. */
static JSClass globalClass = {
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

typedef set<ScriptContext *> CONTEXT_SET;

static void reportError(JSContext *, const char *, JSErrorReport *);

struct GlobalState {
    Text text;
    SpeciesDatabase species;
    MoveDatabase moves;
    GlobalState(ScriptMachine *p): moves(*p) { }
};

struct ScriptMachineImpl {
    JSRuntime *runtime;
    JSObject *global;
    JSContext *cx;
    CONTEXT_SET contexts;
    set<ScriptObject *> roots;
    ScriptMachine *machine;
    GlobalState *state;
    mutex lock;         // lock for contexts set
    mutex rootLock;     // lock for roots set

    ScriptMachineImpl(ScriptMachine *p): machine(p) { }
    
    ScriptContext *newContext() {
        JSContext *cx = JS_NewContext(runtime, 8192);
        JS_SetGlobalObject(cx, global);
        JS_SetOptions(cx, JSOPTION_VAROBJFIX);
        JS_SetVersion(cx, JSVERSION_LATEST);
        JS_SetErrorReporter(cx, reportError);
        ScriptContext *context = new ScriptContext(cx);
        JS_SetContextPrivate(cx, context);
        context->m_machine = machine;
        return context;
    }
};

Text *ScriptMachine::getText() const {
    return &m_impl->state->text;
}
SpeciesDatabase *ScriptMachine::getSpeciesDatabase() const {
    return &m_impl->state->species;
}
MoveDatabase *ScriptMachine::getMoveDatabase() const {
    return &m_impl->state->moves;
}

static JSBool includeMoves(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *) {
    jsval v = argv[0];
    if (!JSVAL_IS_STRING(v)) {
        return JS_FALSE;
    }
    char *str = JS_GetStringBytes(JSVAL_TO_STRING(v));
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    jsrefcount ref = JS_SuspendRequest(cx);
    scx->getMachine()->includeMoves(str);
    JS_ResumeRequest(cx, ref);
    return JS_TRUE;
}

static JSBool include(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *) {
    jsval val = argv[0];
    JSString *str = JS_ValueToString(cx, val);
    char *pstr = JS_GetStringBytes(str);
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    scx->runFile(pstr);
    return JS_TRUE;
}

static JSBool includeSpecies(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *) {
    jsval v = argv[0];
    if (!JSVAL_IS_STRING(v)) {
        return JS_FALSE;
    }
    char *str = JS_GetStringBytes(JSVAL_TO_STRING(v));
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    jsrefcount ref = JS_SuspendRequest(cx);
    scx->getMachine()->includeSpecies(str);
    JS_ResumeRequest(cx, ref);
    return JS_TRUE;
}

class TextLookup {
public:
    TextLookup(JSContext *cx, ScriptContext *scx, ScriptFunction func) {
        m_cx = cx;
        m_scx = scx;
        m_func = func;
    }
    int operator()(const string name) {
        char *pstr = JS_strdup(m_cx, name.c_str());
        JSString *str = JS_NewString(m_cx, pstr, name.length());
        jsval jsv = STRING_TO_JSVAL(str);
        ScriptValue argv[1] = { ScriptValue((void *)jsv) };
        ScriptValue v = m_scx->callFunction(NULL, &m_func, 1, argv);
        return v.getInt();
    }
private:
    JSContext *m_cx;
    ScriptContext *m_scx;
    ScriptFunction m_func;
};

static JSBool getText(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *ret) {
    int32 category, text;
    JS_ConvertArguments(cx, 2, argv, "ii", &category, &text);
    const int count = argc - 2;
    const char *args[count];
    for (int i = 0; i < count; ++i) {
        jsval v = argv[2 + i];
        JSString *str = JS_ValueToString(cx, v);
        char *pstr = JS_GetStringBytes(str);
        args[i] = pstr;
    }
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    string sret = scx->getMachine()->getText(category, text, count, args);
    char *pstr = JS_strdup(cx, sret.c_str());
    JSString *str = JS_NewString(cx, pstr, sret.length());
    *ret = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

string ScriptMachine::getText(int i, int j, int argc, const char **argv) {
    return m_impl->state->text.getText(i, j, argc, argv);
}

static JSBool loadText(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *) {
    jsval v = argv[0];
    if (!JSVAL_IS_STRING(v)) {
        return JS_FALSE;
    }
    jsval v2 = argv[1];
    if (!JSVAL_IS_OBJECT(v2)) {
        return JS_FALSE;
    }
    ScriptFunction func(JSVAL_TO_OBJECT(v2));
    char *str = JS_GetStringBytes(JSVAL_TO_STRING(v));
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    TextLookup lookup(cx, scx, func);
    jsrefcount ref = JS_SuspendRequest(cx);
    try {
        scx->getMachine()->loadText(str, lookup);
    } catch (SyntaxException e) {
        cout << "loadText: Syntax error on line " << e.getLine() << endl;
    }
    JS_ResumeRequest(cx, ref);
    return JS_TRUE;
}

static JSBool populateMoveLists(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *) {
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    scx->getMachine()->populateMoveLists();
    return JS_TRUE;
}

static JSBool printFunction(JSContext *cx,
        JSObject *obj, uintN argc, jsval *argv, jsval *) {
    jsval v = argv[0];
    JSString *jsstr;
    if (!JSVAL_IS_STRING(v)) {
        jsstr = JS_ValueToString(cx, v);
    } else {
        jsstr = JSVAL_TO_STRING(v);
    }
        
    char *str = JS_GetStringBytes(jsstr);
    cout << str << endl;
    return JS_TRUE;
}

void ScriptMachine::populateMoveLists() {
    m_impl->state->species.populateMoveLists(m_impl->state->moves);
}

void ScriptMachine::includeSpecies(const std::string file) {
    m_impl->state->species.loadSpecies(file);
}

void ScriptMachine::loadText(const std::string file, TextLookup &func) {
    m_impl->state->text.loadFile(file, func);
}

void ScriptMachine::includeMoves(const std::string file) {
    m_impl->state->moves.loadMoves(file);
}

static void reportError(JSContext *cx, const char *message,
        JSErrorReport *report) {
    fprintf(stderr, "%s:%u:%s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message);
}

ScriptValue ScriptArray::operator[](const int i) {
    JSContext *cx = (JSContext *)m_cx->m_p;
    JS_BeginRequest(cx);
    jsval val;
    JS_GetElement(cx, (JSObject *)m_p, i, &val);
    JS_EndRequest(cx);
    return ScriptValue((void *)val);
}

ScriptValue::ScriptValue(const ScriptObject *object) {
    m_fail = false;
    m_val = (void *)OBJECT_TO_JSVAL(object->getObject());
}

ScriptValue::ScriptValue(int i) {
    m_fail = false;
    m_val = (void *)INT_TO_JSVAL(i);
}

ScriptValue::ScriptValue(bool b) {
    m_fail = false;
    m_val = (void *)BOOLEAN_TO_JSVAL(b);
}

int ScriptValue::getInt() const {
    return JSVAL_TO_INT((jsval)m_val);
}

double ScriptValue::getDouble(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    jsval val = (jsval)m_val;
    jsdouble p;
    JS_ValueToNumber(cx, val, &p);
    JS_EndRequest(cx);
    return p;
}

ScriptObject ScriptValue::getObject() const {
    JSObject *obj = JSVAL_TO_OBJECT((jsval)m_val);
    return ScriptObject(obj);
}

bool ScriptValue::getBool() const {
    return JSVAL_TO_BOOLEAN((jsval)m_val);
}

/**
 * Get an ability object by looking in the Ability property of the global
 * object for a property by the name of the ability.
 */
StatusObject ScriptContext::getAbility(const string name) const {
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    jsval val;
    JS_GetProperty(cx, m_machine->m_impl->global, "Ability", &val);
    JSObject *obj = JSVAL_TO_OBJECT(val);
    JSBool has;
    JS_HasProperty(cx, obj, name.c_str(), &has);
    StatusObject ret(NULL);
    if (has) {
        JS_GetProperty(cx, obj, name.c_str(), &val);
        ret = StatusObject(JSVAL_TO_OBJECT(val));
    }
    JS_EndRequest(cx);
    return ret;
}

bool ScriptContext::hasProperty(ScriptObject *obj, const string name) const {
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    JSBool ret;
    JS_HasProperty(cx, (JSObject *)obj->getObject(), name.c_str(), &ret);
    if (ret) {
        jsval val;
        JS_GetProperty(cx, (JSObject *)obj->getObject(), name.c_str(), &val);
        ret = !JSVAL_IS_NULL(val);
    }
    JS_EndRequest(cx);
    return ret;
}

ScriptValue ScriptContext::callFunctionByName(ScriptObject *sobj,
        const string name,
        const int argc, ScriptValue *sargv) {
    JSObject *obj = sobj ? (JSObject *)sobj->getObject() : NULL;
    jsval argv[argc];
    for (int i = 0; i < argc; ++i) {
        argv[i] = (jsval)sargv[i].getValue();
    }
    jsval ret;
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    JSBool b = JS_CallFunctionName(cx, obj, name.c_str(), argc, argv, &ret);
    JS_EndRequest(cx);
    if (!b) {
        ScriptValue v;
        v.setFailure();
        return v;
    }
    return ScriptValue((void *)ret);
}

ScriptValue ScriptContext::callFunction(ScriptObject *sobj,
        const ScriptFunction *sfunc,
        const int argc, ScriptValue *sargv) {
    JSObject *obj = sobj ? (JSObject *)sobj->getObject() : NULL;
    JSFunction *func = (JSFunction *)sfunc->getObject();
    jsval argv[argc];
    for (int i = 0; i < argc; ++i) {
        argv[i] = (jsval)sargv[i].getValue();
    }
    jsval ret;
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    JS_CallFunction(cx, obj, func, argc, argv, &ret);
    JS_EndRequest(cx);
    return ScriptValue((void *)ret);
}

ScriptContext::ScriptContext(void *p) {
    m_p = p;
    m_busy = false;
}

ScriptObject::ScriptObject(const ScriptObject &rhs) {
    if (rhs.m_root) {
        // Cannot make a copy of a root.
        throw ScriptMachineException();
    }
    m_root = false;
    m_p = rhs.m_p;
}

ScriptObject &ScriptObject::operator=(const ScriptObject &rhs) {
    if (m_root) {
        // Cannot alter a root.
        throw ScriptMachineException();
    }
    m_root = false;
    m_p = rhs.m_p;
    return *this;
}

void ScriptContext::addRoot(ScriptObject *sobj) {
    if (sobj->m_root) {
        // Already a root.
        throw ScriptMachineException();
    }
    void **obj = sobj->getObjectRef();
    if (obj) {
        JSContext *cx = (JSContext *)m_p;
        JS_BeginRequest(cx);
        JS_AddRoot(cx, obj);
        JS_EndRequest(cx);
        sobj->m_root = true;
        // need external synchronisation for a set
        lock_guard<mutex> guard(m_machine->m_impl->rootLock);
        set<ScriptObject *> &s = m_machine->m_impl->roots;
        s.insert(sobj); // Assume ownership of this ScriptObject
    }
}

void ScriptContext::removeRoot(ScriptObject *sobj) {
    if (!sobj->m_root) {
        // Not a root.
        throw ScriptMachineException();
    }
    void **obj = sobj->getObjectRef();
    if (obj) {
        JSContext *cx = (JSContext *)m_p;
        JS_BeginRequest(cx);
        JS_RemoveRoot(cx, obj);
        JS_EndRequest(cx);
        sobj->m_root = false;
        // need external synchronisation for a set
        lock_guard<mutex> guard(m_machine->m_impl->rootLock);
        set<ScriptObject *> &s = m_machine->m_impl->roots;
        s.erase(sobj);
        // We own this ScriptObject, so delete it.
        delete sobj;
    }
}

ScriptFunction *ScriptContext::compileFunction(
        const vector<string> args,
        const string body,
        const string file,
        const int line) {
    const int argCount = args.size();
    const char *params[argCount];
    for (int i = 0; i < argCount; ++i) {
        params[i] = args[i].c_str();
    }
    const char *pBody = body.c_str();
    const int bodyLength = body.length();

    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    JSFunction *func = JS_CompileFunction(cx, NULL, NULL, argCount, params,
            pBody, bodyLength, file.c_str(), line);
    ScriptFunction *ret = new ScriptFunction(func);
    addRoot(ret);
    JS_EndRequest(cx);

    return ret;
}

void ScriptContext::gc() {
    JS_GC((JSContext *)m_p);
}

void ScriptContext::maybeGc() {
    JS_MaybeGC((JSContext *)m_p);
}

/**
 * Run a file in the scope of the global object.
 */
void ScriptContext::runFile(const string file) {
    // Read in the whole file.
    ifstream is(file.c_str());
    if (!is.is_open()) {
        cout << "Cannot find script " << file << endl;
        return;
    }
    is.seekg(0, ios::end);
    const int length = is.tellg();
    is.seekg(0, ios::beg);
    char text[length];
    is.read(text, length);
    is.close();

    // Execute the script.
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    jsval val;
    JS_EvaluateScript(cx,
            m_machine->m_impl->global,
            text,
            length,
            file.c_str(),
            0,
            &val);
    JS_EndRequest(cx);
}

bool isAvailable(ScriptContext *cx) {
    return !cx->isBusy();
}

ScriptContext *ScriptMachine::acquireContext() {
    // Need external synchronisation for a set.
    lock_guard<mutex> guard(m_impl->lock);
    CONTEXT_SET &contexts = m_impl->contexts;

    // First look for an existing context that isn't busy.
    CONTEXT_SET::iterator i =
            find_if(contexts.begin(), contexts.end(), isAvailable);
    if (i != contexts.end()) {
        ScriptContext *cx = *i;
        cx->m_busy = true;
        return cx;
    }

    // Otherwise, we need to make a new context.
    ScriptContext *context = m_impl->newContext();
    context->m_busy = true;
    contexts.insert(context);
    return context;
}

void ScriptMachine::releaseContext(ScriptContext *cx) {
    // Need external synchronisation for cx->m_busy.
    lock_guard<mutex> guard(m_impl->lock);
    cx->m_busy = false;
}

static JSFunctionSpec globalFunctions[] = {
    JS_FS("print", printFunction, 1, 0, 0),
    JS_FS("loadText", loadText, 2, 0, 0),
    JS_FS("getText", getText, 2, 0, 0),
    JS_FS("includeMoves", includeMoves, 1, 0, 0),
    JS_FS("includeSpecies", includeSpecies, 1, 0, 0),
    JS_FS("populateMoveLists", populateMoveLists, 0, 0, 0),
    JS_FS("include", include, 1, 0, 0),
    JS_FS_END
};

ScriptMachine::ScriptMachine() throw(ScriptMachineException) {
    m_impl = new ScriptMachineImpl(this);

    m_impl->runtime = JS_NewRuntime(8L * 1024L * 1024L);
    if (m_impl->runtime == NULL) {
        delete m_impl;
        throw ScriptMachineException();
    }

    m_impl->cx = JS_NewContext(m_impl->runtime, 8192);
    if (m_impl->cx == NULL) {
        JS_DestroyRuntime(m_impl->runtime);
        delete m_impl;
        throw ScriptMachineException();
    }

    JS_SetOptions(m_impl->cx, JSOPTION_VAROBJFIX);
    JS_SetVersion(m_impl->cx, JSVERSION_LATEST);
    JS_SetErrorReporter(m_impl->cx, reportError);

    JS_BeginRequest(m_impl->cx);
    m_impl->global = JS_NewObject(m_impl->cx, &globalClass, NULL, NULL);
    JS_EndRequest(m_impl->cx);

    if (m_impl->global == NULL) {
        JS_DestroyContext(m_impl->cx);
        JS_DestroyRuntime(m_impl->runtime);
        delete m_impl;
        throw ScriptMachineException();
    }

    JS_BeginRequest(m_impl->cx);
    JS_InitStandardClasses(m_impl->cx, m_impl->global);
    JS_DefineFunctions(m_impl->cx, m_impl->global, globalFunctions);
    JS_EndRequest(m_impl->cx);

    m_impl->state = new GlobalState(this);
}

ScriptMachine::~ScriptMachine() {
    delete m_impl->state;
    CONTEXT_SET::iterator i = m_impl->contexts.begin();
    for (; i != m_impl->contexts.end(); ++i) {
        ScriptContext *cx = *i;
        JS_DestroyContext((JSContext *)cx->m_p);
        delete cx;
    }
    int roots = m_impl->roots.size();
    if (roots != 0) {
        cout << "Warning: " << roots << " unremoved roots." << endl;
        cout << "This indicates a failure to remove a root somewhere.";
        cout << endl;
    }
    set<ScriptObject *>::iterator j = m_impl->roots.begin();
    for (; j != m_impl->roots.end(); ++j) {
        delete *j;
        // No need to call removeRoot() since we are about to destroy
        // the runtime anyway.
    }
    JS_DestroyContext(m_impl->cx);
    JS_DestroyRuntime(m_impl->runtime);
    delete m_impl;
}

} // namespace shoddybattle

/**#include <iostream>

using namespace shoddybattle;

int main() {
    ScriptMachine machine;
    ScriptContext *cx = machine.acquireContext();
    vector<string> args;
    args.push_back("x");
    ScriptFunction func = cx->compileFunction(args, "print(x + 5);", "test.js", 1);
    if (!func.isNull()) {
        cout << "Not null!" << endl;
    }
    machine.releaseContext(cx);
}**/

