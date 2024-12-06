#include "pyutl.h"
#include "py_def_pool.h"
#include "tlobject.h"
#include "tltype.h"

static void Py_DefPool_dealloc(PyObject* self) {
    utl_DefPool_free(((Py_DefPool*)self)->pool);
    self->ob_type->tp_free(self);
}

static PyObject* Py_DefPool_new(PyTypeObject* cls, PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyObject* self = cls->tp_alloc(cls, 0);
    ((Py_DefPool*)self)->pool = utl_DefPool_new();

    return self;
}

PyObject* Py_DefPool_parse(const Py_DefPool* self, PyObject* args) {
    char* str;
    size_t str_len;
    int32_t layer;
    uint8_t section;

    if (!PyArg_ParseTuple(args, "s#ib", &str, &str_len, &layer, &section)) {
        return NULL;
    }

    if(section != TYPES && section != FUNCTIONS) {
        PyErr_SetString(PyExc_ValueError, "\"section\" argument can be either 0 or 1.");
        return NULL;
    }

    utl_Status status;
    utl_MessageDef* message_def = utl_parse_line(self->pool, str, str_len, &status);
    if(!status.ok) {
        PyErr_SetString(PyExc_ValueError, status.message);
        return NULL;
    }
    if(!message_def) {
        PyErr_SetString(PyExc_ValueError, "Status.ok is true, but utl_parse_line returned NULL.");
        return NULL;
    }

    message_def->layer = layer;
    message_def->section = (utl_MessageSection)section;

    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    pyutl_MessageDef* cached_def = utl_Map_search_uint64(state->messages_cache, (uint64_t)message_def);
    if(!cached_def) {
        PyObject* type = Py_TLObject_createType(message_def);
        cached_def = arena_alloc(&state->c_def_pool->arena, sizeof(pyutl_MessageDef));
        cached_def->python_cls = (PyTypeObject*)type;
        cached_def->fields = utl_Map_new_on_arena(message_def->fields_num / 2, &state->c_def_pool->arena);
        for(size_t i = 0; i < message_def->fields_num; ++i) {
            utl_FieldDef* field = &message_def->fields[i];
            utl_Map_insert_str(cached_def->fields, field->name, field);
        }

        utl_Map_insert_uint64(state->messages_cache, (uint64_t)message_def, cached_def);
    }

    return (PyObject*)cached_def->python_cls;
}

PyObject* Py_DefPool_has_type(const Py_DefPool* self, PyObject* args) {
    char* str;
    size_t str_len;

    if (!PyArg_ParseTuple(args, "s#", &str, &str_len)) {
        return NULL;
    }

    const utl_StringView name = {
        .size = str_len,
        .data = str,
    };
    return PyBool_FromLong(utl_DefPool_hasType(self->pool, name));
}

PyObject* Py_DefPool_has_constructor(const Py_DefPool* self, PyObject* args) {
    uint32_t tl_id;

    if (!PyArg_ParseTuple(args, "I", &tl_id)) {
        return NULL;
    }

    return PyBool_FromLong(utl_DefPool_hasMessage(self->pool, tl_id));
}

PyObject* Py_DefPool_get_constructor(const Py_DefPool* self, PyObject* args) {
    uint32_t tl_id;

    if (!PyArg_ParseTuple(args, "I", &tl_id)) {
        return NULL;
    }

    utl_MessageDef* message_def = utl_DefPool_getMessage(self->pool, tl_id);
    if(!message_def)
        return Py_None;

    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    pyutl_MessageDef* cached = utl_Map_search_uint64(state->messages_cache, (uint64_t)message_def);
    if(!cached) {
        return Py_None;
    }

    return (PyObject*)cached->python_cls;
}

PyObject* Py_DefPool_create_type(const Py_DefPool* self, PyObject* args) {
    char* str;
    size_t str_len;

    if (!PyArg_ParseTuple(args, "s#", &str, &str_len)) {
        return NULL;
    }

    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    const utl_StringView name = {
        .size = str_len,
        .data = str,
    };

    utl_TypeDef* type_def = utl_DefPool_getType(self->pool, name);
    if(!type_def) {
        type_def = utl_TypeDef_new(&self->pool->arena);
        type_def->name = utl_StringView_clone(&self->pool->arena, name);
        utl_DefPool_addType(self->pool, type_def);
    }

    pyutl_MessageDef* cached_def = utl_Map_search_uint64(state->messages_cache, (uint64_t)type_def);
    if(!cached_def) {
        PyObject* py_type = Py_TLType_createType(type_def);
        cached_def = arena_alloc(&self->pool->arena, sizeof(pyutl_MessageDef));
        cached_def->python_cls = (PyTypeObject*)py_type;
        cached_def->fields = NULL;

        utl_Map_insert_uint64(state->messages_cache, (uint64_t)type_def, cached_def);
    }

    return (PyObject*)cached_def->python_cls;
}

PyObject* Py_DefPool_get_type(const Py_DefPool* self, PyObject* args) {
    char* str;
    size_t str_len;

    if (!PyArg_ParseTuple(args, "s#", &str, &str_len)) {
        return NULL;
    }

    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    const utl_StringView name = {
        .size = str_len,
        .data = str,
    };

    utl_TypeDef* type_def = utl_DefPool_getType(self->pool, name);
    if(!type_def) {
        return Py_None;
    }

    pyutl_MessageDef* cached_def = utl_Map_search_uint64(state->messages_cache, (uint64_t)type_def);
    if(!cached_def) {
        PyObject* py_type = Py_TLType_createType(type_def);
        cached_def = arena_alloc(&self->pool->arena, sizeof(pyutl_MessageDef));
        cached_def->python_cls = (PyTypeObject*)py_type;
        cached_def->fields = NULL;

        utl_Map_insert_uint64(state->messages_cache, (uint64_t)type_def, cached_def);
    }

    return (PyObject*)cached_def->python_cls;
}

static PyMethodDef Py_DefPool_methods[] = {
    {"parse", (PyCFunction)Py_DefPool_parse, METH_VARARGS, 0,},
    {"has_type", (PyCFunction)Py_DefPool_has_type, METH_VARARGS, 0,},
    {"get_type", (PyCFunction)Py_DefPool_get_type, METH_VARARGS, 0,},
    {"create_type", (PyCFunction)Py_DefPool_create_type, METH_VARARGS, 0,},
    {"has_constructor", (PyCFunction)Py_DefPool_has_constructor, METH_VARARGS, 0,},
    {"get_constructor", (PyCFunction)Py_DefPool_get_constructor, METH_VARARGS, 0,},
    {NULL}
};

static PyType_Slot Py_DefPool_slots[] = {
    {Py_tp_dealloc, Py_DefPool_dealloc},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {Py_tp_methods, Py_DefPool_methods},
    {Py_tp_new, Py_DefPool_new},
    {0, NULL}
};

PyType_Spec pyutl_DefPoolType_spec = {
    "_pyutl.DefPool",
    sizeof(Py_DefPool),
    0,
    Py_TPFLAGS_DEFAULT,
    Py_DefPool_slots,
};
