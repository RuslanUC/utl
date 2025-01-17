#include "pyutl.h"
#include "tlvector.h"
#include "builtins.h"

#include <encoder.h>

#include "tlobject.h"

static PyObject* Py_TLVector_getitem(const Py_TLVector* self, const size_t index) {
    switch (self->vector->message_def->type) {
        case FLAGS:
        case INT32: {
            return PyLong_FromLong(utl_Vector_getInt32(self->vector, index));
        }
        case INT64: {
            return PyLong_FromLong(utl_Vector_getInt64(self->vector, index));
        }
        case INT128: {
            const utl_Int128 value = utl_Vector_getInt128(self->vector, index);
            return _PyLong_FromByteArray(value.value, 16, true, true);
        }
        case INT256: {
            const utl_Int256 value = utl_Vector_getInt256(self->vector, index);
            return _PyLong_FromByteArray(value.value, 32, true, true);
        }
        case DOUBLE: {
            return PyFloat_FromDouble(utl_Vector_getDouble(self->vector, index));
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            if(utl_Vector_getBool(self->vector, index))
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        }
        case BYTES: {
            const utl_StringView bytes = utl_Vector_getBytes(self->vector, index);
            return PyBytes_FromStringAndSize(bytes.data, bytes.size);
        }
        case STRING: {
            const utl_StringView bytes = utl_Vector_getString(self->vector, index);
            return PyUnicode_FromStringAndSize(bytes.data, bytes.size);
        }
        case TLOBJECT: {
            const pyutl_ModuleState* state = pyutl_ModuleState_get();
            utl_Message* message = utl_Vector_getMessage(self->vector, index);

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
            utl_Vector* vector = utl_Vector_getVector(self->vector, index);

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

    Py_RETURN_NONE;
}

bool Py_TLVector_item_set(utl_Vector* vector, PyObject* item, ssize_t index) {
    switch (vector->message_def->type) {
        case FLAGS:
        case INT32: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            index >= 0 ? utl_Vector_setInt32(vector, index, PyLong_AsLong(item)) : utl_Vector_appendInt32(vector, PyLong_AsLong(item));
            return true;
        }
        case INT64: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            index >= 0 ? utl_Vector_setInt64(vector, index, PyLong_AsLong(item)) : utl_Vector_appendInt64(vector, PyLong_AsLong(item));
            return true;
        }
        case INT128: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            utl_Int128 result;
#if PY_MINOR_VERSION < 13
            _PyLong_AsByteArray((PyLongObject*)item, result.value, 16, true, true);
#else
            _PyLong_AsByteArray((PyLongObject*)item, result.value, 16, true, true, true);
#endif
            index >= 0 ? utl_Vector_setInt128(vector, index, result) : utl_Vector_appendInt128(vector, result);
            return true;
        }
        case INT256: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            utl_Int256 result;
#if PY_MINOR_VERSION < 13
            _PyLong_AsByteArray((PyLongObject*)item, result.value, 32, true, true);
#else
            _PyLong_AsByteArray((PyLongObject*)item, result.value, 32, true, true, true);
#endif
            index >= 0 ? utl_Vector_setInt256(vector, index, result) : utl_Vector_appendInt256(vector, result);
            return true;
        }
        case DOUBLE: {
            if(!PyFloat_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"float\"");
                return false;
            }
            index >= 0 ? utl_Vector_setDouble(vector, index, PyFloat_AsDouble(item)) : utl_Vector_appendDouble(vector, PyFloat_AsDouble(item));
            return true;
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            if(!PyBool_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"bool\"");
                return false;
            }
            index >= 0 ? utl_Vector_setBool(vector, index, item == Py_True) : utl_Vector_appendBool(vector, item == Py_True);
            return true;
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
            const utl_StringView bytes = {.size = len, .data = buf};
            index >= 0 ? utl_Vector_setBytes(vector, index, bytes) : utl_Vector_appendBytes(vector, bytes);
            return true;
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
            const utl_StringView bytes = {.size = len, .data = (char*)buf};
            index >= 0 ? utl_Vector_setString(vector, index, bytes) : utl_Vector_appendString(vector, bytes);
            return true;
        }
        case TLOBJECT: {
            const pyutl_ModuleState* state = pyutl_ModuleState_get();
            if(!PyObject_TypeCheck(item, state->tlobject_type)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\"");
                return false;
            }
            utl_Message* message = ((Py_TLObject*)item)->message;
            if(vector->message_def->sub.type_def != NULL && vector->message_def->sub.type_def != message->message_def->type) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\" (TODO: show exact type)");
                return false;
            }

            index >= 0 ? utl_Vector_setMessage(vector, index, message) : utl_Vector_appendMessage(vector, message);
            Py_INCREF(item);
            return true;
        }
        case VECTOR: {
            if(!PyList_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"list\"");
                return false;
            }

            const size_t len = PyList_Size(item);
            utl_Vector* new_vector = utl_Vector_new(vector->message_def->sub.vector_def, len);

            for(size_t i = 0; i < len; i++) {
                if(!Py_TLVector_item_set(new_vector, PyList_GetItem(item, i), i)) {
                    utl_Vector_free(new_vector);
                    return false;
                }
            }

            index >= 0 ? utl_Vector_setVector(vector, index, new_vector) : utl_Vector_appendVector(vector, new_vector);
            return true;
        }
    }

    return false;
}

