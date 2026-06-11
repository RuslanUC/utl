#include "pyutl.h"
#include "tlvector.h"
#include "builtins.h"

#include <encoder.h>

#include "tlobject.h"
#include "constants.h"
#include "py_def_pool.h"

static PyObject* Py_TLVector_getitem_regular(const Py_TLVector* self, const int32_t index) {
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
            return _PyLong_FromByteArray((uint8_t*)value.value, 16, true, true);
        }
        case INT256: {
            const utl_Int256 value = utl_Vector_getInt256(self->vector, index);
            return _PyLong_FromByteArray((uint8_t*)value.value, 32, true, true);
        }
        case DOUBLE: {
            return PyFloat_FromDouble(utl_Vector_getDouble(self->vector, index));
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            const bool res = utl_Vector_getBool(self->vector, index);
            if(res)
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        }
        case BYTES: {
            const utl_StringView bytes = utl_Vector_getBytes(self->vector, index);
            return PyBytes_FromStringAndSize(bytes.data, (ssize_t)bytes.size);
        }
        case STRING: {
            const utl_StringView bytes = utl_Vector_getString(self->vector, index);
            return PyUnicode_FromStringAndSize(bytes.data, (ssize_t)bytes.size);
        }
        case TLOBJECT: {
            utl_Message* message = utl_Vector_getMessage(self->vector, index);
            if(message->userdata != NULL) {
                Py_INCREF(message->userdata);
                return message->userdata;
            }

            utl_MessageDef* message_def = ((utl_MessageHeader*)message)->message_def;
            pyutl_MessageDef* cached_def = Py_DefPool_get_or_create_cached_def(message_def);
            if(!cached_def)
                return NULL;

            PyObject* result_obj = cached_def->python_cls->tp_alloc(cached_def->python_cls, 0);
            Py_TLObject_init_message((Py_TLObject*)result_obj, NULL, message);

            return result_obj;
        }
        case VECTOR: {
            utl_Vector* vector = utl_Vector_getVector(self->vector, index);
            if(vector->userdata != NULL) {
                Py_INCREF(vector->userdata);
                return vector->userdata;
            }

            PyObject* result_obj = tlvector_type->tp_alloc(tlvector_type, 0);
            Py_TLVector_init_message((Py_TLVector*)result_obj, vector);

            break;
        }

        case STATIC_FIELDS_END:
        case ALL_FIELDS_END: return NULL;
    }

    return NULL;
}

static PyObject* Py_TLVector_getitem_readonly(const Py_TLVector* self, const int32_t index) {
    switch (self->vector->message_def->type) {
        case FLAGS:
        case INT32: {
            return PyLong_FromLong(utl_RoVector_getInt32(self->ro_vector, index));
        }
        case INT64: {
            return PyLong_FromLong(utl_RoVector_getInt64(self->ro_vector, index));
        }
        case INT128: {
            const utl_Int128 value = utl_RoVector_getInt128(self->ro_vector, index);
            return _PyLong_FromByteArray((uint8_t*)value.value, 16, true, true);
        }
        case INT256: {
            const utl_Int256 value = utl_RoVector_getInt256(self->ro_vector, index);
            return _PyLong_FromByteArray((uint8_t*)value.value, 32, true, true);
        }
        case DOUBLE: {
            return PyFloat_FromDouble( utl_RoVector_getDouble(self->ro_vector, index));
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            const bool res = utl_RoVector_getBool(self->ro_vector, index);
            if(res)
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        }
        case BYTES: {
            const utl_StringView bytes = utl_RoVector_getBytes(self->ro_vector, index);
            return PyBytes_FromStringAndSize(bytes.data, (ssize_t)bytes.size);
        }
        case STRING: {
            const utl_StringView bytes = utl_RoVector_getString(self->ro_vector, index);
            return PyUnicode_FromStringAndSize(bytes.data, (ssize_t)bytes.size);
        }
        case TLOBJECT: {
            utl_RoMessage* message = utl_RoVector_getMessage(self->ro_vector, index);
            if(message->userdata != NULL) {
                Py_INCREF(message->userdata);
                return message->userdata;
            }

            utl_MessageDef* message_def = ((utl_MessageHeader*)message)->message_def;

            pyutl_MessageDef* cached_def = Py_DefPool_get_or_create_cached_def(message_def);
            if(!cached_def)
                return NULL;

            PyObject* result_obj = cached_def->python_cls->tp_alloc(cached_def->python_cls, 0);
            Py_TLObject_init_message_ro((Py_TLObject*)result_obj, message);

            PyObject* bytes = self->out_refs[self->ro_vector->elements_count];
            ((Py_TLObject*)result_obj)->out_refs[message_def->fields_num] = bytes;
            Py_INCREF(bytes);

            break;
        }
        case VECTOR: {
            utl_RoVector* vector = utl_RoVector_getVector(self->ro_vector, index);
            if(vector->userdata != NULL) {
                Py_INCREF(vector->userdata);
                return vector->userdata;
            }

            PyObject* result_obj = tlvector_type->tp_alloc(tlvector_type, 0);
            Py_TLVector_init_message_ro((Py_TLVector*)result_obj, vector);
            PyObject* bytes = self->out_refs[self->ro_vector->elements_count];

            ((Py_TLObject*)result_obj)->out_refs[vector->elements_count] = bytes;
            Py_INCREF(bytes);

            break;
        }

        case STATIC_FIELDS_END:
        case ALL_FIELDS_END: return NULL;
    }

    return NULL;
}

