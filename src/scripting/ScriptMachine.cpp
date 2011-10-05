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
#include <js/jsapi.h>
#include <set>
#include <fstream>
#include <iostream>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/bind.hpp>

#include "ScriptMachine.h"
#include "../text/Text.h"
#include "../shoddybattle/Pokemon.h"
#include "../shoddybattle/PokemonSpecies.h"
#include "../moves/PokemonMove.h"
#include "../network/ThreadedQueue.h"
#include "../main/Log.h"

using namespace std;
using namespace boost;

#define ENABLE_ROOT_COUNT 0

// Support forward compatibility with the development version of Spidermonkey.
#ifndef JS_TYPED_ROOTING_API
inline JSBool JS_AddObjectRoot(JSContext *cx, JSObject **rp) {
    return JS_AddRoot(cx, rp);
}
inline JSBool JS_RemoveObjectRoot(JSContext *cx, JSObject **rp) {
    return JS_RemoveRoot(cx, rp);
}
#endif

namespace shoddybattle {

/* The class of the global object. */
static JSClass globalClass = {
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
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

typedef network::ThreadedQueue<ScriptObject *> RootQueue;
typedef shared_ptr<RootQueue> RootQueuePtr;

struct ScriptMachineImpl {
    JSRuntime *runtime;
    JSObject *global;
    JSContext *cx;
    CONTEXT_SET contexts;
    ScriptMachine *machine;
    GlobalState *state;
    mutex lock;         // lock for contexts set
    RootQueuePtr deadRoots;
    ScriptContextPtr deadRootContext;

#if ENABLE_ROOT_COUNT
    mutex rootLock;     // lock for roots set
    unsigned int roots;
#endif

    ScriptMachineImpl(ScriptMachine *p):
            machine(p) {
#if ENABLE_ROOT_COUNT
        roots = 0;
#endif
    }

    void startRootThread() {
        deadRoots = RootQueuePtr(new RootQueue(
                boost::bind(&ScriptMachineImpl::reclaimRoot, this, _1)));
        deadRoots->post(boost::bind(
                &ScriptMachineImpl::initialiseRootThread, this));
    }

    void initialiseRootThread() {
        deadRootContext = machine->acquireContext();
    }

    void terminateRootThread() {
        deadRoots->post(boost::bind(
                &ScriptMachineImpl::terminateRootThreadJob, this));
        deadRoots.reset();
    }
    
    void terminateRootThreadJob() {
        deadRootContext.reset();
    }

    void reclaimRoot(ScriptObject *sobj) {
        JSObject **obj = reinterpret_cast<JSObject **>(sobj->getObjectRef());
        assert(obj);
        JSContext *cx = (JSContext *)deadRootContext->m_p;
        JS_BeginRequest(cx);
        JS_RemoveObjectRoot(cx, obj);
        JS_EndRequest(cx);
        // We own this ScriptObject, so delete it.
        delete sobj;
#if ENABLE_ROOT_COUNT
        lock_guard<mutex> guard(rootLock);
        --roots;
        Log::out() << "There are " << roots << " roots remaining." << endl;
#endif
    }
    
    ScriptContext *newContext() {
        JSContext *cx = JS_NewContext(runtime, 8192);
        JS_BeginRequest(cx);
        JS_SetGlobalObject(cx, global);
        JS_SetOptions(cx, JSOPTION_VAROBJFIX);
        JS_SetVersion(cx, JSVERSION_LATEST);
        JS_SetErrorReporter(cx, reportError);
        JS_EndRequest(cx);
        ScriptContext *context = new ScriptContext(cx);
        JS_SetContextPrivate(cx, context);
        context->m_machine = machine;
        return context;
    }

    void releaseContext(ScriptContext *cx) {
        // Need external synchronisation for cx->m_busy.
        lock_guard<mutex> guard(lock);
        cx->m_busy = false;
        cx->clearContextThread();
    }

    StatusObject getSpecialStatus(JSContext *cx,
            const string &type, const string &name) {
        JS_BeginRequest(cx);
        jsval val;
        JS_GetProperty(cx, global, type.c_str(), &val);
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

    void getStatusList(JSContext *cx, const string &type,
            vector<StatusObject> &list) {
        JS_BeginRequest(cx);
        jsval val;
        JS_GetProperty(cx, global, type.c_str(), &val);
        JSObject *obj = JSVAL_TO_OBJECT(val); // Clause object
        JSIdArray *ids = JS_Enumerate(cx, obj);
        if (ids) {
            for (int i = 0; i < ids->length; ++i) {
                JS_GetPropertyById(cx, obj, ids->vector[i], &val);
                if (JSVAL_IS_OBJECT(val)) {
                    JSBool inst;
                    if (JS_HasInstance(cx, obj, val, &inst) && inst) {
                        JSObject *entry = JSVAL_TO_OBJECT(val);
                        list.push_back(StatusObject(entry));
                    }
                }
            }
            JS_DestroyIdArray(cx, ids);
        }
        JS_EndRequest(cx);
    }
};

unsigned int ScriptMachine::getRootCount() const {
#if ENABLE_ROOT_COUNT
    return m_impl->roots;
#else
    return 0;
#endif
}

Text *ScriptMachine::getText() const {
    return &m_impl->state->text;
}
SpeciesDatabase *ScriptMachine::getSpeciesDatabase() const {
    return &m_impl->state->species;
}
MoveDatabase *ScriptMachine::getMoveDatabase() const {
    return &m_impl->state->moves;
}

static JSBool includeMoves(JSContext *cx, uintN /*argc*/, jsval *argv) {
    jsval v = argv[0];
    if (!JSVAL_IS_STRING(v)) {
        return JS_FALSE;
    }
    char *str = JS_EncodeString(cx, JSVAL_TO_STRING(v));
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    jsrefcount ref = JS_SuspendRequest(cx);
    scx->getMachine()->includeMoves(str);
    JS_ResumeRequest(cx, ref);
    return JS_TRUE;
}

static JSBool include(JSContext *cx, uintN /*argc*/, jsval *argv) {
    jsval val = argv[0];
    JSString *str = JS_ValueToString(cx, val);
    char *pstr = JS_EncodeString(cx, str);
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    scx->runFile(pstr);
    return JS_TRUE;
}

static JSBool includeSpecies(JSContext *cx, uintN /*argc*/, jsval *argv) {
    jsval v = argv[0];
    if (!JSVAL_IS_STRING(v)) {
        return JS_FALSE;
    }
    char *str = JS_EncodeString(cx, JSVAL_TO_STRING(v));
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
        JSString *str = JS_NewStringCopyN(m_cx, pstr, name.length());
        jsval jsv = STRING_TO_JSVAL(str);
        ScriptValue argv[1] = { ScriptValue(JSVAL_TO_PRIVATE(jsv)) };
        ScriptValue v = m_scx->callFunction(NULL, &m_func, 1, argv);
        return v.getInt();
    }
private:
    JSContext *m_cx;
    ScriptContext *m_scx;
    ScriptFunction m_func;
};

static JSBool getText(JSContext *cx, uintN argc, jsval *argv) {
    int32 category, text;
    JS_ConvertArguments(cx, 2, argv, "ii", &category, &text);
    const int count = argc - 2;
    const char *args[count];
    for (int i = 0; i < count; ++i) {
        jsval v = argv[2 + i];
        JSString *str = JS_ValueToString(cx, v);
        char *pstr = JS_EncodeString(cx, str);
        args[i] = pstr;
    }
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    string sret = scx->getMachine()->getText(category, text, count, args);
    char *pstr = JS_strdup(cx, sret.c_str());
    JSString *str = JS_NewStringCopyN(cx, pstr, sret.length());
    JS_SET_RVAL(cx, argv, STRING_TO_JSVAL(str));
    return JS_TRUE;
}

string ScriptMachine::getText(int i, int j, int argc, const char **argv) {
    return m_impl->state->text.getText(i, j, argc, argv);
}

static JSBool loadText(JSContext *cx, uintN /*argc*/, jsval *argv) {
    jsval v = argv[0];
    if (!JSVAL_IS_STRING(v)) {
        return JS_FALSE;
    }
    jsval v2 = argv[1];
    if (!JSVAL_IS_OBJECT(v2)) {
        return JS_FALSE;
    }
    ScriptFunction func(JSVAL_TO_OBJECT(v2));
    char *str = JS_EncodeString(cx, JSVAL_TO_STRING(v));
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    TextLookup lookup(cx, scx, func);
    try {
        scx->getMachine()->loadText(str, lookup);
    } catch (SyntaxException e) {
        Log::out() << "loadText: Syntax error on line " << e.getLine() << endl;
    }
    return JS_TRUE;
}

static JSBool populateMoveLists(JSContext *cx, uintN /*argc*/, jsval * /*argv*/) {
    ScriptContext *scx = (ScriptContext *)JS_GetContextPrivate(cx);
    scx->getMachine()->populateMoveLists();
    return JS_TRUE;
}

static JSBool printFunction(JSContext *cx, uintN /*argc*/, jsval *argv) {
    jsval v = argv[0];
    JSString *jsstr;
    if (!JSVAL_IS_STRING(v)) {
        jsstr = JS_ValueToString(cx, v);
    } else {
        jsstr = JSVAL_TO_STRING(v);
    }
        
    char *str = JS_EncodeString(cx, jsstr);
    Log::out() << str << endl;
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

static void reportError(JSContext * /*cx*/, const char *message,
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
    return ScriptValue(JSVAL_TO_PRIVATE(val));
}

ScriptArrayPtr ScriptArray::newTeamArray(const Pokemon::ARRAY &team,
        ScriptContext *scx) {
    const int length = team.size();
    jsval array[length];
    for (int i = 0; i < length; ++i) {
        ScriptObject *obj = team[i]->getObject();
        array[i] = OBJECT_TO_JSVAL((JSObject *)obj->getObject());
    }
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    JSObject *obj = JS_NewArrayObject(cx, length, array);
    ScriptArrayPtr ret = scx->addRoot(new ScriptArray(obj, scx));
    JS_EndRequest(cx);
    return ret;
}

ScriptValue::ScriptValue(const ScriptObject *object) {
    m_fail = false;
    m_val = JSVAL_TO_PRIVATE(OBJECT_TO_JSVAL((JSObject *)object->getObject()));
}

ScriptValue::ScriptValue(int i) {
    m_fail = false;
    m_val = JSVAL_TO_PRIVATE(INT_TO_JSVAL(i));
}

ScriptValue::ScriptValue(bool b) {
    m_fail = false;
    m_val = JSVAL_TO_PRIVATE(BOOLEAN_TO_JSVAL(b));
}

int ScriptValue::getInt() const {
    return JSVAL_TO_INT(PRIVATE_TO_JSVAL(m_val));
}

bool ScriptValue::isNull() const {
    return JSVAL_IS_NULL(PRIVATE_TO_JSVAL(m_val));
}

double ScriptValue::getDouble(ScriptContext *scx) const {
    JSContext *cx = (JSContext *)scx->m_p;
    JS_BeginRequest(cx);
    jsval val = PRIVATE_TO_JSVAL(m_val);
    jsdouble p;
    JS_ValueToNumber(cx, val, &p);
    JS_EndRequest(cx);
    return p;
}

ScriptObject ScriptValue::getObject() const {
    JSObject *obj = JSVAL_TO_OBJECT(PRIVATE_TO_JSVAL(m_val));
    return ScriptObject(obj);
}

bool ScriptValue::getBool() const {
    return JSVAL_TO_BOOLEAN(PRIVATE_TO_JSVAL(m_val));
}

StatusObject ScriptContext::getAbility(const string &name) const {
    JSContext *cx = (JSContext *)m_p;
    return m_machine->m_impl->getSpecialStatus(cx, "Ability", name);
}

StatusObject ScriptContext::getItem(const string &name) const {
    JSContext *cx = (JSContext *)m_p;
    return m_machine->m_impl->getSpecialStatus(cx, "HoldItem", name);
}

StatusObject ScriptContext::getClause(const string &name) const {
    JSContext *cx = (JSContext *)m_p;
    return m_machine->m_impl->getSpecialStatus(cx, "Clause", name);
}

void ScriptContext::getClauseList(vector<StatusObject> &clauses) const {
    JSContext *cx = (JSContext *)m_p;
    m_machine->m_impl->getStatusList(cx, "Clause", clauses);
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
        argv[i] = PRIVATE_TO_JSVAL(sargv[i].getValue());
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
    return ScriptValue(JSVAL_TO_PRIVATE(ret));
}

ScriptValue ScriptContext::callFunction(ScriptObject *sobj,
        const ScriptFunction *sfunc,
        const int argc, ScriptValue *sargv) {
    JSObject *obj = sobj ? (JSObject *)sobj->getObject() : NULL;
    JSFunction *func = (JSFunction *)sfunc->getObject();
    jsval argv[argc];
    for (int i = 0; i < argc; ++i) {
        argv[i] = PRIVATE_TO_JSVAL(sargv[i].getValue());
    }
    jsval ret;
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    JS_CallFunction(cx, obj, func, argc, argv, &ret);
    JS_EndRequest(cx);
    return ScriptValue(JSVAL_TO_PRIVATE(ret));
}

ScriptContext::ScriptContext(void *p) {
    m_p = p;
    m_busy = false;
}

ScriptObject::ScriptObject(const ScriptObject &rhs) {
    m_p = rhs.m_p;
}

ScriptObject &ScriptObject::operator=(const ScriptObject &rhs) {
    m_p = rhs.m_p;
    return *this;
}

bool ScriptContext::makeRoot(ScriptObject *sobj) {
    JSObject **obj = reinterpret_cast<JSObject **>(sobj->getObjectRef());
    if (!obj) {
        return false;
    }
    JSContext *cx = (JSContext *)m_p;
    JS_BeginRequest(cx);
    JS_AddObjectRoot(cx, obj);
    JS_EndRequest(cx);
#if ENABLE_ROOT_COUNT
    lock_guard<mutex> guard(m_machine->m_impl->rootLock);
    ++m_machine->m_impl->roots;
#endif
    return true;
}

void ScriptContext::removeRoot(ScriptMachine *machine, ScriptObject *sobj) {
    machine->m_impl->deadRoots->post(sobj);
}

shared_ptr<ScriptFunction> ScriptContext::compileFunction(
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
    shared_ptr<ScriptFunction> ret = addRoot(new ScriptFunction(func));
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
        Log::out() << "Cannot find script " << file << endl;
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

ScriptContextPtr ScriptMachine::acquireContext() {
    // Need external synchronisation for a set.
    lock_guard<mutex> guard(m_impl->lock);
    CONTEXT_SET &contexts = m_impl->contexts;

    // First look for an existing context that isn't busy.
    CONTEXT_SET::iterator i =
            find_if(contexts.begin(), contexts.end(), isAvailable);
    if (i != contexts.end()) {
        ScriptContext *cx = *i;
        cx->m_busy = true;
        cx->setContextThread(0);
        return ScriptContextPtr(cx,
                boost::bind(&ScriptMachineImpl::releaseContext, m_impl, _1));
    }

    // Otherwise, we need to make a new context.
    ScriptContext *context = m_impl->newContext();
    context->m_busy = true;
    contexts.insert(context);
    return ScriptContextPtr(context,
            boost::bind(&ScriptMachineImpl::releaseContext, m_impl, _1));
}

int ScriptContext::clearContextThread() {
    const int depth = JS_SuspendRequest((JSContext *)m_p);
    JS_ClearContextThread((JSContext *)m_p);
    return depth;
}

void ScriptContext::setContextThread(const int depth) {
    JS_SetContextThread((JSContext *)m_p);
    JS_ResumeRequest((JSContext *)m_p, depth);
}

void ScriptMachine::finalise() {
    getSpeciesDatabase()->verifyAbilities(this);
}

static JSFunctionSpec globalFunctions[] = {
    JS_FS("print", &printFunction, 1, 0),
    JS_FS("loadText", &loadText, 2, 0),
    JS_FS("getText", &getText, 2, 0),
    JS_FS("includeMoves", &includeMoves, 1, 0),
    JS_FS("includeSpecies", &includeSpecies, 1, 0),
    JS_FS("populateMoveLists", &populateMoveLists, 0, 0),
    JS_FS("include", &include, 1, 0),
    JS_FS_END
};

ScriptMachine::ScriptMachine() throw(ScriptMachineException) {
    m_impl = new ScriptMachineImpl(this);

    m_impl->runtime = JS_NewRuntime(512L * 1024L * 1024L);
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

    //JS_SetGCZeal(m_impl->cx, 2);

    JS_BeginRequest(m_impl->cx);
    m_impl->global = JS_NewGlobalObject(m_impl->cx, &globalClass);
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

    m_impl->startRootThread();
    m_impl->state = new GlobalState(this);
}

ScriptMachine::~ScriptMachine() {
    delete m_impl->state;
    m_impl->terminateRootThread();
    CONTEXT_SET::iterator i = m_impl->contexts.begin();
    for (; i != m_impl->contexts.end(); ++i) {
        ScriptContext *cx = *i;
        if (cx->isBusy()) {
            Log::out() << "Error: Busy context in ~ScriptMachine()." << endl;
        }
        JS_SetContextThread((JSContext *)cx->m_p);
        JS_DestroyContext((JSContext *)cx->m_p);
        delete cx;
    }
    JS_SetContextThread(m_impl->cx);
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
        Log::out() << "Not null!" << endl;
    }
    machine.releaseContext(cx);
}**/

