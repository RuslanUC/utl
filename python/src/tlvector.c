#include "pyutl.h"
#include "tlvector.h"
#include "tlobject.h"

static PyObject* Py_TLVector_getitem(Py_TLVector* self, size_t index) {
    void* value = utl_Vector_value(self->vector, index);

    switch (self->vector->message_def->type) {
        case FLAGS:
        case INT32: {
            return PyLong_FromLong(((utl_Int32*)value)->value);
        }
        case INT64: {
            return PyLong_FromLong(((utl_Int64*)value)->value);
        }
        case INT128: {
            char* bytes = ((utl_Int128*)value)->value;
            return _PyLong_FromByteArray((uint8_t*)bytes, 16, true, true);
        }
        case INT256: {
            char* bytes = ((utl_Int256*)value)->value;
            return _PyLong_FromByteArray((uint8_t*)bytes, 32, true, true);
        }
        case DOUBLE: {
            return PyFloat_FromDouble(((utl_Double*)value)->value);
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            return ((utl_Bool*)value)->value ? Py_True : Py_False;
        }
        case BYTES: {
            utl_StringView bytes = ((utl_Bytes*)value)->value;
            return PyBytes_FromStringAndSize(bytes.data, bytes.size);
        }
        case STRING: {
            utl_StringView bytes = ((utl_Bytes*)value)->value;
            return PyUnicode_FromStringAndSize(bytes.data, bytes.size);
        }
        case TLOBJECT: {
            pyutl_ModuleState* state = pyutl_ModuleState_get();
            utl_Message* message = (utl_Message*)value;

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
            pyutl_ModuleState* state = pyutl_ModuleState_get();
            utl_Vector* vector = (utl_Vector*)value;

            PyObject* obj = utl_PtrMap_search(state->objects_cache, vector);
            if(!obj) {
                pyutl_MessageDef* cached_def = utl_Map_search_uint64(state->messages_cache, (uint64_t)vector->message_def);
                if(!cached_def) {
                    PyErr_SetString(PyExc_TypeError, "object type is not found");
                    return NULL;
                }

                obj = cached_def->python_cls->tp_alloc(cached_def->python_cls, 0);
                Py_TLVector_init_message((Py_TLVector*)obj, vector);
            }

            Py_INCREF(obj);
            return obj;
        }
    }

    return Py_None;
}

void* Py_TLVector_item_to_utl(utl_Vector* vector, PyObject* item) {
    void* result = NULL;

    switch (vector->message_def->type) {
        case FLAGS:
        case INT32: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return NULL;
            }
            result = arena_alloc(&vector->arena, sizeof(utl_Int32));
            ((utl_Int32*)result)->value = PyLong_AsLong(item);
            break;
        }
        case INT64: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return NULL;
            }
            result = arena_alloc(&vector->arena, sizeof(utl_Int64));
            ((utl_Int64*)result)->value = PyLong_AsLong(item);
            break;
        }
        case INT128: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return NULL;
            }
            result = arena_alloc(&vector->arena, sizeof(utl_Int128));
            _PyLong_AsByteArray((PyLongObject*)item, ((utl_Int128*)result)->value, 16, true, true);
            break;
        }
        case INT256: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return NULL;
            }
            result = arena_alloc(&vector->arena, sizeof(utl_Int256));
            _PyLong_AsByteArray((PyLongObject*)item, ((utl_Int256*)result)->value, 16, true, true);
            break;
        }
        case DOUBLE: {
            if(!PyFloat_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"float\"");
                return NULL;
            }
            result = arena_alloc(&vector->arena, sizeof(utl_Double));
            ((utl_Double*)result)->value = PyFloat_AsDouble(item);
            break;
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            if(!PyBool_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"bool\"");
                return NULL;
            }
            result = arena_alloc(&vector->arena, sizeof(utl_Bool));
            ((utl_Bool*)result)->value = item = Py_True;
            break;
        }
        case BYTES: {
            if(!PyBytes_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"bytes\"");
                return NULL;
            }
            char* buf;
            ssize_t len;
            if(PyBytes_AsStringAndSize(item, &buf, &len)) {
                return NULL;
            }
            result = arena_alloc(&vector->arena, sizeof(utl_Bytes));
            ((utl_Bytes*)result)->value = utl_StringView_new(&vector->arena, len);
            memcpy(((utl_Bytes*)result)->value.data, buf, len);
            break;
        }
        case STRING: {
            if(!PyUnicode_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"str\"");
                return NULL;
            }
            ssize_t len;
            const char *buf = PyUnicode_AsUTF8AndSize(item, &len);
            if(!buf) {
                return NULL;
            }
            result = arena_alloc(&vector->arena, sizeof(utl_Bytes));
            ((utl_Bytes*)result)->value = utl_StringView_new(&vector->arena, len);
            memcpy(((utl_Bytes*)result)->value.data, buf, len);
            break;
        }
        case TLOBJECT: {
            pyutl_ModuleState* state = pyutl_ModuleState_get();
            if(!PyObject_TypeCheck(item, state->tlobject_type)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\"");
                return NULL;
            }
            utl_Message* message = ((Py_TLObject*)item)->message;
            if(vector->message_def->sub_message_def != NULL && (utl_TypeDef*)vector->message_def->sub_message_def != message->message_def->type) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\" (TODO: show exact type)");
                return NULL;
            }

            // TODO: decref old message?
            result = message;
            Py_INCREF(item);
            break;
        }
        case VECTOR: {
            if(!PyList_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"list\"");
                return false;
            }

            size_t len = PyList_Size(item);
            utl_Vector* new_vector = utl_Vector_new((utl_MessageDefVector*)vector->message_def->sub_message_def, len);

            for(size_t i = 0; i < len; i++) {
                void* element = Py_TLVector_item_to_utl(new_vector, PyList_GetItem(item, i));
                if(!element) {
                    utl_Vector_free(new_vector);
                    return false;
                }
                utl_Vector_append(new_vector, element);
            }

            result = new_vector;
            break;
        }
    }

    return result;
}