static PyObject* Py_TLVector_getitem(const Py_TLVector* self, const int32_t index) {
    if(self->out_refs[index] != NULL) {
        PyObject* obj = self->out_refs[index];
        Py_INCREF(obj);
        return obj;
    }

    PyObject* result_obj;
    if(self->readonly)
        result_obj = Py_TLVector_getitem_readonly(self, index);
    else
        result_obj = Py_TLVector_getitem_regular(self, index);

    if(result_obj != NULL) {
        Py_INCREF(result_obj);
        self->out_refs[index] = result_obj;
        return result_obj;
    }

    Py_RETURN_NONE;
}

bool Py_TLVector_item_set(utl_Vector* vector, PyObject* item, const int32_t index) {
    switch (vector->message_def->type) {
        case FLAGS:
        case INT32: {
            if(!PyLong_Check(item)) {
                PyErr_Format(PyExc_TypeError, "expected object of type \"int\", got \"%s\"", Py_TYPE(item)->tp_name);
                return false;
            }
            const int32_t value = (int32_t)PyLong_AsLong(item);
            index >= 0 ? utl_Vector_setInt32(vector, index, value) : utl_Vector_appendInt32(vector, value);
            return true;
        }
        case INT64: {
            if(!PyLong_Check(item)) {
                PyErr_Format(PyExc_TypeError, "expected object of type \"int\", got \"%s\"", Py_TYPE(item)->tp_name);
                return false;
            }
            const int64_t value = PyLong_AsLong(item);
            index >= 0 ? utl_Vector_setInt64(vector, index, value) : utl_Vector_appendInt64(vector, value);
            return true;
        }
        case INT128: {
            if(!PyLong_Check(item)) {
                PyErr_Format(PyExc_TypeError, "expected object of type \"int\", got \"%s\"", Py_TYPE(item)->tp_name);
                return false;
            }
            utl_Int128 result;
#if PY_MINOR_VERSION < 13
            _PyLong_AsByteArray((PyLongObject*)item, (uint8_t*)result.value, 16, true, true);
#else
            _PyLong_AsByteArray((PyLongObject*)item, (uint8_t*)result.value, 16, true, true, true);
#endif
            index >= 0 ? utl_Vector_setInt128(vector, index, result) : utl_Vector_appendInt128(vector, result);
            return true;
        }
        case INT256: {
            if(!PyLong_Check(item)) {
                PyErr_Format(PyExc_TypeError, "expected object of type \"int\", got \"%s\"", Py_TYPE(item)->tp_name);
                return false;
            }
            utl_Int256 result;
#if PY_MINOR_VERSION < 13
            _PyLong_AsByteArray((PyLongObject*)item, (uint8_t*)result.value, 32, true, true);
#else
            _PyLong_AsByteArray((PyLongObject*)item, (uint8_t*)result.value, 32, true, true, true);
#endif
            index >= 0 ? utl_Vector_setInt256(vector, index, result) : utl_Vector_appendInt256(vector, result);
            return true;
        }
        case DOUBLE: {
            if(!PyFloat_Check(item)) {
                PyErr_Format(PyExc_TypeError, "expected object of type \"float\", got \"%s\"", Py_TYPE(item)->tp_name);
                return false;
            }
            const double value = PyFloat_AsDouble(item);
            index >= 0 ? utl_Vector_setDouble(vector, index, value) : utl_Vector_appendDouble(vector, value);
            return true;
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            if(!PyBool_Check(item)) {
                PyErr_Format(PyExc_TypeError, "expected object of type \"bool\", got \"%s\"", Py_TYPE(item)->tp_name);
                return false;
            }
            index >= 0 ? utl_Vector_setBool(vector, index, item == Py_True) : utl_Vector_appendBool(vector, item == Py_True);
            return true;
        }
        case BYTES: {
            if(!PyBytes_Check(item)) {
                PyErr_Format(PyExc_TypeError, "expected object of type \"bytes\", got \"%s\"", Py_TYPE(item)->tp_name);
                return false;
            }
            char* buf;
            ssize_t len;
            if(PyBytes_AsStringAndSize(item, &buf, &len)) {
                return false;
            }
            if(len > UTL_MAX_STRINT_LENGTH) {
                PyErr_Format(PyExc_ValueError, "bytes object is too big, maximum allowed length is %zu", UTL_MAX_STRINT_LENGTH);
                return false;
            }
            const utl_StringView bytes = {.size = len, .data = buf};
            index >= 0 ? utl_Vector_setBytes(vector, index, bytes) : utl_Vector_appendBytes(vector, bytes);
            return true;
        }
        case STRING: {
            if(!PyUnicode_Check(item)) {
                PyErr_Format(PyExc_TypeError, "expected object of type \"str\", got \"%s\"", Py_TYPE(item)->tp_name);
                return false;
            }
            ssize_t len;
            const char *buf = PyUnicode_AsUTF8AndSize(item, &len);
            if(!buf) {
                return false;
            }
            if(len > UTL_MAX_STRINT_LENGTH) {
                PyErr_Format(PyExc_ValueError, "string object is too big, maximum allowed length is %zu", UTL_MAX_STRINT_LENGTH);
                return false;
            }
            const utl_StringView bytes = {.size = len, .data = (char*)buf};
            index >= 0 ? utl_Vector_setString(vector, index, bytes) : utl_Vector_appendString(vector, bytes);
            return true;
        }
        case TLOBJECT: {
            if(!PyObject_TypeCheck(item, tlobject_type)) {
                PyErr_Format(PyExc_TypeError, "expected object of type \"TLObject\", got \"%s\"", Py_TYPE(item)->tp_name);
                return false;
            }
            utl_Message* message = ((Py_TLObject*)item)->message;
            if(vector->message_def->sub.type_def != NULL && vector->message_def->sub.type_def != message->message_def->type) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\" (TODO: show exact type)");
                return false;
            }
            if(((Py_TLObject*)item)->readonly) {
                PyErr_SetString(PyExc_TypeError, "setting read-only object in regular object is not allowed");
                return false;
            }

            if(index >= 0) {
                utl_Message* old = utl_Vector_getMessage(vector, index);
                // TODO: do a decref if userdata is not null?
                if(old != NULL && old->userdata == NULL) {
                    utl_Message_free(old);
                }
                utl_Vector_setMessage(vector, index, message);
            } else {
                utl_Vector_appendMessage(vector, message);
            }

            return true;
        }
        case VECTOR: {
            if(!PyList_Check(item)) {
                PyErr_Format(PyExc_TypeError, "expected object of type \"list\", got \"%s\"", Py_TYPE(item)->tp_name);
                return false;
            }

            const ssize_t len_ssize = PyList_Size(item);
            if(len_ssize > INT32_MAX || len_ssize < INT32_MIN) {
                PyErr_SetString(PyExc_TypeError, "invalid index, expected 32-bit number");
                return false;
            }

            const int32_t len = (int32_t)len_ssize;
            utl_Vector* new_vector = utl_Vector_new(vector->message_def->sub.vector_def, len);

            for(int32_t i = 0; i < len; i++) {
                if(!Py_TLVector_item_set(new_vector, PyList_GetItem(item, i), -1)) {
                    utl_Vector_free(new_vector);
                    return false;
                }
            }

            if(index >= 0) {
                utl_Vector* old = utl_Vector_getVector(vector, index);
                // TODO: do a decref if userdata is not null?
                if(old != NULL && old->userdata == NULL) {
                    utl_Vector_free(old);
                }
                utl_Vector_setVector(vector, index, new_vector);
            } else {
                utl_Vector_appendVector(vector, new_vector);
            }

            return true;
        }

        case STATIC_FIELDS_END:
        case ALL_FIELDS_END: return false;
    }

    return false;
}

