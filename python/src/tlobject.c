#include "pyutl.h"
#include "tlobject.h"
#include "tlvector.h"
#include "encoder.h"
#include "decoder.h"

static PyObject* Py_TLObject_getitem(const Py_TLObject* self, const utl_FieldDef* field) {
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
            const utl_StringView bytes = utl_Message_getBytes(self->message, field);
            return PyBytes_FromStringAndSize(bytes.data, bytes.size);
        }
        case STRING: {
            const utl_StringView bytes = utl_Message_getString(self->message, field);
            return PyUnicode_FromStringAndSize(bytes.data, bytes.size);
        }
        case TLOBJECT: {
            const pyutl_ModuleState* state = pyutl_ModuleState_get();
            utl_Message* message = utl_Message_getMessage(self->message, field);

            PyObject* obj = utl_PtrMap_search(state->objects_cache, message);
            if(!obj) {
                pyutl_MessageDef* cached_def = utl_Map_search_uint64(state->messages_cache, (uint64_t)message->message_def);
                if(!cached_def) {
                    PyErr_SetString(PyExc_TypeError, "object type is not found");
                    return NULL;
                }

                obj = cached_def->python_cls->tp_alloc(cached_def->python_cls, 0);
                Py_TLObject_init_message((Py_TLObject*)obj, NULL, message);
            }

            Py_INCREF(obj);
            return obj;
        }
        case VECTOR: {
            const pyutl_ModuleState* state = pyutl_ModuleState_get();
            utl_Vector* vector = utl_Message_getVector(self->message, field);

            PyObject* obj = utl_PtrMap_search(state->objects_cache, vector);
            if(!obj) {
                obj = state->tlvector_type->tp_alloc(state->tlvector_type, 0);
                Py_TLVector_init_message((Py_TLVector*)obj, vector);
            }

            Py_INCREF(obj);
            return obj;
        }
    }

    return Py_None;
}

static bool Py_TLObject_setitem(const Py_TLObject* self, const utl_FieldDef* field, PyObject* item) {
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
#if PY_MINOR_VERSION < 13
            _PyLong_AsByteArray((PyLongObject*)item, bytes, 16, true, true);
#else
            _PyLong_AsByteArray((PyLongObject*)item, bytes, 16, true, true, true);
#endif
            utl_Message_setInt128(self->message, field, bytes);
            break;
        }
        case INT256: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            char bytes[32];
#if PY_MINOR_VERSION < 13
            _PyLong_AsByteArray((PyLongObject*)item, bytes, 32, true, true);
#else
            _PyLong_AsByteArray((PyLongObject*)item, bytes, 32, true, true, true);
#endif
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
            utl_Message_setBool(self->message, field, item == Py_True);
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
            const utl_StringView bytes = utl_StringView_new(&self->message->arena, len);
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
            const utl_StringView bytes = utl_StringView_new(&self->message->arena, len);
            memcpy(bytes.data, buf, len);
            utl_Message_setString(self->message, field, bytes);
            break;
        }
        case TLOBJECT: {
            const pyutl_ModuleState* state = pyutl_ModuleState_get();
            if(!PyObject_TypeCheck(item, state->tlobject_type)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\"");
                return false;
            }
            utl_Message* message = ((Py_TLObject*)item)->message;
            if(field->sub.type_def != NULL && field->sub.type_def != message->message_def->type) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\" (TODO: show exact type)");
                return false;
            }

            utl_Message* old_message = utl_Message_getMessage(self->message, field);
            PyObject* old_obj = utl_PtrMap_search(state->objects_cache, old_message);
            Py_XDECREF(old_obj);

            utl_Message_setMessage(self->message, field, message);
            Py_INCREF(item);
            break;
        }
        case VECTOR: {
            if(!PyList_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"list\"");
                return false;
            }

            utl_Vector* old_vector = utl_Message_getVector(self->message, field);
            const size_t len = PyList_Size(item);
            utl_Vector* vector = utl_Vector_new(field->sub.vector_def, len);
            utl_Message_setVector(self->message, field, vector);

            for(size_t i = 0; i < len; i++) {
                void* element = Py_TLVector_item_to_utl(vector, PyList_GetItem(item, i));
                if(!element) {
                    utl_Message_setVector(self->message, field, old_vector);
                    utl_Vector_free(vector);
                    return false;
                }
                utl_Vector_append(vector, element);
            }

            const pyutl_ModuleState* state = pyutl_ModuleState_get();
            PyObject* old_obj = utl_PtrMap_search(state->objects_cache, old_vector);
            Py_XDECREF(old_obj);

            break;
        }
    }
    return true;
}

