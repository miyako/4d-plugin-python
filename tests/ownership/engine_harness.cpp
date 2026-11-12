/*
 * Synthetic 4D engine boundary for ownership regressions.
 *
 * Compiles the real plugin source (4DPlugin-python.cpp, included below so its
 * static helpers are reachable) and links the real support/4DPlugin-JSON.cpp,
 * the bundled, unmodified 4D Plugin API (4DPluginAPI.c) and the bundled
 * jsoncpp. Only gCall4D is replaced, and the embedded Python interpreter is
 * stubbed at its C API boundary: no Python code is ever run. A test supplies
 * the PyObject a "script" evaluates to.
 *
 * Every engine resource (Unistrings, objects/collections, handles) is a real
 * heap block tracked in a live set, so leaks are detected exactly and a
 * dispose of something the engine no longer owns is reported by name instead
 * of being freed a second time.
 *
 * Engine semantics modelled (per the SDK contract):
 *  - PA_GetStringVariable returns a view borrowed from the variable.
 *  - PA_SetObjectProperty/PA_SetCollectionElement deep-copy strings and
 *    retain objects/collections; the caller still owns its variable.
 *  - PA_ExecuteCommandByID (JSON Stringify, New Object/Collection) and
 *    PA_ExecuteMethodByID return an owned result that must be cleared.
 *  - PA_ExecuteMethodByID borrows its parameters.
 */

/* Resolved through -I <source-root>, so the same harness tests baseline and fixed trees. */
#include "4DPlugin-python.cpp"

#include "longintrepr.h"
#include "PrivateTypes.h"
#include "EntryPoints.h"

#include <cstdarg>
#include <functional>
#include <map>
#include <set>
#include <vector>

extern "C" Call4DProcPtr gCall4D;