void pyutl_internal_tlvector_free_recursive(utl_VectorHeader* obj, const bool is_readonly) {
    if(obj == NULL || obj->userdata != NULL)
        return;

    const utl_MessageDefVector def = *obj->message_def;
    if(is_readonly) {
        utl_RoVector* tlvec = (utl_RoVector*)obj;
        const int32_t size = utl_RoVector_size(tlvec);
        if(def.type == TLOBJECT) {
            for(int32_t i = 0; i < size; ++i) {
                pyutl_internal_tlobject_free_recursive((utl_MessageHeader*)utl_RoVector_getMessage(tlvec, i), true);
            }
        } else if(def.type == VECTOR) {
            for(int32_t i = 0; i < size; ++i) {
                pyutl_internal_tlvector_free_recursive((utl_VectorHeader*)utl_RoVector_getVector(tlvec, i), true);
            }
        }
        utl_RoVector_free(tlvec);
    } else {
        utl_Vector* tlvec = (utl_Vector*)obj;
        const int32_t size = utl_Vector_size(tlvec);
        if(def.type == TLOBJECT) {
            for(int32_t i = 0; i < size; ++i) {
                pyutl_internal_tlobject_free_recursive((utl_MessageHeader*)utl_Vector_getMessage(tlvec, i), false);
            }
        } else if(def.type == VECTOR) {
            for(int32_t i = 0; i < size; ++i) {
                pyutl_internal_tlvector_free_recursive((utl_VectorHeader*)utl_Vector_getVector(tlvec, i), false);
            }
        }
        utl_Vector_free(tlvec);
    }
}

