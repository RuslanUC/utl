#include "pyutl.h"
#include "tlobject.h"
#include "encoder.h"
#include "decoder.h"

static void Py_TLObject_dealloc(PyObject* self) {
    utl_Message_free(((Py_TLObject*)self)->message);
    self->ob_type->tp_free(self);
}

static PyObject* Py_TLObject_getitem(Py_TLObject* self, utl_FieldDef* field) {
    switch (field->type) {
        case FLAGS:
        case INT32: {
            return PyLong_FromLong(utl_Message_getInt32(self->message, field));
        }
        case INT64: {
            return PyLong_FromLong(utl_Message_getInt64(self->message, field));
        }
        case INT128: {
            char* bytes = utl_Message_getInt128(self->message, field);
            return _PyLong_FromByteArray((uint8_t*)bytes, 16, true, true);
        }
        case INT256: {
            char* bytes = utl_Message_getInt256(self->message, field);
            return _PyLong_FromByteArray((uint8_t*)bytes, 32, true, true);
        }
        case DOUBLE: {
            return PyFloat_FromDouble(utl_Message_getDouble(self->message, field));
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            return utl_Message_getBool(self->message, field) ? Py_True : Py_False;
        }
        case BYTES: {
            utl_StringView bytes = utl_Message_getBytes(self->message, field);
            return PyBytes_FromStringAndSize(bytes.data, bytes.size);
        }
        case STRING: {
            utl_StringView bytes = utl_Message_getString(self->message, field);
            return PyUnicode_FromStringAndSize(bytes.data, bytes.size);
        }
        case TLOBJECT: {
            PyErr_SetString(PyExc_NotImplementedError, "Objects nesting is not implemented yet.");
            return Py_None;
        }
        case VECTOR: {
            PyErr_SetString(PyExc_NotImplementedError, "Vectors are not implemented yet.");
            return Py_None;
        }
    }

    return Py_None;
}

static bool Py_TLObject_setitem(Py_TLObject* self, utl_FieldDef* field, PyObject* item) {
    switch (field->type) {
        case FLAGS:
        case INT32: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            utl_Message_setInt32(self->message, field, PyLong_AsLong(item));
            break;
        }
        case INT64: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            utl_Message_setInt64(self->message, field, PyLong_AsLong(item));
            break;
        }
        case INT128: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            char bytes[16];
            _PyLong_AsByteArray((PyLongObject*)item, bytes, 16, true, true);
            utl_Message_setInt128(self->message, field, bytes);
            break;
        }
        case INT256: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            char bytes[32];
            _PyLong_AsByteArray((PyLongObject*)item, bytes, 32, true, true);
            utl_Message_setInt256(self->message, field, bytes);
            break;
        }
        case DOUBLE: {
            if(!PyFloat_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"float\"");
                return false;
            }
            utl_Message_setDouble(self->message, field, PyFloat_AsDouble(item));
            break;
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            if(!PyBool_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"bool\"");
                return false;
            }
            utl_Message_setBool(self->message, field, item = Py_True);
            break;
        }
        case BYTES: {
            if(!PyBytes_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"bytes\"");
                return false;
            }
            char* buf;
            ssize_t len;
            if(PyBytes_AsStringAndSize(item, &buf, &len)) {
                return false;
            }
            utl_StringView bytes = utl_StringView_new(&self->message->arena, len);
            memcpy(bytes.data, buf, len);
            utl_Message_setBytes(self->message, field, bytes);
            break;
        }
        case STRING: {
            if(!PyUnicode_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"str\"");
                return false;
            }
            ssize_t len;
            const char *buf = PyUnicode_AsUTF8AndSize(item, &len);
            if(!buf) {
                return false;
            }
            utl_StringView bytes = utl_StringView_new(&self->message->arena, len);
            memcpy(bytes.data, buf, len);
            utl_Message_setBytes(self->message, field, bytes);
            break;
        }
        case TLOBJECT: {
            PyErr_SetString(PyExc_NotImplementedError, "Objects nesting is not implemented yet.");
            return false;
        }
        case VECTOR: {
            PyErr_SetString(PyExc_NotImplementedError, "Vectors are not implemented yet.");
            return false;
        }
    }
    return true;
}