namespace {

typedef std::basic_string<PA_Unichar> u16;

struct Obj {
    int rc;
    bool coll;
    std::vector<std::pair<u16, PA_Variable>> props;
    std::vector<PA_Variable> items;
};

struct Handle { char *data; size_t size; };

std::set<void *> g_strings;
std::set<Obj *> g_objects;
std::set<void *> g_handles;
std::vector<std::string> g_violations;

void violation(const std::string &what) {
    fprintf(stderr, "  ENGINE CONTRACT: %s\n", what.c_str());
    g_violations.push_back(what);
}

[[noreturn]] void die(const char *msg) {
    fprintf(stderr, "HARNESS ERROR: %s\n", msg);
    fflush(stderr);
    abort();
}

std::string narrow(const u16 &s) { std::string r; for(PA_Unichar c : s) r.push_back((char)c); return r; }
u16 widen(const std::string &s) { u16 r; for(char c : s) r.push_back((PA_Unichar)(unsigned char)c); return r; }

PA_Unistring new_string(const PA_Unichar *src, size_t len) {
    PA_Unichar *buf = (PA_Unichar *)malloc((len + 1) * sizeof(PA_Unichar));
    if(len) memcpy(buf, src, len * sizeof(PA_Unichar));
    buf[len] = 0;
    g_strings.insert(buf);
    PA_Unistring s;
    s.fLength = (PA_long32)len;
    s.fString = buf;
    s.fReserved1 = 0;
    s.fReserved2 = 0;
    return s;
}

void free_string(PA_Unistring *s) {
    if(!s->fString) return;
    if(!g_strings.erase(s->fString)) {
        violation("double_dispose_string");
    } else {
        free(s->fString);
    }
    s->fString = 0;
    s->fLength = 0;
}

Handle *new_handle(size_t size) {
    Handle *h = (Handle *)malloc(sizeof(Handle));
    h->size = size;
    h->data = (char *)malloc(size ? size : 1);
    g_handles.insert(h);
    return h;
}

void free_handle(Handle *h) {
    if(!h) return;
    if(!g_handles.erase(h)) { violation("double_dispose_handle"); return; }
    free(h->data);
    free(h);
}

Handle *live_handle(void *p) {
    if(!g_handles.count(p)) die("used an unknown handle");
    return (Handle *)p;
}

void release_object(Obj *o);

void clear_var(PA_Variable *v) {
    switch(v->fType) {
        case eVK_Unistring: free_string(&v->uValue.fString); break;
        case eVK_Blob: free_handle((Handle *)v->uValue.fBlob.fHandle); break;
        case eVK_Object: if(v->uValue.fObject) release_object((Obj *)v->uValue.fObject); break;
        case eVK_Collection: if(v->uValue.fCollection) release_object((Obj *)v->uValue.fCollection); break;
        default: break;
    }
    v->fType = eVK_Undefined;
}

Obj *live(void *p) {
    if(!g_objects.count((Obj *)p)) die("used a dead object/collection");
    return (Obj *)p;
}

PA_Variable undefined_var() { PA_Variable v; memset(&v, 0, sizeof(v)); v.fType = eVK_Undefined; return v; }

PA_Variable copy_var(const PA_Variable &src) {
    PA_Variable dst = src;
    switch(src.fType) {
        case eVK_Unistring:
            if(!g_strings.count(src.uValue.fString.fString)) die("stored a dead string");
            dst.uValue.fString = new_string(src.uValue.fString.fString, src.uValue.fString.fLength);
            break;
        case eVK_Object:
            if(src.uValue.fObject) live(src.uValue.fObject)->rc++;
            break;
        case eVK_Collection:
            if(src.uValue.fCollection) live(src.uValue.fCollection)->rc++;
            break;
        default: break;
    }
    return dst;
}

Obj *new_object(bool coll = false) {
    Obj *o = new Obj();
    o->rc = 1;
    o->coll = coll;
    g_objects.insert(o);
    return o;
}

void release_object(Obj *o) {
    if(!g_objects.count(o)) { violation("double_release_object"); return; }
    if(--o->rc == 0) {
        for(auto &kv : o->props) clear_var(&kv.second);
        for(auto &v : o->items) clear_var(&v);
        g_objects.erase(o);
        delete o;
    }
}

u16 key_of(const PA_Unistring &k) { return u16(k.fString, k.fLength); }

PA_Variable *find_prop(Obj *o, const u16 &k) {
    for(auto &kv : o->props) if(kv.first == k) return &kv.second;
    return nullptr;
}

PA_Variable object_result(Obj *o) {
    PA_Variable v = undefined_var();
    v.fType = o->coll ? eVK_Collection : eVK_Object;
    if(o->coll) v.uValue.fCollection = (PA_CollectionRef)o; else v.uValue.fObject = (PA_ObjectRef)o;
    return v;
}

PA_Variable text_result(const char *s) {
    PA_Variable v = undefined_var();
    u16 w = widen(s);
    v.fType = eVK_Unistring;
    v.uValue.fString = new_string(w.c_str(), w.size());
    return v;
}

/* ---------- JSON Stringify model (ASCII only) ---------- */

void json_of(const PA_Variable &v, std::string &out);

void json_of_obj(Obj *o, std::string &out) {
    if(o->coll) {
        out += '[';
        for(size_t i = 0; i < o->items.size(); i++) { if(i) out += ','; json_of(o->items[i], out); }
        out += ']';
    } else {
        out += '{';
        bool first = true;
        for(auto &kv : o->props) {
            if(!first) out += ',';
            first = false;
            out += '"' + narrow(kv.first) + "\":";
            json_of(kv.second, out);
        }
        out += '}';
    }
}

void json_of(const PA_Variable &v, std::string &out) {
    char buf[64];
    switch(v.fType) {
        case eVK_Unistring: out += '"' + narrow(u16(v.uValue.fString.fString, v.uValue.fString.fLength)) + '"'; break;
        case eVK_Real: snprintf(buf, sizeof(buf), "%.17g", v.uValue.fReal); out += buf; break;
        case eVK_Longint: snprintf(buf, sizeof(buf), "%d", (int)v.uValue.fLongint); out += buf; break;
        case eVK_Boolean: out += v.uValue.fBoolean ? "true" : "false"; break;
        case eVK_Object: json_of_obj(live(v.uValue.fObject), out); break;
        case eVK_Collection: json_of_obj(live(v.uValue.fCollection), out); break;
        default: out += "null"; break;
    }
}

int g_stringify_calls = 0;

void execute_command(EngineBlock *eb) {
    PA_Variable **params = (PA_Variable **)eb->fPtr1;
    PA_Variable *ret = (PA_Variable *)eb->fPtr2;
    switch(eb->fParam1) {
        case 1217: { /* JSON Stringify */
            if(eb->fParam2 != 1) die("unexpected JSON Stringify parameters");
            g_stringify_calls++;
            std::string json;
            json_of(*params[0], json);
            *ret = text_result(json.c_str());
            break;
        }
        case 1471: *ret = object_result(new_object(false)); break; /* New Object */
        case 1472: *ret = object_result(new_object(true)); break;  /* New Collection */
        default: {
            char msg[64];
            snprintf(msg, sizeof(msg), "unexpected PA_ExecuteCommandByID %d", (int)eb->fParam1);
            die(msg);
        }
    }
}

/* ---------- project methods (PA_GetMethodID / PA_ExecuteMethodByID) ---------- */

const PA_long32 kMethodID = 42;
std::function<PA_Variable(PA_Variable *params, int count)> g_method;
short g_method_error = 0;
int g_method_calls = 0;
int g_method_param_count = -1;
std::string g_method_param_json;

void call_method(EngineBlock *eb) {
    if(eb->fParam1 != kMethodID) die("unexpected method id");
    g_method_calls++;
    PA_Variable *slots = (PA_Variable *)live_handle(eb->fHandle)->data;
    if(slots[0].fType != eVK_Undefined) die("result slot not pre-initialised");
    PA_Variable *params = slots + 1;
    int count = (int)eb->fParam2;
    g_method_param_count = count;
    g_method_param_json.clear();
    if(count > 0) json_of(params[0], g_method_param_json);
    if(g_method_error) { eb->fError = g_method_error; return; } /* result slot left as pre-initialised */
    if(g_method) slots[0] = g_method(params, count);
}

void engine(short selector, EngineBlock *eb) {
    switch(selector) {
        case EX_CREATE_UNISTRING: {
            const PA_Unichar *src = (const PA_Unichar *)eb->fHandle;
            size_t len = 0;
            while(src[len]) len++;
            eb->fUniString1 = new_string(src, len);
            break;
        }
        case EX_DISPOSE_UNISTRING:
            free_string(&eb->fUniString1);
            break;
        case EX_VARIABLE_TO_STRING: {
            PA_Variable *v = (PA_Variable *)eb->fHandle;
            if(v->fType == eVK_Unistring) eb->fUniString1 = v->uValue.fString; /* borrowed */
            break;
        }
        case EX_CLEAR_VARIABLE:
            clear_var((PA_Variable *)eb->fPtr1);
            break;
        case EX_GET_INFORMATION:
            eb->fParam2 = 0x1800; /* 4D v18+: PA_CreateObject uses New Object */
            break;
        case EX_SET_OBJ_VALUE: {
            Obj *o = live(eb->fPtr1);
            PA_Variable value = copy_var(*(PA_Variable *)eb->fPtr2);
            if(eb->fShort == eVK_Collection) {
                if(!o->coll || eb->fLongint < 0) die("bad collection set");
                size_t i = (size_t)eb->fLongint;
                if(i >= o->items.size()) o->items.resize(i + 1, undefined_var());
                clear_var(&o->items[i]);
                o->items[i] = value;
            } else {
                u16 k = key_of(eb->fUniString1);
                PA_Variable *p = find_prop(o, k);
                if(p) { clear_var(p); *p = value; }
                else o->props.push_back(std::make_pair(k, value));
            }
            break;
        }
        case EX_GET_OBJ_VALUE: {
            Obj *o = live(eb->fPtr1);
            PA_Variable *out = (PA_Variable *)eb->fPtr2;
            *out = undefined_var();
            if(eb->fShort == eVK_Collection) {
                if(!o->coll) die("collection get on an object");
                if(eb->fLongint == -1) {
                    out->fType = eVK_Longint;
                    out->uValue.fLongint = (PA_long32)o->items.size();
                } else if(eb->fLongint >= 0 && (size_t)eb->fLongint < o->items.size()) {
                    *out = copy_var(o->items[(size_t)eb->fLongint]);
                }
                break;
            }
            PA_Variable *p = find_prop(o, key_of(eb->fUniString1));
            if(p) *out = copy_var(*p);
            break;
        }
        case EX_EXECUTE_COMMAND_BY_ID:
            execute_command(eb);
            break;
        case EX_GET_METHOD_ID:
            eb->fParam1 = narrow(u16(eb->fUName)) == "fourd_method" ? kMethodID : 0;
            break;
        case EX_CALL_BY_PROCID:
            call_method(eb);
            break;
        case EX_HANDLE_MANAGER: {
            switch(eb->fParam1) {
                case 1: eb->fHandle = (PA_Handle)new_handle((size_t)eb->fParam2); break;
                case 2: free_handle((Handle *)eb->fHandle); break;
                case 5: eb->fParam3 = (sLONG_PTR)live_handle(eb->fHandle)->data; break;
                case 6: break;
                default: die("unexpected handle manager op");
            }
            break;
        }
        default: {
            char msg[64];
            snprintf(msg, sizeof(msg), "unmodelled engine selector %d", selector);
            die(msg);
        }
    }
}

} // namespace