static void Py_TLVector_dealloc(Py_TLVector* self) {
    const utl_MessageDefVector def = *self->vector_hdr->message_def;
    const bool is_readonly = self->readonly;
    const int32_t fields_count = is_readonly ? self->ro_vector->elements_count : self->vector->size;

    for(int32_t i = 0; i < fields_count + is_readonly; ++i) {
        Py_XDECREF(self->out_refs[i]);

        if(self->out_refs[i] == NULL) {
            if(def.type == TLOBJECT) {
                utl_MessageHeader* hdr = is_readonly
                                             ? (utl_MessageHeader*)utl_RoVector_getMessage(self->ro_vector, i)
                                             : (utl_MessageHeader*)utl_Vector_getMessage(self->vector, i);
                pyutl_internal_tlobject_free_recursive(hdr, is_readonly);
            } else if(def.type == VECTOR) {
                utl_VectorHeader* hdr = is_readonly
                                             ? (utl_VectorHeader*)utl_RoVector_getVector(self->ro_vector, i)
                                             : (utl_VectorHeader*)utl_Vector_getVector(self->vector, i);
                pyutl_internal_tlvector_free_recursive(hdr, is_readonly);
            }
        }
    }

    self->vector_hdr->userdata = NULL;

    if(is_readonly) {
        utl_RoVector_free(self->ro_vector);
    } else {
        utl_Vector_free(self->vector);
    }

    free(self->out_refs);
    ((PyObject*)self)->ob_type->tp_free(self);
}

void Py_TLVector_init_message(Py_TLVector* self, utl_Vector* vector) {
    self->readonly = 0;
    self->vector = vector;
    vector->userdata = self;

    const size_t vec_size = utl_Vector_size(vector);
    const size_t refs_capacity = (vec_size / 128 + 2) * 128;
    self->out_refs = calloc(refs_capacity, sizeof(void*));
}

void Py_TLVector_init_message_ro(Py_TLVector* self, utl_RoVector* vector) {
    self->readonly = 1;
    self->ro_vector = vector;
    vector->userdata = self;

    const size_t vec_size = utl_RoVector_size(vector);
    const size_t refs_capacity = ((vec_size + 1) / 128 + 2) * 128;
    self->out_refs = calloc(refs_capacity, sizeof(void*));
}

static PyObject* Py_TLVector_new(PyTypeObject* Py_UNUSED(cls), PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyErr_SetString(PyExc_RuntimeError, "Type TLVector cannot be created directly.");

    return NULL;
}

static PyObject* Py_TLVector_sq_item(const Py_TLVector* self, const ssize_t index_ssize) {
    if(index_ssize > INT32_MAX || index_ssize < INT32_MIN) {
        PyErr_SetString(PyExc_IndexError, "invalid index, expected 32-bit number");
        return NULL;
    }

    const int32_t index = (int32_t)index_ssize;
    const int32_t vector_size = self->readonly ? utl_RoVector_size(self->ro_vector) : utl_Vector_size(self->vector);
    if(index >= vector_size) {
        PyErr_SetString(PyExc_IndexError, "list index out of range");
        return NULL;
    }

    return Py_TLVector_getitem(self, index);
}

