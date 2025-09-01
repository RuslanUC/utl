#include "stb_ds.h"
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

static inline int qsort_strcmp(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

// TODO: check that ->type is PYUTL_CACHED_OBJECT or PYUTL_CACHED_TYPE in callers of this function
pyutl_MessageDef* Py_DefPool_get_or_create_cached_def(const utl_MessageDef* message_def) {
    pyutl_ModuleState* state = pyutl_ModuleState_get();
    const ptrdiff_t cached_def_idx = hmgeti(state->defs_cache, (intptr_t)message_def);

    if(cached_def_idx >= 0)
        return state->defs_cache[cached_def_idx].value;

    PyObject* type = Py_TLObject_createType(message_def);

    const size_t struct_size = sizeof(pyutl_MessageDef);
    const size_t names_arr_size = sizeof(char*) * message_def->fields_num;
    const size_t nums_arr_size = sizeof(uint8_t) * message_def->fields_num;
    size_t names_buf_size = 0;

    for(size_t i = 0; i < message_def->fields_num; ++i)
        names_buf_size += message_def->fields[i].name.size + 1;

    uint8_t* base_def_ptr = malloc(struct_size + names_buf_size + names_arr_size + nums_arr_size);
    pyutl_MessageDef* cached_def = (pyutl_MessageDef*)base_def_ptr;
    cached_def->type = PYUTL_CACHED_OBJECT;
    cached_def->python_cls = (PyTypeObject*)type;
    cached_def->field_names_buf = (char*)(base_def_ptr + struct_size);
    cached_def->field_names = (char**)(base_def_ptr + struct_size + names_buf_size);
    cached_def->field_nums = (uint8_t*)(base_def_ptr + struct_size + names_buf_size + names_arr_size);

    size_t names_buf_offset = 0;
    for(size_t i = 0; i < message_def->fields_num; ++i) {
        const utl_StringView field_name = message_def->fields[i].name;
        char* dst = cached_def->field_names_buf + names_buf_offset;
        dst[field_name.size] = '\0';
        names_buf_offset += field_name.size + 1;

        memcpy(dst, field_name.data, field_name.size);
        cached_def->field_names[i] = dst;
    }

    // @formatter:off
    _UTL_LOG("->field_names before sorting:");
    _UTL_LOG_INCIND();
        for(size_t i = 0; i < message_def->fields_num; ++i)
            _UTL_LOG("[%zu]: \"%s\"", i, cached_def->field_names[i]);
    _UTL_LOG_DECIND();
    // @formatter:on

    qsort(cached_def->field_names, message_def->fields_num, sizeof(char*), qsort_strcmp);

    // @formatter:off
    _UTL_LOG("->field_names after sorting:");
    _UTL_LOG_INCIND();
        for(size_t i = 0; i < message_def->fields_num; ++i)
            _UTL_LOG("[%zu]: \"%s\"", i, cached_def->field_names[i]);
    _UTL_LOG_DECIND();
    // @formatter:on

    for(size_t i = 0; i < message_def->fields_num; ++i) {
        const utl_StringView field_name = message_def->fields[i].name;
        const int index = binary_search_str(cached_def->field_names, message_def->fields_num, field_name.data, field_name.size);
        if(index < 0) {
            _UTL_LOG("Failed field look up: \"%.*s\"", (int)field_name.size, field_name.data);
            free(cached_def);
            PyErr_SetString(PyExc_RuntimeError, "Failed to look up just inserted field name inside pyutl_MessageDef.");
            return NULL;
        }
        cached_def->field_nums[index] = i;
    }

    hmput(state->defs_cache, (intptr_t)message_def, cached_def);

    return cached_def;
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

    // @formatter:off
    _UTL_LOG("Parsed tl constructor definition: \"%.*s\"", (int)str_len, str);
    _UTL_LOG_INCIND();
        _UTL_LOG("message_def->id = 0x%08x", message_def->id);
        _UTL_LOG("message_def->name = \"%.*s\"", (int)message_def->name.size, message_def->name.data);
        _UTL_LOG("message_def->namespace_ = \"%.*s\"", (int)message_def->namespace_.size, message_def->namespace_.data);
        _UTL_LOG("message_def->type = \"%.*s\"", (int)message_def->type->name.size, message_def->type->name.data);
        _UTL_LOG("message_def->section = %s", message_def->section == TYPES ? "types" : "functions");
        _UTL_LOG("message_def->layer = %d", message_def->layer);
        _UTL_LOG("message_def->fields_num = %d", message_def->fields_num);
        _UTL_LOG("message_def->fields = [");
        _UTL_LOG_INCIND();
            for(size_t i = 0; i < message_def->fields_num; ++i) {
                _UTL_LOG("message_def->fields[%zu].type = %d", i, message_def->fields[i].type);
                _UTL_LOG("message_def->fields[%zu].num = %zu", i, message_def->fields[i].num);
                _UTL_LOG("message_def->fields[%zu].offset = %zu", i, message_def->fields[i].offset);
                _UTL_LOG("message_def->fields[%zu].name = \"%.*s\"", i, (int)message_def->fields[i].name.size, message_def->fields[i].name.data);
                _UTL_LOG("message_def->fields[%zu].flag_info = %d", i, message_def->fields[i].flag_info);
                if(i < (message_def->fields_num - 1))
                    _UTL_LOG("");
            }
        _UTL_LOG_DECIND();
        _UTL_LOG("]");
        _UTL_LOG("message_def->flags_num = %d", message_def->flags_num);
        _UTL_LOG("message_def->strings_num = %d", message_def->strings_num);
        _UTL_LOG("message_def->size = %zu", message_def->size);
    _UTL_LOG_DECIND();
    // @formatter:on

    pyutl_MessageDef* cached_def = Py_DefPool_get_or_create_cached_def(message_def);
    if(!cached_def)
        return NULL;

    PyObject* type = (PyObject*)cached_def->python_cls;
    Py_INCREF(type);
    return type;
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
        Py_RETURN_NONE;

    pyutl_MessageDef* cached_def = Py_DefPool_get_or_create_cached_def(message_def);
    if(!cached_def)
        return NULL;

    PyObject* type = (PyObject*)cached_def->python_cls;
    Py_INCREF(type);
    return type;
}

PyObject* Py_DefPool_create_type(const Py_DefPool* self, PyObject* args) {
    char* str;
    size_t str_len;

    if (!PyArg_ParseTuple(args, "s#", &str, &str_len)) {
        return NULL;
    }

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

    PyObject* type = Py_TLType_createType(type_def);
    Py_INCREF(type);
    return type;
}

PyObject* Py_DefPool_get_type(const Py_DefPool* self, PyObject* args) {
    char* str;
    size_t str_len;

    if (!PyArg_ParseTuple(args, "s#", &str, &str_len)) {
        return NULL;
    }

    const utl_StringView name = {
        .size = str_len,
        .data = str,
    };

    utl_TypeDef* type_def = utl_DefPool_getType(self->pool, name);
    if(!type_def) {
        Py_RETURN_NONE;
    }

    PyObject* type = Py_TLType_createType(type_def);
    Py_INCREF(type);
    return type;
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
