#include "pyutl.h"
#include "tlvector.h"
#include "builtins.h"

#include <encoder.h>

#include "tlobject.h"
#include "constants.h"

static bool bitmap_bit_get(const uint64_t* bitmap, const size_t bitmap_size, const size_t bit_num) {
    const size_t byte_num = bit_num / 64;
    return (byte_num >= bitmap_size) ? 0 : bitmap[byte_num] & ((uint64_t)1 << (bit_num % 64));
}

static void bitmap_bit_set(uint64_t* bitmap, const size_t bitmap_size, const size_t bit_num) {
    const size_t byte_num = bit_num / 64;
    if(byte_num < bitmap_size)
        bitmap[byte_num] |= (uint64_t)1 << (bit_num % 64);
}

static void bitmap_bit_clr(uint64_t* bitmap, const size_t bitmap_size, const size_t bit_num) {
    const size_t byte_num = bit_num / 64;
    if(byte_num < bitmap_size)
        bitmap[byte_num] &= ~((uint64_t)1 << (bit_num % 64));
}

#define tlvector_is_readonly(OBJECT_PTR) (((OBJECT_PTR)->refs_bitmap[self->refs_bitmap_bytes - 1]) & ((uint64_t)1 << 63))

static PyObject* Py_TLVector_getitem(const Py_TLVector* self, const size_t index) {
    if(self->out_refs[index] != NULL && bitmap_bit_get(self->refs_bitmap, self->refs_bitmap_bytes, index)) {
        PyObject* obj = self->out_refs[index];
        Py_INCREF(obj);
        return obj;
    }

    const bool vector_is_read_only = tlvector_is_readonly(self);
    PyObject* result_obj = NULL;

    switch (self->vector->message_def->type) {
        case FLAGS:
        case INT32: {
            result_obj = PyLong_FromLong(
                vector_is_read_only
                    ? utl_RoVector_getInt32(self->ro_vector, index)
                    : utl_Vector_getInt32(self->vector, index)
            );
            break;
        }
        case INT64: {
            result_obj = PyLong_FromLong(
                vector_is_read_only
                    ? utl_RoVector_getInt64(self->ro_vector, index)
                    : utl_Vector_getInt64(self->vector, index)
            );
            break;
        }
        case INT128: {
            const utl_Int128 value = vector_is_read_only
                    ? utl_RoVector_getInt128(self->ro_vector, index)
                    : utl_Vector_getInt128(self->vector, index);
            result_obj = _PyLong_FromByteArray(value.value, 16, true, true);
            break;
        }
        case INT256: {
            const utl_Int256 value = vector_is_read_only
                    ? utl_RoVector_getInt256(self->ro_vector, index)
                    : utl_Vector_getInt256(self->vector, index);
            result_obj = _PyLong_FromByteArray(value.value, 32, true, true);
            break;
        }
        case DOUBLE: {
            result_obj = PyFloat_FromDouble(
                vector_is_read_only
                    ? utl_RoVector_getDouble(self->ro_vector, index)
                    : utl_Vector_getDouble(self->vector, index)
            );
            break;
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            const bool res = vector_is_read_only
                                ? utl_RoVector_getBool(self->ro_vector, index)
                                : utl_Vector_getBool(self->vector, index);
            if(res)
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        }
        case BYTES: {
            const utl_StringView bytes = vector_is_read_only
                    ? utl_RoVector_getBytes(self->ro_vector, index)
                    : utl_Vector_getBytes(self->vector, index);
            result_obj = PyBytes_FromStringAndSize(bytes.data, bytes.size);
            break;
        }
        case STRING: {
            const utl_StringView bytes = vector_is_read_only
                    ? utl_RoVector_getString(self->ro_vector, index)
                    : utl_Vector_getString(self->vector, index);
            result_obj = PyUnicode_FromStringAndSize(bytes.data, bytes.size);
            break;
        }
        case TLOBJECT: {
            const pyutl_ModuleState* state = pyutl_ModuleState_get();
            void* message = vector_is_read_only
                                ? (void*)utl_RoVector_getMessage(self->ro_vector, index)
                                : (void*)utl_Vector_getMessage(self->vector, index);
            utl_MessageDef* message_def = vector_is_read_only
                                              ? ((utl_RoMessage*)message)->message_def
                                              : ((utl_Message*)message)->message_def;

            pyutl_MessageDef* cached_def = utl_Map_search_uint64(state->messages_cache, (uint64_t)message_def);
            if(!cached_def) {
                PyErr_SetString(PyExc_TypeError, "object type is not found");
                return NULL;
            }

            result_obj = cached_def->python_cls->tp_alloc(cached_def->python_cls, 0);
            if(vector_is_read_only) {
                Py_TLObject_init_message_ro((Py_TLObject*)result_obj, message);
                PyObject* bytes = self->out_refs[self->ro_vector->elements_count];
                ((Py_TLObject*)result_obj)->out_refs[message_def->fields_num] = bytes;
                Py_INCREF(bytes);
            } else {
                Py_TLObject_init_message((Py_TLObject*)result_obj, NULL, message);
            }

            break;
        }
        case VECTOR: {
            void* vector = vector_is_read_only
                                ? (void*)utl_RoVector_getVector(self->ro_vector, index)
                                : (void*)utl_Vector_getVector(self->vector, index);

            result_obj = tlvector_type->tp_alloc(tlvector_type, 0);
            if(vector_is_read_only) {
                Py_TLVector_init_message_ro((Py_TLVector*)result_obj, vector);
                PyObject* bytes = self->out_refs[self->ro_vector->elements_count];
                ((Py_TLObject*)result_obj)->out_refs[((utl_RoVector*)vector)->elements_count] = bytes;
                Py_INCREF(bytes);
            } else {
                Py_TLVector_init_message((Py_TLVector*)result_obj, vector);
            }

            break;
        }

        case STATIC_FIELDS_END: return NULL;
    }

    if(result_obj != NULL) {
        Py_INCREF(result_obj);
        self->out_refs[index] = result_obj;
        bitmap_bit_set(self->refs_bitmap, self->refs_bitmap_bytes, index);
        return result_obj;
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
            if(len > UTL_MAX_STRINT_LENGTH) {
                PyErr_SetString(PyExc_ValueError, "bytes object is too big");
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
            if(len > UTL_MAX_STRINT_LENGTH) {
                PyErr_SetString(PyExc_ValueError, "string is too big");
                return false;
            }
            const utl_StringView bytes = {.size = len, .data = (char*)buf};
            index >= 0 ? utl_Vector_setString(vector, index, bytes) : utl_Vector_appendString(vector, bytes);
            return true;
        }
        case TLOBJECT: {
            if(!PyObject_TypeCheck(item, tlobject_type)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\"");
                return false;
            }
            utl_Message* message = ((Py_TLObject*)item)->message;
            if(vector->message_def->sub.type_def != NULL && vector->message_def->sub.type_def != message->message_def->type) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\" (TODO: show exact type)");
                return false;
            }
            // TODO: check if message is read-only

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
                if(!Py_TLVector_item_set(new_vector, PyList_GetItem(item, i), -1)) {
                    utl_Vector_free(new_vector);
                    return false;
                }
            }

            index >= 0 ? utl_Vector_setVector(vector, index, new_vector) : utl_Vector_appendVector(vector, new_vector);

            return true;
        }

        case STATIC_FIELDS_END: return false;
    }

    return false;
}