static void Py_TLVector_remove_at_index(const Py_TLVector* self, const ssize_t index_ssize) {
    if(index_ssize > INT32_MAX || index_ssize < INT32_MIN) {
        return;
    }

    const int32_t index = (int32_t)index_ssize;
    const PyObject* old_ref = self->out_refs[index];

    if (self->vector->message_def->type == TLOBJECT && old_ref == NULL) {
        utl_Message* message = utl_Vector_getMessage(self->vector, index);
        if(message != NULL) {
            utl_Message_free(message);
        }
    }
    else if(self->vector->message_def->type == VECTOR && old_ref == NULL) {
        utl_Vector* vector = utl_Vector_getVector(self->vector, index);
        if(vector != NULL) {
            utl_Vector_free(vector);
        }
    }

    Py_XDECREF(old_ref);
    utl_Vector_remove(self->vector, index);

    const size_t new_size = utl_Vector_size(self->vector);
    memcpy(self->out_refs + index, self->out_refs + index + 1, (new_size - index) * sizeof(void*));
    self->out_refs[new_size] = NULL;
}

static int Py_TLVector_sq_setitem(const Py_TLVector* self, const ssize_t index_ssize, PyObject* value) {
    if(self->readonly) {
        PyErr_SetString(PyExc_AttributeError, "Vector is read-only");
        return -1;
    }

    if(index_ssize > INT32_MAX || index_ssize < INT32_MIN) {
        PyErr_SetString(PyExc_IndexError, "invalid index, expected 32-bit number");
        return -1;
    }

    const int32_t index = (int32_t)index_ssize;

    if(index >= utl_Vector_size(self->vector) || (index < 0 && value == NULL)) {
        PyErr_SetString(PyExc_IndexError, "list index out of range");
        return -1;
    }

    Py_XDECREF(self->out_refs[index]);

    if(value == NULL) {
        Py_TLVector_remove_at_index(self, index);
        return 0;
    }

    if(!Py_TLVector_item_set(self->vector, value, index)) {
        return -1;
    }

    if(value != Py_True && value != Py_False) {
        self->out_refs[index] = value;
        Py_INCREF(value);
    }

    return 0;
}

static PyObject* Py_TLVector_repr(const Py_TLVector* self) {
    const int32_t size = self->readonly ? utl_RoVector_size(self->ro_vector) : utl_Vector_size(self->vector);
    const int32_t alloc_size = size * 8;

    utl_EncodeBuf repr_buf = {
        .data = malloc(alloc_size),
        .pos = 0,
        .size = alloc_size,
    };

    char* tmp = utl_EncodeBuf_alloc(&repr_buf, 1);
    *tmp = '[';

    for(int32_t i = 0; i < size; i++) {
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

    PyObject* result = PyUnicode_FromStringAndSize(repr_buf.data, (ssize_t)repr_buf.pos);
    free(repr_buf.data);

    return result;
}

static PyObject* Py_TLVector_compare(const Py_TLVector* self, PyObject* other_, const int op) {
    if(op != Py_EQ && op != Py_NE) {
        return Py_NotImplemented;
    }

    if(!PyObject_TypeCheck(other_, tlvector_type)) {
        return Py_False;
    }

    const Py_TLVector* other = (Py_TLVector*)other_;
    const bool this_ro = self->readonly;
    const bool other_ro = other->readonly;

    bool eq;
    if (this_ro != other_ro)
        eq = false;
    else
        eq = this_ro
                 ? utl_RoVector_equals(self->ro_vector, other->ro_vector)
                 : utl_Vector_equals(self->vector, other->vector);

    if(op == Py_NE)
        eq = !eq;

    if(eq)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* Py_TLVector_append(Py_TLVector* self, PyObject* args) {
    if(self->readonly) {
        PyErr_SetString(PyExc_AttributeError, "Vector is read-only");
        return NULL;
    }

    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    if(!Py_TLVector_item_set(self->vector, obj, -1)) {
        return NULL;
    }

    if(obj != Py_True && obj != Py_False) {
        const ssize_t size = utl_Vector_size(self->vector);
        const ssize_t refs_capacity = (size / 128 + 2) * 128;
        const ssize_t refs_capacity_old = ((size - 1) / 128 + 2) * 128;
        const ssize_t index = size - 1;

        if(refs_capacity_old != refs_capacity) {
            self->out_refs = realloc(self->out_refs, refs_capacity * sizeof(void*));
            for(ssize_t i = index + 1; i < refs_capacity; ++i) {
                self->out_refs[i] = NULL;
            }
        }

        self->out_refs[index] = obj;

        Py_INCREF(obj);
    }

    Py_RETURN_NONE;
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