void Py_TLVector_dealloc_recursive(utl_Vector* vector) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();

    const utl_FieldType type = vector->message_def->type;
    if(type != TLOBJECT && type != VECTOR) {
        goto vec_free;
    }

    const size_t size = utl_Vector_size(vector);
    for(size_t i = 0; i < size; i++) {
        void* element = type == TLOBJECT ? (void*)utl_Vector_getMessage(vector, i) : (void*)utl_Vector_getVector(vector, i);
        if(!element) {
            continue;
        }

        PyObject* obj = utl_PtrMap_search(state->objects_cache, element);
        if(obj) {
            Py_DECREF(obj);
        } else {
            if(type == TLOBJECT) {
                Py_TLObject_dealloc_recursive(element);
            } else if(type == VECTOR) {
                Py_TLVector_dealloc_recursive(element);
            }
        }

        type == TLOBJECT ? utl_Vector_setMessage(vector, i, NULL) : utl_Vector_setVector(vector, i, NULL);
    }

vec_free:
    utl_Vector_free(vector);
}

static void Py_TLVector_dealloc(PyObject* self) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    utl_PtrMap_remove(state->objects_cache, ((Py_TLVector*)self)->vector);

    Py_TLVector_dealloc_recursive(((Py_TLVector*)self)->vector);
    self->ob_type->tp_free(self);
}

void Py_TLVector_init_message(Py_TLVector* self, utl_Vector* vector) {
    self->vector = vector;

    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    utl_PtrMap_insert(state->objects_cache, self->vector, self);
}

static PyObject* Py_TLVector_new(PyTypeObject* Py_UNUSED(cls), PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyErr_SetString(PyExc_RuntimeError, "Type TLVector cannot be created directly.");

    return NULL;
}

static PyObject* Py_TLVector_sq_item(const Py_TLVector* self, const ssize_t index) {
    if(index >= utl_Vector_size(self->vector)) {
        PyErr_SetString(PyExc_IndexError, "list index out of range");
        return NULL;
    }

    return Py_TLVector_getitem(self, index);
}

static int Py_TLVector_sq_setitem(const Py_TLVector* self, const ssize_t index, PyObject* value) {
    if(value == NULL) {
        // TODO: dealloc/decref value
        utl_Vector_remove(self->vector, index);
        return 0;
    }

    if(index >= utl_Vector_size(self->vector)) {
        PyErr_SetString(PyExc_IndexError, "list index out of range");
        return -1;
    }

    // TODO: dealloc/decref old value
    if(!Py_TLVector_item_set(self->vector, value, index)) {
        return -1;
    }

    return 0;
}

static PyObject* Py_TLVector_repr(const Py_TLVector* self) {
    const size_t alloc_size = utl_Vector_size(self->vector) * 8;
    utl_EncodeBuf repr_buf = {
        .data = malloc(alloc_size),
        .pos = 0,
        .size = alloc_size,
    };

    char* tmp = utl_EncodeBuf_alloc(&repr_buf, 1);
    *tmp = '[';

    const size_t size = utl_Vector_size(self->vector);
    for(size_t i = 0; i < size; i++) {
        PyObject* value = Py_TLVector_getitem(self, i);

        PyObject* repr = PyObject_Repr(value);
        if(repr) {
            ssize_t len;
            const char *buf = PyUnicode_AsUTF8AndSize(repr, &len);
            if(buf) {
                tmp = utl_EncodeBuf_alloc(&repr_buf, len);
                memcpy(tmp, buf, len);
            } else {
                Py_XDECREF(repr);
                repr = NULL;
            }
        }

        if(!repr) {
            tmp = utl_EncodeBuf_alloc(&repr_buf, 6);
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
            tmp = utl_EncodeBuf_alloc(&repr_buf, 2);
            tmp[0] = ',';
            tmp[1] = ' ';
        }
    }

    tmp = utl_EncodeBuf_alloc(&repr_buf, 1);
    *tmp = ']';

    PyObject* result = PyUnicode_FromStringAndSize(repr_buf.data, repr_buf.pos);
    free(repr_buf.data);

    return result;
}

static PyObject* Py_TLVector_compare(const Py_TLVector* self, PyObject* other_, const int op) {
    if(op != Py_EQ && op != Py_NE) {
        return Py_NotImplemented;
    }

    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    if(!PyObject_TypeCheck(other_, state->tlvector_type)) {
        return Py_False;
    }

    const Py_TLVector* other = (Py_TLVector*)other_;
    bool eq = utl_Vector_equals(self->vector, other->vector);
    if(op == Py_NE) {
        eq = !eq;
    }

    if(eq)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* Py_TLVector_append(const Py_TLVector* self, PyObject* args) {
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    if(!Py_TLVector_item_set(self->vector, obj, -1)) {
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* Py_TLVector_remove(const Py_TLVector* self, PyObject* args) {
    uint32_t index;
    if (!PyArg_ParseTuple(args, "I", &index)) {
        return NULL;
    }

    // TODO: dealloc/decref value
    utl_Vector_remove(self->vector, index);
    Py_RETURN_NONE;
}

static PyMethodDef Py_TLVector_methods[] = {
    {"append", (PyCFunction)Py_TLVector_append, METH_VARARGS, 0,},
    {"remove", (PyCFunction)Py_TLVector_remove, METH_VARARGS, 0,},
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