/* ---------- Python C API stub (no interpreter) ---------- */

namespace {

enum PyKind { kStr, kLong, kFloat, kDict, kList, kTuple, kBytes, kModule };

struct SObj {
    PyObject ob;
    PyKind kind;
    std::string s;
    long l = 0;
    double d = 0;
    std::vector<PyObject *> items;              /* list / tuple */
    std::vector<std::pair<PyObject *, PyObject *>> dict;
};

void sobj_dealloc(PyObject *o);

PyTypeObject make_type(const char *name, unsigned long flags) {
    PyTypeObject t;
    memset(&t, 0, sizeof(t));
    t.ob_base.ob_base.ob_refcnt = 1 << 20;
    t.tp_name = name;
    t.tp_flags = flags;
    t.tp_dealloc = sobj_dealloc;
    return t;
}

PyTypeObject g_str_type = make_type("str", Py_TPFLAGS_UNICODE_SUBCLASS);
PyTypeObject g_long_type = make_type("int", Py_TPFLAGS_LONG_SUBCLASS);
PyTypeObject g_dict_type = make_type("dict", Py_TPFLAGS_DICT_SUBCLASS);
PyTypeObject g_list_type = make_type("list", Py_TPFLAGS_LIST_SUBCLASS);
PyTypeObject g_tuple_type = make_type("tuple", Py_TPFLAGS_TUPLE_SUBCLASS);
PyTypeObject g_bytes_type = make_type("bytes", Py_TPFLAGS_BYTES_SUBCLASS);
PyTypeObject g_module_type = make_type("module", 0);
PyTypeObject g_none_type = make_type("NoneType", 0);

SObj *S(PyObject *o) { return (SObj *)o; }

void sobj_dealloc(PyObject *o) {
    SObj *s = S(o);
    for(PyObject *i : s->items) Py_DECREF(i);
    for(auto &kv : s->dict) { Py_DECREF(kv.first); Py_DECREF(kv.second); }
    delete s;
}

PyObject *new_sobj(PyTypeObject *t, PyKind k) {
    SObj *s = new SObj();
    s->ob.ob_refcnt = 1;
    s->ob.ob_type = t;
    s->kind = k;
    return &s->ob;
}

bool is(PyObject *o, PyKind k) { return o && Py_TYPE(o) != &PyBool_Type && Py_TYPE(o) != &g_none_type && S(o)->kind == k; }

std::function<PyObject *()> g_script;   /* what PyRun_String "evaluates" to */
std::string g_raise;                    /* set to raise an exception with this text */
PyObject *g_err = nullptr;

PyObject *str_obj(const char *s) {
    PyObject *o = new_sobj(&g_str_type, kStr);
    S(o)->s = s;
    return o;
}

} // namespace