static void Py_TLVector_dealloc(Py_TLVector* self) {
    const bool vector_is_read_only = tlvector_is_readonly(self);
    const utl_MessageDefVector* def = vector_is_read_only ? self->ro_vector->message_def : self->vector->message_def;
    const size_t fields_count = vector_is_read_only ? self->ro_vector->elements_count : self->vector->size;

    if(vector_is_read_only) {
        for (size_t i = 0; i < fields_count + 1; ++i) {
            if(self->out_refs[i] != NULL)
                Py_DECREF(self->out_refs[i]);
        }
        utl_RoVector_free(self->ro_vector);
    } else {
        for (size_t i = 0; i < fields_count; ++i) {
            if(self->out_refs[i] == NULL)
                continue;
            if(bitmap_bit_get(self->refs_bitmap, self->refs_bitmap_bytes, i))
                Py_DECREF(self->out_refs[i]);
            else if(def->type == TLOBJECT)
                utl_Message_free(self->out_refs[i]);
            else if(def->type == VECTOR)
                utl_Vector_free(self->out_refs[i]);
        }
        utl_Vector_free(self->vector);
    }

    free(self->out_refs);
    free(self->refs_bitmap);
    ((PyObject*)self)->ob_type->tp_free(self);
}

void Py_TLVector_init_message(Py_TLVector* self, utl_Vector* vector) {
    self->vector = vector;

    const size_t vec_size = utl_Vector_size(vector);
    const size_t out_refs_bytes = sizeof(void*) * vec_size;
    self->out_refs = malloc(out_refs_bytes);
    self->refs_bitmap = malloc((vec_size + 7) / 8);
    self->refs_bitmap_bytes = (vec_size + 7) / 8;
    memset(self->out_refs, 0, out_refs_bytes);
    memset(self->refs_bitmap, 0, sizeof(self->refs_bitmap));
}

void Py_TLVector_init_message_ro(Py_TLVector* self, utl_RoVector* vector) {
    self->ro_vector = vector;

    const size_t vec_size = utl_RoVector_size(vector);
    const size_t out_refs_bytes = sizeof(void*) * (utl_RoVector_size(vector) + 1);
    self->out_refs = malloc(out_refs_bytes);
    self->refs_bitmap = malloc(vec_size / 8 + 1);
    self->refs_bitmap_bytes = vec_size / 8 + 1;
    memset(self->out_refs, 0, out_refs_bytes);
    memset(self->refs_bitmap, 0xff, sizeof(self->refs_bitmap));
}