void Py_TLObject_dealloc_recursive(utl_Message* message) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();

    for(size_t i = 0; i < message->message_def->fields_num; i++) {
        utl_FieldDef field = message->message_def->fields[i];
        if(field.type == TLOBJECT) {
            utl_Message* f_message = utl_Message_getMessage(message, &field);
            if(!f_message) {
                continue;
            }

            PyObject* obj = utl_PtrMap_search(state->objects_cache, f_message);
            if(obj) {
                Py_DECREF(obj);
            } else {
                Py_TLObject_dealloc_recursive(f_message);
            }

            utl_Message_setMessage(message, &field, NULL);
        } else if(field.type == VECTOR) {
            utl_Vector* f_vec = utl_Message_getVector(message, &field);
            if(!f_vec) {
                continue;
            }

            PyObject* obj = utl_PtrMap_search(state->objects_cache, f_vec);
            if(obj) {
                Py_DECREF(obj);
            } else {
                Py_TLVector_dealloc_recursive(f_vec);
            }

            utl_Message_setVector(message, &field, NULL);
        }
    }

    utl_Message_free(message);
}

static void Py_TLObject_dealloc(PyObject* self) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    utl_PtrMap_remove(state->objects_cache, ((Py_TLObject*)self)->message);

    Py_TLObject_dealloc_recursive(((Py_TLObject*)self)->message);
    self->ob_type->tp_free(self);
}

void Py_TLObject_init_message(Py_TLObject* self, utl_MessageDef* def, utl_Message* message) {
    if(message != NULL) {
        self->message = message;
    } else {
        self->message = utl_Message_new(def);
    }

    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    utl_PtrMap_insert(state->objects_cache, self->message, self);
}

static PyObject* Py_TLObject_new(PyTypeObject* cls, PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyObject* result = PyObject_GetAttrString((PyObject*)cls, "__message_def__");
    if(!result) {
        PyErr_SetString(PyExc_NotImplementedError, "Object of type \"TLObject\" cannot be instantiated.");
        return 0;
    }

    PyObject* self = cls->tp_alloc(cls, 0);
    utl_MessageDef* def = PyCapsule_GetPointer(result, NULL);
    Py_TLObject_init_message((Py_TLObject*)self, def, NULL);

    return self;
}

static int Py_TLObject_init(const Py_TLObject* self, PyObject* Py_UNUSED(args), PyObject* kwargs) {
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
        if(Py_IsNone(item)) {
            if(!field.flag_info) {
                PyErr_SetString(PyExc_TypeError, "field is not optional");
                return 0;
            }
            utl_Message_clearField(self->message, &field);
            continue;
        }

        if(!Py_TLObject_setitem(self, &field, item)) {
            return 0;
        }
    }

    return 0;
}

static PyObject* Py_TLObject_getattro(Py_TLObject* self, PyObject* attr) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    pyutl_MessageDef* cached = utl_Map_search_uint64(state->messages_cache, (uint64_t)self->message->message_def);
    if(!cached) {
        return NULL;
    }

    ssize_t len;
    char *buf = (char*)PyUnicode_AsUTF8AndSize(attr, &len);
    if(!buf) {
        return NULL;
    }

    const utl_StringView field_name = { .data = buf, .size = len };
    utl_FieldDef* field = utl_Map_search_str(cached->fields, field_name);
    if(!field) {
        return PyObject_GenericGetAttr((PyObject*)self, attr);
    }

    if(!utl_Message_hasField(self->message, field)) {
        return Py_None;
    }

    return Py_TLObject_getitem(self, field);
}

static int Py_TLObject_setattro(const Py_TLObject* self, PyObject* attr, PyObject* value) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    pyutl_MessageDef* cached = utl_Map_search_uint64(state->messages_cache, (uint64_t)self->message->message_def);
    if(!cached) {
        return -1;
    }

    ssize_t len;
    char *buf = (char*)PyUnicode_AsUTF8AndSize(attr, &len);
    if(!buf) {
        return -1;
    }

    const utl_StringView field_name = { .data = buf, .size = len };
    utl_FieldDef* field = utl_Map_search_str(cached->fields, field_name);
    if(!field) {
        return -1;
    }

    return Py_TLObject_setitem(self, field, value) ? 0 : -1;
}