static void Py_TLVector_dealloc(PyObject* self) {
    pyutl_ModuleState* state = pyutl_ModuleState_get();
    utl_PtrMap_remove(state->objects_cache, ((Py_TLVector*)self)->vector);

    utl_Vector_free(((Py_TLVector*)self)->vector);
    self->ob_type->tp_free(self);
}

void Py_TLVector_init_message(Py_TLVector* self, utl_Vector* vector) {
    self->vector = vector;

    pyutl_ModuleState* state = pyutl_ModuleState_get();
    utl_PtrMap_insert(state->objects_cache, self->vector, self);
}

static PyObject* Py_TLVector_new(PyTypeObject* Py_UNUSED(cls), PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyErr_SetString(PyExc_RuntimeError,
               "Type TLVector cannot be created directly.");

    /*PyObject* self = cls->tp_alloc(cls, 0);
    Py_TLVector_init_message((Py_TLVector*)self, NULL);*/

    return NULL;
}

static PyObject* Py_TLVector_sq_item(Py_TLVector* self, ssize_t index) {
    if(index >= utl_Vector_size(self->vector)) {
        // TODO: set error
        return NULL;
    }

    return Py_TLVector_getitem(self, index);
}

static int Py_TLVector_sq_setitem(Py_TLVector* self, ssize_t index, PyObject* value) {
    if(value == NULL) {
        // TODO: remove element
        return 0;
    }

    if(index >= utl_Vector_size(self->vector)) {
        // TODO: set error
        return -1;
    }

    void* item = Py_TLVector_item_to_utl(self->vector, value);
    if(!item) {
        // TODO: set error
        return -1;
    }

    utl_Vector_setValue(self->vector, index, item);
    return 0;
}

static PyObject* Py_TLVector_repr(Py_TLVector* self) {
    arena_t repr_arena = arena_new();
    repr_arena.flags |= ARENA_DONTALIGN;
    char* tmp;

    tmp = arena_alloc(&repr_arena, 1);
    *tmp = '[';

    size_t size = utl_Vector_size(self->vector);
    for(size_t i = 0; i < size; i++) {
        PyObject* value = Py_TLVector_getitem(self, i);

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

        if(i != size - 1) {
            tmp = arena_alloc(&repr_arena, 2);
            tmp[0] = ',';
            tmp[1] = ' ';
        }
    }

    tmp = arena_alloc(&repr_arena, 1);
    *tmp = ']';

    PyObject* result = PyUnicode_FromStringAndSize(repr_arena.data, repr_arena.size);
    arena_delete(&repr_arena);
    return result;
}

static PyObject* Py_TLVector_compare(Py_TLVector* self, PyObject* other_, int op) {
    if(op != Py_EQ && op != Py_NE) {
        return Py_NotImplemented;
    }

    pyutl_ModuleState* state = pyutl_ModuleState_get();
    if(!PyObject_TypeCheck(other_, state->tlvector_type)) {
        return Py_False;
    }

    Py_TLVector* other = (Py_TLVector*)other_;
    bool eq = utl_Vector_equals(self->vector, other->vector);
    if(op == Py_NE) {
        eq = !eq;
    }

    return eq ? Py_True : Py_False;
}

static PyObject* Py_TLVector_append(Py_TLVector* self, PyObject* args) {
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    void* item = Py_TLVector_item_to_utl(self->vector, obj);
    if(!item) {
        return NULL;
    }
    utl_Vector_append(self->vector, item);

    return Py_None;
}

static PyMethodDef Py_TLVector_methods[] = {
    {"append", (PyCFunction)Py_TLVector_append, METH_VARARGS, 0,},
    {NULL}
};

static PyType_Slot Py_TLVector_slots[] = {
    {Py_tp_dealloc, Py_TLVector_dealloc},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {Py_tp_methods, Py_TLVector_methods},
    {Py_tp_new, Py_TLVector_new},
    {Py_sq_item, Py_TLVector_sq_item},
    {Py_sq_ass_item, Py_TLVector_sq_setitem},
    {Py_tp_repr, Py_TLVector_repr},
    {Py_tp_str, Py_TLVector_repr},
    {Py_tp_richcompare, Py_TLVector_compare},
    {0, NULL}
};

PyType_Spec pyutl_TLVectorType_spec = {
    "_pyutl.TLVector",
    sizeof(Py_TLVector),
    0,
    Py_TPFLAGS_DEFAULT,
    Py_TLVector_slots,
};