extern "C" {

PyTypeObject PyBool_Type = make_type("bool", Py_TPFLAGS_LONG_SUBCLASS);
PyTypeObject PyFloat_Type = make_type("float", 0);
PyObject _Py_NoneStruct = { 1 << 20, &g_none_type };
struct _longobject _Py_TrueStruct = { { { 1 << 20, &PyBool_Type }, 1 }, { 1 } };
struct _longobject _Py_FalseStruct = { { { 1 << 20, &PyBool_Type }, 0 }, { 0 } };
int Py_NoSiteFlag, Py_IgnoreEnvironmentFlag, Py_NoUserSiteDirectory;

int PyType_IsSubtype(PyTypeObject *a, PyTypeObject *b) { return a == b; }

PyObject *PyUnicode_FromStringAndSize(const char *u, Py_ssize_t size) {
    PyObject *o = new_sobj(&g_str_type, kStr);
    S(o)->s.assign(u, (size_t)size);
    return o;
}
char *PyUnicode_AsUTF8AndSize(PyObject *o, Py_ssize_t *size) {
    if(!is(o, kStr)) return NULL;
    if(size) *size = (Py_ssize_t)S(o)->s.size();
    return (char *)S(o)->s.c_str();
}
PyObject *PyLong_FromLong(long v) { PyObject *o = new_sobj(&g_long_type, kLong); S(o)->l = v; return o; }
long PyLong_AsLong(PyObject *o) {
    if(o == Py_True) return 1;
    if(o == Py_False) return 0;
    return S(o)->l;
}
PyObject *PyFloat_FromDouble(double v) { PyObject *o = new_sobj(&PyFloat_Type, kFloat); S(o)->d = v; return o; }
double PyFloat_AsDouble(PyObject *o) { return S(o)->d; }
PyObject *PyBytes_FromStringAndSize(const char *b, Py_ssize_t size) {
    PyObject *o = new_sobj(&g_bytes_type, kBytes);
    if(b) S(o)->s.assign(b, (size_t)size);
    return o;
}
PyObject *PyDict_New(void) { return new_sobj(&g_dict_type, kDict); }
int PyDict_SetItem(PyObject *d, PyObject *k, PyObject *v) {
    Py_INCREF(k);
    Py_INCREF(v);
    S(d)->dict.push_back(std::make_pair(k, v));
    return 0;
}
int PyDict_Next(PyObject *d, Py_ssize_t *pos, PyObject **k, PyObject **v) {
    if(!is(d, kDict) || *pos < 0 || (size_t)*pos >= S(d)->dict.size()) return 0;
    *k = S(d)->dict[(size_t)*pos].first;  /* borrowed */
    *v = S(d)->dict[(size_t)*pos].second; /* borrowed */
    (*pos)++;
    return 1;
}
PyObject *PyList_New(Py_ssize_t size) {
    if(size) die("PyList_New with items");
    return new_sobj(&g_list_type, kList);
}
int PyList_Append(PyObject *l, PyObject *v) { Py_INCREF(v); S(l)->items.push_back(v); return 0; }
Py_ssize_t PyList_Size(PyObject *l) { return is(l, kList) ? (Py_ssize_t)S(l)->items.size() : -1; }
PyObject *PyList_GetItem(PyObject *l, Py_ssize_t i) { return S(l)->items[(size_t)i]; } /* borrowed */

int PyArg_ParseTuple(PyObject *args, const char *fmt, ...) {
    if(strcmp(fmt, "s|O") || !is(args, kTuple) || S(args)->items.empty() || !is(S(args)->items[0], kStr))
        die("unexpected PyArg_ParseTuple");
    va_list ap;
    va_start(ap, fmt);
    char **s = va_arg(ap, char **);
    PyObject **o = va_arg(ap, PyObject **);
    va_end(ap);
    *s = (char *)S(S(args)->items[0])->s.c_str();
    if(S(args)->items.size() > 1) *o = S(args)->items[1];
    return 1;
}

PyObject *PyErr_Occurred(void) { return g_err; }
void PyErr_Clear(void) { if(g_err) { Py_DECREF(g_err); g_err = nullptr; } }
void PyErr_Fetch(PyObject **t, PyObject **v, PyObject **tb) { *t = nullptr; *v = g_err; *tb = nullptr; g_err = nullptr; }
void PyErr_NormalizeException(PyObject **, PyObject **, PyObject **) {}
PyObject *PyObject_Str(PyObject *o) { Py_INCREF(o); return o; }

const char *Py_GetVersion(void) { return "3.6.7 (stub)"; }
const char *Py_GetBuildInfo(void) { return "stub"; }
const char *Py_GetCopyright(void) { return "stub"; }
const char *Py_GetCompiler(void) { return "stub"; }
int Py_IsInitialized(void) { return 1; }

PyObject *g_main_dict = nullptr;
PyObject *PyImport_AddModule(const char *) { static PyObject *m = new_sobj(&g_module_type, kModule); return m; }
PyObject *PyModule_GetDict(PyObject *) { if(!g_main_dict) g_main_dict = PyDict_New(); return g_main_dict; }
PyObject *PyModule_Create2(struct PyModuleDef *, int) { return new_sobj(&g_module_type, kModule); }

PyObject *PyRun_StringFlags(const char *, int, PyObject *, PyObject *, PyCompilerFlags *) {
    if(!g_raise.empty()) { g_err = str_obj(g_raise.c_str()); return NULL; }
    return g_script ? g_script() : NULL;
}

void Py_SetPath(const wchar_t *) {}
void Py_SetPythonHome(wchar_t *) {}
int PyImport_AppendInittab(const char *, PyObject *(*)(void)) { return 0; }
void Py_InitializeEx(int) {}
int Py_FinalizeEx(void) { return 0; }

} // extern "C"