static PyObject* Py_TLObject_repr(const Py_TLObject* self) {
    arena_t repr_arena = arena_new();
    repr_arena.flags |= ARENA_DONTALIGN;
    char* tmp;

    const utl_MessageDef* def = self->message->message_def;

    if(def->namespace_.size) {
        tmp = arena_alloc(&repr_arena, def->namespace_.size);
        memcpy(tmp, def->namespace_.data, def->namespace_.size);
        tmp = arena_alloc(&repr_arena, 1);
        *tmp = '.';
    }

    tmp = arena_alloc(&repr_arena, def->name.size);
    memcpy(tmp, def->name.data, def->name.size);
    tmp = arena_alloc(&repr_arena, 1);
    *tmp = '(';

    for(size_t i = 0; i < def->fields_num; i++) {
        utl_FieldDef field = def->fields[i];
        tmp = arena_alloc(&repr_arena, field.name.size);
        memcpy(tmp, field.name.data, field.name.size);
        tmp = arena_alloc(&repr_arena, 1);
        *tmp = '=';

        PyObject* value;
        if(!utl_Message_hasField(self->message, &field)) {
            value = Py_None;
        } else {
            value = Py_TLObject_getitem(self, &field);
        }

        PyObject* repr = PyObject_Repr(value);
        if(repr) {
            ssize_t len;
            const char *buf = PyUnicode_AsUTF8AndSize(repr, &len);
            if(buf) {
                tmp = arena_alloc(&repr_arena, len);
                memcpy(tmp, buf, len);
            } else {
                Py_XDECREF(repr);
                repr = NULL;
            }
        }

        if(!repr) {
            tmp = arena_alloc(&repr_arena, 6);
            tmp[0] = '<';
            tmp[1] = 'N';
            tmp[2] = 'U';
            tmp[3] = 'L';
            tmp[4] = 'L';
            tmp[5] = '>';
        }

        Py_XDECREF(value);
        Py_XDECREF(repr);

        if(i != def->fields_num - 1) {
            tmp = arena_alloc(&repr_arena, 2);
            tmp[0] = ',';
            tmp[1] = ' ';
        }
    }

    tmp = arena_alloc(&repr_arena, 1);
    *tmp = ')';

    PyObject* result = PyUnicode_FromStringAndSize(repr_arena.data, repr_arena.size);
    arena_delete(&repr_arena);
    return result;
}

static PyObject* Py_TLObject_compare(const Py_TLObject* self, PyObject* other_, const int op) {
    if(op != Py_EQ && op != Py_NE) {
        return Py_NotImplemented;
    }

    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    if(!PyObject_TypeCheck(other_, state->tlobject_type)) {
        return Py_False;
    }

    const Py_TLObject* other = (Py_TLObject*)other_;
    bool eq = utl_Message_equals(self->message, other->message);
    if(op == Py_NE) {
        eq = !eq;
    }

    return eq ? Py_True : Py_False;
}

static PyObject* Py_TLObject_read(PyTypeObject* cls, char* buf, size_t buf_len, size_t* bytes_read) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();

    PyObject* result = PyObject_GetAttrString((PyObject*)cls, "__message_def__");
    if(!result) {
        if(buf_len < 4) {
            PyErr_SetString(PyExc_ValueError, "need at least 4 bytes");
            return NULL;
        }
        utl_DecodeBuf dbuf = {
            .data = buf,
            .pos = 0,
            .size = 4,
        };
        const uint32_t tl_id = utl_decode_int32(&dbuf);
        utl_MessageDef* def = utl_DefPool_getMessage(state->c_def_pool, tl_id);
        if (!def) {
            PyErr_SetString(PyExc_TypeError, "Unknown object id");
            return NULL;
        }

        pyutl_MessageDef* cached_def = utl_Map_search_uint64(state->messages_cache, (uint64_t)def);
        if(!cached_def) {
            PyErr_SetString(PyExc_TypeError, "object type is not found");
            return NULL;
        }

        cls = cached_def->python_cls;
        buf += 4;
        buf_len -= 4;
    }

    Py_TLObject* obj = (Py_TLObject*)Py_TLObject_new(cls, NULL, NULL);

    utl_Status status;
    const size_t read = utl_decode(obj->message, state->c_def_pool, buf, buf_len, &status);
    if(!status.ok) {
        PyErr_SetString(PyExc_ValueError, status.message);
        return NULL;
    }
    if(bytes_read) {
        *bytes_read = read;
    }

    return (PyObject*)obj;
}