static PyObject* Py_TLObject_new(PyTypeObject* cls, PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyObject* result = PyObject_GetAttrString((PyObject*)cls, "__message_def__");
    if(!result) {
        PyErr_SetString(PyExc_NotImplementedError, "Object of type \"TLObject\" cannot be instantiated.");
        return 0;
    }

    PyObject* self = cls->tp_alloc(cls, 0);
    utl_MessageDef* def = PyCapsule_GetPointer(result, NULL);
    ((Py_TLObject*)self)->message = utl_Message_new(def);

    return self;
}

static int Py_TLObject_init(Py_TLObject* self, PyObject* Py_UNUSED(args), PyObject* kwargs) {
    for(size_t i = 0; i < self->message->message_def->fields_num; ++i) {
        utl_FieldDef field = self->message->message_def->fields[i];
        if(field.type == FLAGS) {
            continue;
        }

        PyObject* field_name = PyUnicode_FromStringAndSize(field.name.data, field.name.size);
        PyObject* item = PyDict_GetItem(kwargs, field_name);
        Py_DECREF(field_name);

        if(!item) {
            if(!field.flag_info) {
                PyErr_SetString(PyExc_TypeError, "missing required keyword-only argument");
                return 0;
            }
            continue;
        }

        if(!Py_TLObject_setitem(self, &field, item)) {
            return 0;
        }
    }

    return 0;
}

PyObject* Py_TLObject_getattro(Py_TLObject* self, PyObject* attr) {
    pyutl_ModuleState* state = pyutl_ModuleState_get();
    pyutl_MessageDef* cached = utl_Map_search_uint64(state->messages_cache, (uint64_t)self->message->message_def);
    if(!cached) {
        return NULL;
    }

    ssize_t len;
    char *buf = (char*)PyUnicode_AsUTF8AndSize(attr, &len);
    if(!buf) {
        return NULL;
    }

    utl_StringView field_name = { .data = buf, .size = len };
    utl_FieldDef* field = utl_Map_search_str(cached->fields, field_name);
    if(!field) {
        return PyObject_GenericGetAttr((PyObject*)self, attr);
    }

    if(!utl_Message_hasField(self->message, field)) {
        return Py_None;
    }

    return Py_TLObject_getitem(self, field);
}

int Py_TLObject_setattro(Py_TLObject* self, PyObject* attr, PyObject* value) {
    pyutl_ModuleState* state = pyutl_ModuleState_get();
    pyutl_MessageDef* cached = utl_Map_search_uint64(state->messages_cache, (uint64_t)self->message->message_def);
    if(!cached) {
        return -1;
    }

    ssize_t len;
    char *buf = (char*)PyUnicode_AsUTF8AndSize(attr, &len);
    if(!buf) {
        return -1;
    }

    utl_StringView field_name = { .data = buf, .size = len };
    utl_FieldDef* field = utl_Map_search_str(cached->fields, field_name);
    if(!field) {
        return -1;
    }

    return Py_TLObject_setitem(self, field, value) ? 0 : -1;
}

/*PyObject* Py_TLObject_repr(Py_TLObject* self) {
    arena_t repr_arena = arena_new();
    repr_arena.flags |= ARENA_DONTALIGN;

    // TODO

    PyObject* result = PyUnicode_FromStringAndSize(buf, size);
    arena_delete(&repr_arena);
    return result;
}*/