namespace {

/* ---------- test helpers ---------- */

PA_Unistring U(const char *ascii) {
    u16 s = widen(ascii);
    return new_string(s.c_str(), s.size());
}

void put(Obj *o, const char *key, PA_Variable v) {
    PA_Unistring k = U(key);
    PA_SetObjectProperty((PA_ObjectRef)o, &k, v);
    PA_DisposeUnistring(&k);
}

void put_text(Obj *o, const char *key, const char *value) {
    PA_Variable v = text_result(value);
    put(o, key, v);
    PA_ClearVariable(&v);
}

void put_real(Obj *o, const char *key, double d) {
    PA_Variable v = undefined_var();
    PA_SetRealVariable(&v, d);
    put(o, key, v);
}

bool check(bool ok, const char *what) {
    if(!ok) fprintf(stderr, "  check failed: %s\n", what);
    return ok;
}

bool no_leaks() {
    bool ok = true;
    for(void *p : g_strings) {
        fprintf(stderr, "  LEAK: engine string \"%s\"\n", narrow(u16((PA_Unichar *)p)).c_str());
        ok = false;
    }
    if(!g_objects.empty()) { fprintf(stderr, "  LEAK: %zu engine object(s)/collection(s)\n", g_objects.size()); ok = false; }
    if(!g_handles.empty()) { fprintf(stderr, "  LEAK: %zu engine handle(s)\n", g_handles.size()); ok = false; }
    if(!g_violations.empty()) ok = false;
    return ok;
}

/* Python value builders (new references) */
PyObject *py_str(const char *s) { return PyUnicode_FromStringAndSize(s, (Py_ssize_t)strlen(s)); }
PyObject *py_list(std::initializer_list<PyObject *> items) {
    PyObject *l = PyList_New(0);
    for(PyObject *i : items) { PyList_Append(l, i); Py_DECREF(i); }
    return l;
}
PyObject *py_dict(std::initializer_list<std::pair<const char *, PyObject *>> items) {
    PyObject *d = PyDict_New();
    for(auto &kv : items) { PyObject *k = py_str(kv.first); PyDict_SetItem(d, k, kv.second); Py_DECREF(k); Py_DECREF(kv.second); }
    return d;
}
PyObject *py_true() { Py_INCREF(Py_True); return Py_True; }
PyObject *py_none() { Py_INCREF(Py_None); return Py_None; }

/* Calls the real `python` command (selector 1) through PluginMain. */
Obj *call_python(const char *script, Obj *variables, PA_long32 mode) {
    PA_Unistring s = U(script);
    PA_ObjectRef p2 = (PA_ObjectRef)variables;
    PA_long32 p3 = mode;
    void *args[3] = { &s, &p2, &p3 };
    PA_ObjectRef result = NULL;
    PluginBlock pb;
    pb.fParameters = args;
    pb.fResult = &result;
    pb.fData = NULL;
    PluginMain(1, &pb);
    PA_DisposeUnistring(&s);
    return (Obj *)result;
}

/* Calls the real fourd.call() implementation with ("fourd_method"[, arg]). */
PyObject *call_fourd(const char *method, PyObject *arg) {
    PyObject *t = new_sobj(&g_tuple_type, kTuple);
    S(t)->items.push_back(py_str(method));
    if(arg) S(t)->items.push_back(arg);
    PyObject *r = fourd_call(NULL, t);
    Py_DECREF(t);
    return r;
}

Obj *prop_obj(Obj *o, const char *key, PA_VariableKind kind) {
    PA_Variable *p = find_prop(o, widen(key));
    if(!p || p->fType != kind) return nullptr;
    return (Obj *)(kind == eVK_Collection ? (void *)p->uValue.fCollection : (void *)p->uValue.fObject);
}

std::string prop_json(Obj *o, const char *key) {
    PA_Variable *p = find_prop(o, widen(key));
    std::string s;
    if(p) json_of(*p, s);
    return s;
}

std::string py_text(PyObject *o) { return is(o, kStr) ? S(o)->s : std::string("<not str>"); }

/* ---------- tests ---------- */

/* LIVE: `python` with $2 variables: the JSON Stringify text result (1217)
   in fourd_object_to_json was never cleared. */
bool defect_cmd_python_variables_stringify_leak() {
    Obj *vars = new_object(false);
    put_text(vars, "name", "x");
    put_real(vars, "n", 3);
    std::string seen;
    g_script = [&]() -> PyObject * { return PyDict_New(); };
    Obj *status = call_python("pass", vars, 0);
    bool ok = check(status != nullptr, "status returned");
    ok &= check(g_stringify_calls == 1, "variables stringified once");
    ok &= check(g_objects.count(vars) && vars->rc == 1, "caller's $2 object untouched (borrowed)");
    release_object(vars);
    if(status) release_object(status);
    return no_leaks() && ok;
}

/* LIVE: a returned list element that is a string: PA_SetCollectionElement
   deep-copies, the source variable's Unistring was never cleared. */
bool defect_cmd_python_list_string_element_leak() {
    g_script = []() -> PyObject * { return py_dict({ { "l", py_list({ py_str("item") }) } }); };
    Obj *status = call_python("x", NULL, 1);
    Obj *rv = status ? prop_obj(status, "returnValue", eVK_Object) : nullptr;
    bool ok = check(rv && prop_json(rv, "l") == "[\"item\"]", "returnValue.l == [\"item\"]");
    if(status) release_object(status);
    return no_leaks() && ok;
}

/* LIVE: a returned list element that is a dict: the created object was
   retained by the collection but the plugin's own reference never released. */
bool defect_cmd_python_list_dict_element_leak() {
    g_script = []() -> PyObject * { return py_dict({ { "l", py_list({ py_dict({ { "k", py_str("v") } }) }) } }); };
    Obj *status = call_python("x", NULL, 1);
    Obj *rv = status ? prop_obj(status, "returnValue", eVK_Object) : nullptr;
    bool ok = check(rv && prop_json(rv, "l") == "[{\"k\":\"v\"}]", "returnValue.l == [{k:v}]");
    if(status) release_object(status);
    return no_leaks() && ok;
}

/* LIVE: a returned list element that is a list (nested collection). */
bool defect_cmd_python_list_list_element_leak() {
    g_script = []() -> PyObject * { return py_dict({ { "l", py_list({ py_list({ PyLong_FromLong(1) }) }) } }); };
    Obj *status = call_python("x", NULL, 1);
    Obj *rv = status ? prop_obj(status, "returnValue", eVK_Object) : nullptr;
    bool ok = check(rv && prop_json(rv, "l") == "[[1]]", "returnValue.l == [[1]]");
    if(status) release_object(status);
    return no_leaks() && ok;
}

/* LIVE: fourd.call(): the text result of PA_ExecuteMethodByID was never cleared. */
bool defect_fourd_call_text_result_leak() {
    g_method = [](PA_Variable *, int) { return text_result("hello"); };
    PyObject *r = call_fourd("fourd_method", NULL);
    bool ok = check(py_text(r) == "hello", "fourd.call returned \"hello\"");
    ok &= check(g_method_calls == 1 && g_method_param_count == 0, "method called once without a parameter");
    Py_XDECREF(r);
    return no_leaks() && ok;
}

/* LIVE: fourd.call(): an object result was never released, nor the JSON
   Stringify text of fourd_object_to_json. */
bool defect_fourd_call_object_result_leak() {
    g_method = [](PA_Variable *, int) { Obj *o = new_object(false); put_text(o, "a", "b"); return object_result(o); };
    PyObject *r = call_fourd("fourd_method", py_dict({ { "in", py_str("v") } }));
    bool ok = check(is(r, kDict) && S(r)->dict.size() == 1 && py_text(S(r)->dict[0].second) == "b", "fourd.call returned {a: b}");
    ok &= check(g_method_param_count == 1 && g_method_param_json == "{\"in\":\"v\"}", "method got $1 {in: v}");
    Py_XDECREF(r);
    return no_leaks() && ok;
}

/* LIVE: fourd.call(): a collection result was never released, nor the JSON
   Stringify text of fourd_collection_to_json. */
bool defect_fourd_call_collection_result_leak() {
    g_method = [](PA_Variable *, int) {
        Obj *c = new_object(true);
        PA_Variable v = text_result("e");
        PA_SetCollectionElement((PA_CollectionRef)c, 0, v);
        PA_ClearVariable(&v);
        return object_result(c);
    };
    PyObject *r = call_fourd("fourd_method", NULL);
    bool ok = check(is(r, kList) && S(r)->items.size() == 1 && py_text(S(r)->items[0]) == "e", "fourd.call returned [\"e\"]");
    Py_XDECREF(r);
    return no_leaks() && ok;
}

/* LIVE end to end: a script calling fourd.call() and returning its result. */
bool defect_cmd_python_fourd_call_end_to_end() {
    g_method = [](PA_Variable *, int) { return text_result("from4d"); };
    g_script = []() -> PyObject * { return py_dict({ { "r", call_fourd("fourd_method", py_dict({})) } }); };
    Obj *status = call_python("x", NULL, 1);
    Obj *rv = status ? prop_obj(status, "returnValue", eVK_Object) : nullptr;
    bool ok = check(rv && prop_json(rv, "r") == "\"from4d\"", "returnValue.r == \"from4d\"");
    if(status) release_object(status);
    return no_leaks() && ok;
}

/* ---------- controls (pass on both trees) ---------- */

bool control_cmd_python_scalar_dict() {
    g_script = []() -> PyObject * {
        return py_dict({ { "s", py_str("x") }, { "n", PyLong_FromLong(7) }, { "f", PyFloat_FromDouble(2.5) },
                         { "b", py_true() }, { "z", py_none() }, { "d", py_dict({ { "k", py_str("v") } }) } });
    };
    Obj *status = call_python("x", NULL, 1);
    Obj *rv = status ? prop_obj(status, "returnValue", eVK_Object) : nullptr;
    std::string json;
    if(rv) json_of_obj(rv, json);
    bool ok = check(json == "{\"s\":\"x\",\"n\":7,\"f\":2.5,\"b\":true,\"z\":null,\"d\":{\"k\":\"v\"}}", "returnValue round-trip");
    ok &= check(status && prop_json(status, "version") == "\"3.6.7 (stub)\"", "version reported");
    if(status) release_object(status);
    return no_leaks() && ok;
}

bool control_cmd_python_list_scalars() {
    g_script = []() -> PyObject * {
        return py_dict({ { "l", py_list({ PyLong_FromLong(1), PyFloat_FromDouble(2.5), py_true(), py_none() }) } });
    };
    Obj *status = call_python("x", NULL, 1);
    Obj *rv = status ? prop_obj(status, "returnValue", eVK_Object) : nullptr;
    bool ok = check(rv && prop_json(rv, "l") == "[1,2.5,true,null]", "returnValue.l == [1,2.5,true,null]");
    if(status) release_object(status);
    return no_leaks() && ok;
}

bool control_cmd_python_error_message() {
    g_raise = "boom";
    Obj *status = call_python("x", NULL, 0);
    bool ok = check(status && prop_json(status, "errorMessage") == "\"boom\"", "errorMessage reported");
    ok &= check(status && !find_prop(status, widen("returnValue")), "no returnValue");
    if(status) release_object(status);
    return no_leaks() && ok;
}

bool control_fourd_call_unknown_method() {
    PyObject *r = call_fourd("no_such_method", NULL);
    bool ok = check(r == Py_None, "None for an unknown method");
    ok &= check(g_method_calls == 0, "nothing executed");
    Py_XDECREF(r);
    return no_leaks() && ok;
}

bool control_fourd_call_longint_result() {
    g_method = [](PA_Variable *, int) { PA_Variable v = undefined_var(); PA_SetLongintVariable(&v, 5); return v; };
    PyObject *r = call_fourd("fourd_method", NULL);
    bool ok = check(is(r, kLong) && S(r)->l == 5, "fourd.call returned 5");
    Py_XDECREF(r);
    return no_leaks() && ok;
}

/* The method fails: the result slot stays as the SDK pre-initialised it
   (Undefined); nothing to convert, nothing leaked. */
bool control_fourd_call_method_error() {
    g_method_error = -1;
    PyObject *r = call_fourd("fourd_method", NULL);
    bool ok = check(r == Py_None, "None when the method fails");
    Py_XDECREF(r);
    return no_leaks() && ok;
}

struct Test { const char *name; bool (*fn)(); };
const Test kTests[] = {
    { "defect_cmd_python_variables_stringify_leak", defect_cmd_python_variables_stringify_leak },
    { "defect_cmd_python_list_string_element_leak", defect_cmd_python_list_string_element_leak },
    { "defect_cmd_python_list_dict_element_leak", defect_cmd_python_list_dict_element_leak },
    { "defect_cmd_python_list_list_element_leak", defect_cmd_python_list_list_element_leak },
    { "defect_fourd_call_text_result_leak", defect_fourd_call_text_result_leak },
    { "defect_fourd_call_object_result_leak", defect_fourd_call_object_result_leak },
    { "defect_fourd_call_collection_result_leak", defect_fourd_call_collection_result_leak },
    { "defect_cmd_python_fourd_call_end_to_end", defect_cmd_python_fourd_call_end_to_end },
    { "control_cmd_python_scalar_dict", control_cmd_python_scalar_dict },
    { "control_cmd_python_list_scalars", control_cmd_python_list_scalars },
    { "control_cmd_python_error_message", control_cmd_python_error_message },
    { "control_fourd_call_unknown_method", control_fourd_call_unknown_method },
    { "control_fourd_call_longint_result", control_fourd_call_longint_result },
    { "control_fourd_call_method_error", control_fourd_call_method_error },
};

} // namespace

int main(int argc, char **argv) {
    gCall4D = engine;
    if(argc == 2 && !strcmp(argv[1], "--list")) {
        for(const Test &t : kTests) printf("%s\n", t.name);
        return 0;
    }
    if(argc != 2) { fprintf(stderr, "usage: %s --list | <test>\n", argv[0]); return 2; }
    for(const Test &t : kTests) {
        if(!strcmp(argv[1], t.name)) {
            bool ok = t.fn();
            printf("%s: %s\n", t.name, ok ? "PASS" : "FAIL");
            return ok ? 0 : 1;
        }
    }
    fprintf(stderr, "unknown test %s\n", argv[1]);
    return 2;
}