static PyObject* Py_TLObject_read_bytesio(PyTypeObject* cls, PyObject* args) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();

    PyObject* bio;
    if (!PyArg_ParseTuple(args, "O!", state->bytesio_type, &bio)) {
        return NULL;
    }

    PyObject* memoryview = PyObject_CallMethod(bio, "getbuffer", NULL);
    if(!memoryview) {
        return NULL;
    }

    Py_buffer* view = PyMemoryView_GET_BUFFER(memoryview);
    if(!view) {
        Py_XDECREF(memoryview);
        return NULL;
    }

    size_t read = 0;
    PyObject* result = Py_TLObject_read(cls, view->buf, view->len, &read);

    Py_XDECREF(memoryview);

    if(result) {
        Py_XDECREF(PyObject_CallMethod(bio, "seek", "ki", read, 1)); // SEEK_CUR
    }

    return result;
}

static PyObject* Py_TLObject_read_bytes(PyTypeObject* cls, PyObject* args) {
    char* buf;
    size_t buf_len;
    if (!PyArg_ParseTuple(args, "s#", &buf, &buf_len)) {
        return NULL;
    }

    return Py_TLObject_read(cls, buf, buf_len, NULL);
}

static PyObject* Py_TLObject_write(const Py_TLObject* self, PyObject* Py_UNUSED(args)) {
    arena_t encoder_arena = arena_new();
    encoder_arena.flags |= ARENA_DONTALIGN;
    const size_t written_bytes = utl_encode(self->message, &encoder_arena);

    PyObject* result = PyBytes_FromStringAndSize(encoder_arena.data + sizeof(uint32_t*) * self->message->message_def->flags_num, written_bytes);
    arena_delete(&encoder_arena);

    return result;
}

static PyMethodDef Py_TLObject_methods[] = {
    {"read", (PyCFunction)Py_TLObject_read_bytesio, METH_VARARGS | METH_CLASS, 0,},
    {"read_bytes", (PyCFunction)Py_TLObject_read_bytes, METH_VARARGS | METH_CLASS, 0,},
    {"write", (PyCFunction)Py_TLObject_write, METH_NOARGS, 0,},
    {NULL}
};

static PyType_Slot Py_TLObject_slots[] = {
    {Py_tp_dealloc, Py_TLObject_dealloc},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {Py_tp_methods, Py_TLObject_methods},
    {Py_tp_init, Py_TLObject_init},
    {Py_tp_getattro, Py_TLObject_getattro},
    {Py_tp_setattro, Py_TLObject_setattro},
    {Py_tp_repr, Py_TLObject_repr},
    {Py_tp_str, Py_TLObject_repr},
    {Py_tp_richcompare, Py_TLObject_compare},
    {0, NULL}
};

// TODO: allow to create utl_MessageDef from type-annotated python class
PyType_Spec pyutl_TLObjectType_spec = {
    "_pyutl.TLObject",
    sizeof(Py_TLObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_IS_ABSTRACT,
    Py_TLObject_slots,
};

PyObject* Py_TLObject_createType(utl_MessageDef* message_def) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    arena_t* arena = &state->c_def_pool->arena;
    const size_t real_size = arena->size;

    char* name = arena_alloc(
        arena, 11 + (message_def->namespace_.size ? message_def->namespace_.size + 1 : 0) + message_def->name.size);
    memcpy(name, "_pyutl._tl.", 11);
    if(message_def->namespace_.size)
        memcpy(name+11, message_def->namespace_.data, message_def->namespace_.size);
    memcpy(name+11+message_def->namespace_.size, message_def->name.data, message_def->name.size);

    PyType_Slot slots[] = {
        {Py_tp_base, state->tlobject_type},
        {Py_tp_new, Py_TLObject_new},
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
        goto failed;
    }

    if (PyObject_SetAttrString(new_type, "__message_def__", msgdef_capsule) < 0 ||
        PyObject_SetAttrString(new_type, "__tl_id__", PyLong_FromUnsignedLong(message_def->id)) < 0 ||
        PyObject_SetAttrString(new_type, "__layer__", PyLong_FromUnsignedLong(message_def->layer)) < 0 ||
        PyObject_SetAttrString(new_type, "__section__", PyLong_FromUnsignedLong(message_def->section)) < 0 ||
        PyObject_SetAttrString(new_type, "__tl__", Py_None) < 0) {
        goto failed;
    }

    Py_DECREF(msgdef_capsule);

    return new_type;

failed:
    Py_XDECREF(msgdef_capsule);
    Py_XDECREF(new_type);
    return NULL;
}