static PyObject* Py_TLObject_read_bytes(PyTypeObject* cls, PyObject* args) {
    char* buf;
    size_t buf_len;
    if (!PyArg_ParseTuple(args, "s#", &buf, &buf_len)) {
        return NULL;
    }

    PyObject* result = PyObject_GetAttrString((PyObject*)cls, "__message_def__");
    if(!result) {
        // cls = ... // TODO: get by tl id if class is TLObject
    }

    pyutl_ModuleState* state = pyutl_ModuleState_get();
    Py_TLObject* obj = (Py_TLObject*)Py_TLObject_new(cls, NULL, NULL);

    utl_Status status;
    utl_decode(obj->message, state->default_c_def_pool, buf, buf_len, &status);
    if(!status.ok) {
        PyErr_SetString(PyExc_ValueError, status.message);
        return NULL;
    }

    return (PyObject*)obj;
}

static PyObject* Py_TLObject_write(Py_TLObject* self, PyObject* args) {
    arena_t encoder_arena = arena_new();
    encoder_arena.flags |= ARENA_DONTALIGN;
    size_t written_bytes = utl_encode(self->message, &encoder_arena);

    PyObject* result = PyBytes_FromStringAndSize(encoder_arena.data + sizeof(uint32_t*) * self->message->message_def->flags_num, written_bytes);
    arena_delete(&encoder_arena);

    return result;
}

static PyMethodDef Py_TLObject_methods[] = {
    {"read_bytes", (PyCFunction)Py_TLObject_read_bytes, METH_VARARGS | METH_CLASS, 0,},
    {"write", (PyCFunction)Py_TLObject_write, METH_NOARGS, 0,},
    {NULL}
};

static PyType_Slot Py_TLObject_slots[] = {
    {Py_tp_dealloc, Py_TLObject_dealloc},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {Py_tp_methods, Py_TLObject_methods},
    {Py_tp_new, Py_TLObject_new},
    {Py_tp_init, Py_TLObject_init},
    {Py_tp_getattro, Py_TLObject_getattro},
    {Py_tp_setattro, Py_TLObject_setattro},
    {0, NULL}
};

PyType_Spec pyutl_TLObjectType_spec = {
    "_pyutl.TLObject",
    sizeof(Py_TLObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    Py_TLObject_slots,
};

PyObject* Py_TLObject_createType(utl_MessageDef* message_def) {
    pyutl_ModuleState* state = pyutl_ModuleState_get();
    arena_t* arena = &state->default_c_def_pool->arena;
    size_t real_size = arena->size;

    char* name = arena_alloc(
        arena, 11 + (message_def->namespace_.size ? message_def->namespace_.size + 1 : 0) + message_def->name.size);
    memcpy(name, "_pyutl._tl.", 11);
    if(message_def->namespace_.size)
        memcpy(name+11, message_def->namespace_.data, message_def->namespace_.size);
    memcpy(name+11+message_def->namespace_.size, message_def->name.data, message_def->name.size);

    PyType_Slot slots[] = {
        {Py_tp_base, state->tlobject_type},
        {Py_tp_dealloc, Py_TLObject_dealloc},
        {Py_tp_hash, PyObject_HashNotImplemented},
        {Py_tp_methods, Py_TLObject_methods},
        {Py_tp_new, Py_TLObject_new},
        {Py_tp_init, Py_TLObject_init},
        {Py_tp_getattro, Py_TLObject_getattro},
        {Py_tp_setattro, Py_TLObject_setattro},
        {0, NULL}
    };

    PyType_Spec spec = {
        name,
        0,
        0,
        Py_TPFLAGS_DEFAULT,
        slots,
    };

    PyObject* new_type = PyType_FromSpec(&spec);
    if(!new_type) {
        arena->size = real_size;
        return 0;
    }

    arena->size = real_size;

    PyObject* msgdef_capsule = PyCapsule_New(message_def, NULL, NULL);
    if (!msgdef_capsule) {
        Py_DECREF(new_type);
        return 0;
    }

    if (PyObject_SetAttrString(new_type, "__message_def__", msgdef_capsule) < 0) {
        Py_DECREF(msgdef_capsule);
        Py_DECREF(new_type);
        return 0;
    }

    Py_DECREF(msgdef_capsule);

    return new_type;
}