static PyObject* Py_TLVector_new(PyTypeObject* Py_UNUSED(cls), PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyErr_SetString(PyExc_RuntimeError, "Type TLVector cannot be created directly.");

    return NULL;
}

static PyObject* Py_TLVector_sq_item(const Py_TLVector* self, const ssize_t index) {
    const size_t vector_size = tlvector_is_readonly(self) ? utl_RoVector_size(self->ro_vector) : utl_Vector_size(self->vector);
    if(index >= vector_size) {
        PyErr_SetString(PyExc_IndexError, "list index out of range");
        return NULL;
    }

    return Py_TLVector_getitem(self, index);
}

static void Py_TLVector_remove_at_index(const Py_TLVector* self, const size_t index) {
    PyObject* old_ref = self->out_refs[index];
    const bool old_bit = bitmap_bit_get(self->refs_bitmap, self->refs_bitmap_bytes, index);

    utl_Vector_remove(self->vector, index);

    if(old_bit)
        Py_XDECREF(old_ref);
    else if (self->vector->message_def->type == TLOBJECT)
        utl_Message_free((utl_Message*)old_ref);
    else if(self->vector->message_def->type == VECTOR)
        utl_Vector_free((utl_Vector*)old_ref);

    const size_t new_size = utl_Vector_size(self->vector);
    memcpy(self->out_refs + index, self->out_refs + index + 1, (new_size - index) * sizeof(void*));
    for(size_t bit_num = index + 1; bit_num < new_size; ++bit_num)
        if(bitmap_bit_get(self->refs_bitmap, self->refs_bitmap_bytes, bit_num))
            bitmap_bit_set(self->refs_bitmap, self->refs_bitmap_bytes, bit_num - 1);
        else
            bitmap_bit_clr(self->refs_bitmap, self->refs_bitmap_bytes, bit_num - 1);
}

static int Py_TLVector_sq_setitem(const Py_TLVector* self, const ssize_t index, PyObject* value) {
    if(tlvector_is_readonly(self)) {
        PyErr_SetString(PyExc_AttributeError, "Vector is read-only");
        return -1;
    }

    if(index >= utl_Vector_size(self->vector) || (index < 0 && value == NULL)) {
        PyErr_SetString(PyExc_IndexError, "list index out of range");
        return -1;
    }

    PyObject* old_ref = self->out_refs[index];
    const bool old_bit = bitmap_bit_get(self->refs_bitmap, self->refs_bitmap_bytes, index);

    if(value == NULL) {
        Py_TLVector_remove_at_index(self, index);
        return 0;
    }

    if(!Py_TLVector_item_set(self->vector, value, index)) {
        return -1;
    }

    if(old_ref != NULL) {
        if(old_bit)
            Py_XDECREF(old_ref);
        else if (self->vector->message_def->type == TLOBJECT)
            utl_Message_free((utl_Message*)old_ref);
        else if(self->vector->message_def->type == VECTOR)
            utl_Vector_free((utl_Vector*)old_ref);
    }

    if(value != Py_True && value != Py_False && self->vector->message_def->type != VECTOR) {
        self->out_refs[index] = value;
        bitmap_bit_set(self->refs_bitmap, self->refs_bitmap_bytes, index);
        Py_INCREF(value);
    }

    return 0;
}

static PyObject* Py_TLVector_repr(const Py_TLVector* self) {
    const size_t size = tlvector_is_readonly(self) ? utl_RoVector_size(self->ro_vector) : utl_Vector_size(self->vector);
    const size_t alloc_size = size * 8;

    utl_EncodeBuf repr_buf = {
        .data = malloc(alloc_size),
        .pos = 0,
        .size = alloc_size,
    };

    char* tmp = utl_EncodeBuf_alloc(&repr_buf, 1);
    *tmp = '[';


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

    if(!PyObject_TypeCheck(other_, tlvector_type)) {
        return Py_False;
    }

    const Py_TLVector* other = (Py_TLVector*)other_;
    const bool this_ro = tlvector_is_readonly(self);
    const bool other_ro = tlvector_is_readonly(other);

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
    const bool readonly = tlvector_is_readonly(self);
    if(readonly) {
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

    if(obj != Py_True && obj != Py_False && self->vector->message_def->type != VECTOR) {
        const size_t size = utl_Vector_size(self->vector);
        const size_t index = size - 1;
        const size_t refs_capacity = (size / 128 + 2) * 128;

        self->out_refs = realloc(self->out_refs, refs_capacity * sizeof(void*));
        self->refs_bitmap = realloc(self->refs_bitmap, refs_capacity / 8);
        self->refs_bitmap_bytes = refs_capacity / 8;

        self->out_refs[index] = obj;
        bitmap_bit_set(self->refs_bitmap, self->refs_bitmap_bytes, index);

        // TODO: only set refs to zeros if capacity is changed
        for(size_t i = index + 1; i < refs_capacity; ++i) {
            self->out_refs[i] = NULL;
            bitmap_bit_clr(self->refs_bitmap, self->refs_bitmap_bytes, i);
        }

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
