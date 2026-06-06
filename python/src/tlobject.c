#include "pyutl.h"
#include "tlobject.h"
#include "tlvector.h"
#include "encoder.h"
#include "decoder.h"
#include "constants.h"
#include "py_def_pool.h"


static PyObject* Py_TLObject_getitem_regular(Py_TLObject* self, const utl_FieldDef* field) {
    switch (field->type) {
        case FLAGS:
        case INT32: {
            const int32_t value = utl_Message_getInt32(self->message, field);
            _UTL_LOG("Get int32 field %d (%.*s) of object %p: %d", (int)field->num, (int)field->name.size, field->name.data, self, value);
            return PyLong_FromLong(value);
        }
        case INT64: {
            const int64_t value = utl_Message_getInt64(self->message, field);
            _UTL_LOG("Get int64 field %d (%.*s) of object %p: %ld", (int)field->num, (int)field->name.size, field->name.data, self, value);
            return PyLong_FromLong(value);
        }
        case INT128: {
            const utl_Int128 bytes = utl_Message_getInt128(self->message, field);
            _UTL_LOG("Get int128 field %d (%.*s) of object %p: %08lx%08lx", (int)field->num, (int)field->name.size, field->name.data, self,
                ((uint64_t*)bytes.value)[1], ((uint64_t*)bytes.value)[0]);
            return _PyLong_FromByteArray(bytes.value, 16, true, true);
        }
        case INT256: {
            const utl_Int256 bytes = utl_Message_getInt256(self->message, field);
            _UTL_LOG("Get int256 field %d (%.*s) of object %p: %08lx%08lx%08lx%08lx", (int)field->num, (int)field->name.size, field->name.data, self,
                ((uint64_t*)bytes.value)[3], ((uint64_t*)bytes.value)[2], ((uint64_t*)bytes.value)[1], ((uint64_t*)bytes.value)[0]);
            return _PyLong_FromByteArray(bytes.value, 32, true, true);
        }
        case DOUBLE: {
            const double value = utl_Message_getDouble(self->message, field);
            _UTL_LOG("Get double field %d (%.*s) of object %p: %f", (int)field->num, (int)field->name.size, field->name.data, self, value);
            return PyFloat_FromDouble(utl_Message_getDouble(self->message, field));
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            const bool value = utl_Message_getBool(self->message, field);
            _UTL_LOG("Get bool field %d (%.*s) of object %p: %d", (int)field->num, (int)field->name.size, field->name.data, self, value);
            if(value)
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        }
        case BYTES: {
            const utl_StringView bytes = utl_Message_getBytes(self->message, field);
            _UTL_LOG("Get bytes field %d (%.*s) of object %p: <bytes of size %zu>", (int)field->num, (int)field->name.size, field->name.data, self, bytes.size);
            return PyBytes_FromStringAndSize(bytes.data, (ssize_t)bytes.size);
        }
        case STRING: {
            const utl_StringView bytes = utl_Message_getString(self->message, field);
            _UTL_LOG("Get string field %d (%.*s) of object %p: \"%.*s\"", (int)field->num, (int)field->name.size, field->name.data, self, (int)bytes.size, bytes.data);
            return PyUnicode_FromStringAndSize(bytes.data, (ssize_t)bytes.size);
        }
        case TLOBJECT: {
            utl_Message* message = utl_Message_getMessage(self->message, field);
            if(message->userdata != NULL) {
                Py_INCREF(message->userdata);
                return message->userdata;
            }

            const utl_MessageDef* message_def = message->message_def;

            pyutl_MessageDef* cached_def = Py_DefPool_get_or_create_cached_def(message_def);
            if(!cached_def)
                return NULL;

            PyObject* result_obj = cached_def->python_cls->tp_alloc(cached_def->python_cls, 0);
            Py_TLObject_init_message((Py_TLObject*)result_obj, NULL, message);

            _UTL_LOG("Get object field %d (%.*s) of object %p: %p", (int)field->num, (int)field->name.size, field->name.data, self, message);

            return result_obj;
        }
        case VECTOR: {
            utl_Vector* vector = utl_Message_getVector(self->message, field);
            if(vector->userdata != NULL) {
                Py_INCREF(vector->userdata);
                return vector->userdata;
            }

            PyObject* result_obj = tlvector_type->tp_alloc(tlvector_type, 0);
            Py_TLVector_init_message((Py_TLVector*)result_obj, vector);

            _UTL_LOG("Get vector field %d (%.*s) of object %p: %p", (int)field->num, (int)field->name.size, field->name.data, self, vector);

            return result_obj;
        }

        case STATIC_FIELDS_END: return NULL;
    }

    return NULL;
}

static PyObject* Py_TLObject_getitem_readonly(Py_TLObject* self, const utl_FieldDef* field) {
    switch (field->type) {
        case FLAGS:
        case INT32: {
            const int32_t value = utl_RoMessage_getInt32(self->ro_message, field);
            _UTL_LOG("Get int32 field %d (%.*s) of object %p: %d", (int)field->num, (int)field->name.size, field->name.data, self, value);
            return PyLong_FromLong(value);
        }
        case INT64: {
            const int64_t value = utl_RoMessage_getInt64(self->ro_message, field);
            _UTL_LOG("Get int64 field %d (%.*s) of object %p: %ld", (int)field->num, (int)field->name.size, field->name.data, self, value);
            return PyLong_FromLong(value);
        }
        case INT128: {
            const utl_Int128 bytes = utl_RoMessage_getInt128(self->ro_message, field);
            _UTL_LOG("Get int128 field %d (%.*s) of object %p: %08lx%08lx", (int)field->num, (int)field->name.size, field->name.data, self,
                ((uint64_t*)bytes.value)[1], ((uint64_t*)bytes.value)[0]);
            return _PyLong_FromByteArray(bytes.value, 16, true, true);
        }
        case INT256: {
            const utl_Int256 bytes = utl_RoMessage_getInt256(self->ro_message, field);
            _UTL_LOG("Get int256 field %d (%.*s) of object %p: %08lx%08lx%08lx%08lx", (int)field->num, (int)field->name.size, field->name.data, self,
                ((uint64_t*)bytes.value)[3], ((uint64_t*)bytes.value)[2], ((uint64_t*)bytes.value)[1], ((uint64_t*)bytes.value)[0]);
            return _PyLong_FromByteArray(bytes.value, 32, true, true);
        }
        case DOUBLE: {
            const double value = utl_RoMessage_getDouble(self->ro_message, field);
            _UTL_LOG("Get double field %d (%.*s) of object %p: %f", (int)field->num, (int)field->name.size, field->name.data, self, value);
            return PyFloat_FromDouble(value);
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            const bool res = utl_RoMessage_getBool(self->ro_message, field);
            _UTL_LOG("Get bool field %d (%.*s) of object %p: %d", (int)field->num, (int)field->name.size, field->name.data, self, res);
            if(res)
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        }
        case BYTES: {
            const utl_StringView bytes = utl_RoMessage_getBytes(self->ro_message, field);
            _UTL_LOG("Get bytes field %d (%.*s) of object %p: <bytes of size %zu>", (int)field->num, (int)field->name.size, field->name.data, self, bytes.size);
            return PyBytes_FromStringAndSize(bytes.data, (ssize_t)bytes.size);
        }
        case STRING: {
            const utl_StringView bytes = utl_RoMessage_getString(self->ro_message, field);
            _UTL_LOG("Get string field %d (%.*s) of object %p: \"%.*s\"", (int)field->num, (int)field->name.size, field->name.data, self, (int)bytes.size, bytes.data);
            return PyUnicode_FromStringAndSize(bytes.data, (ssize_t)bytes.size);
        }
        case TLOBJECT: {
            utl_RoMessage* message = utl_RoMessage_getMessage(self->ro_message, field);
            if(message->userdata != NULL) {
                Py_INCREF(message->userdata);
                return message->userdata;
            }

            const utl_MessageDef* message_def = message->message_def;

            pyutl_MessageDef* cached_def = Py_DefPool_get_or_create_cached_def(message_def);
            if(!cached_def)
                return NULL;

            PyObject* result_obj = cached_def->python_cls->tp_alloc(cached_def->python_cls, 0);
            Py_TLObject_init_message_ro((Py_TLObject*)result_obj, message);

            PyObject* bytes = self->out_refs[self->ro_message->message_def->fields_num];
            ((Py_TLObject*)result_obj)->out_refs[message_def->fields_num] = bytes;
            Py_INCREF(bytes);

            _UTL_LOG("Get object field %d (%.*s) of object %p: %p", (int)field->num, (int)field->name.size, field->name.data, self, message);

            return result_obj;
        }
        case VECTOR: {
            utl_RoVector* vector = utl_RoMessage_getVector(self->ro_message, field);
            if(vector->userdata != NULL) {
                Py_INCREF(vector->userdata);
                return vector->userdata;
            }

            PyObject* result_obj = tlvector_type->tp_alloc(tlvector_type, 0);
            Py_TLVector_init_message_ro((Py_TLVector*)result_obj, vector);

            PyObject* bytes = self->out_refs[self->ro_message->message_def->fields_num];
            ((Py_TLObject*)result_obj)->out_refs[vector->elements_count] = bytes;
            Py_INCREF(bytes);

            _UTL_LOG("Get vector field %d (%.*s) of object %p: %p", (int)field->num, (int)field->name.size, field->name.data, self, vector);

            return result_obj;
        }

        case STATIC_FIELDS_END: return NULL;
    }

    return NULL;
}

static PyObject* Py_TLObject_getitem(Py_TLObject* self, const utl_FieldDef* field) {
    if(self->out_refs[field->num] != NULL) {
        PyObject* obj = self->out_refs[field->num];
        _UTL_LOG("Found field %d (%.*s) in refs cache: %p", (int)field->num, (int)field->name.size, field->name.data, obj);
        Py_INCREF(obj);
        return obj;
    }

    PyObject* result_obj;
    if(self->readonly) {
        result_obj = Py_TLObject_getitem_readonly(self, field);
    } else {
        result_obj = Py_TLObject_getitem_regular(self, field);
    }

    if(result_obj != NULL) {
        Py_INCREF(result_obj);
        _UTL_LOG("Insert field %d (%.*s) in refs cache: %p", (int)field->num, (int)field->name.size, field->name.data, result_obj);
        self->out_refs[field->num] = result_obj;
        return result_obj;
    }

    Py_RETURN_NONE;
}

static bool Py_TLObject_setitem(Py_TLObject* self, const utl_FieldDef* field, PyObject* item) {
    switch (field->type) {
        case FLAGS:
        case INT32: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            const int64_t num = PyLong_AsLong(item);
            if(num > INT32_MAX || num < INT32_MIN) {
                PyErr_SetString(PyExc_TypeError, "number size exceeds 32 bits");
                return false;
            }
            utl_Message_setInt32(self->message, field, (int32_t)num);
            _UTL_LOG("Set field %d (%.*s) of object %p to int32 %d", (int)field->num, (int)field->name.size, field->name.data, self, (int)PyLong_AsLong(item));
            break;
        }
        case INT64: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            utl_Message_setInt64(self->message, field, PyLong_AsLong(item));
            _UTL_LOG("Set field %d (%.*s) of object %p to int64 %ld", (int)field->num, (int)field->name.size, field->name.data, self, (long)PyLong_AsLong(item));
            break;
        }
        case INT128: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            const utl_Int128 bytes = {{0}};
#if PY_MINOR_VERSION < 13
            _PyLong_AsByteArray((PyLongObject*)item, (uint8_t*)bytes.value, 16, true, true);
#else
            _PyLong_AsByteArray((PyLongObject*)item, (uint8_t*)bytes.value, 16, true, true, true);
#endif
            utl_Message_setInt128(self->message, field, bytes);
            _UTL_LOG("Set field %d (%.*s) of object %p to int128 %08lx%08lx", (int)field->num, (int)field->name.size, field->name.data, self,
                ((uint64_t*)bytes.value)[1], ((uint64_t*)bytes.value)[0]);
            break;
        }
        case INT256: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            const utl_Int256 bytes = {{0}};
#if PY_MINOR_VERSION < 13
            _PyLong_AsByteArray((PyLongObject*)item, (uint8_t*)bytes.value, 32, true, true);
#else
            _PyLong_AsByteArray((PyLongObject*)item, (uint8_t*)bytes.value, 32, true, true, true);
#endif
            utl_Message_setInt256(self->message, field, bytes);
            _UTL_LOG("Set field %d (%.*s) of object %p to int256 %08lx%08lx%08lx%08lx", (int)field->num, (int)field->name.size, field->name.data, self,
                ((uint64_t*)bytes.value)[3], ((uint64_t*)bytes.value)[2], ((uint64_t*)bytes.value)[1], ((uint64_t*)bytes.value)[0]);
            break;
        }
        case DOUBLE: {
            if(!PyFloat_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"float\"");
                return false;
            }
            utl_Message_setDouble(self->message, field, PyFloat_AsDouble(item));
            _UTL_LOG("Set field %d (%.*s) of object %p to double %f", (int)field->num, (int)field->name.size, field->name.data, self, PyFloat_AsDouble(item));
            break;
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            if(!PyBool_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"bool\"");
                return false;
            }
            utl_Message_setBool(self->message, field, item == Py_True);
            _UTL_LOG("Set field %d (%.*s) of object %p to bool %d", (int)field->num, (int)field->name.size, field->name.data, self, item == Py_True);
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
            if(len > UTL_MAX_STRINT_LENGTH) {
                PyErr_SetString(PyExc_ValueError, "bytes object is too big");
                return false;
            }
            const utl_StringView bytes = {
                .size = len,
                .data = buf,
            };
            utl_Message_setBytes(self->message, field, bytes);
            _UTL_LOG("Set field %d (%.*s) of object %p to bytes of len %zu", (int)field->num, (int)field->name.size, field->name.data, self, len);
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
            if(len > UTL_MAX_STRINT_LENGTH) {
                PyErr_SetString(PyExc_ValueError, "string is too big");
                return false;
            }
            const utl_StringView bytes = {
                .size = len,
                .data = (char*)buf,
            };
            utl_Message_setString(self->message, field, bytes);
            _UTL_LOG("Set field %d (%.*s) of object %p to string \"%.*s\"", (int)field->num, (int)field->name.size, field->name.data, self, (int)len, buf);
            break;
        }
        case TLOBJECT: {
            if(!PyObject_TypeCheck(item, tlobject_type)) {
                // TODO: use PyType_GetName(Py_TYPE(item))
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\"");
                return false;
            }
            utl_Message* message = ((Py_TLObject*)item)->message;
            if(field->sub.type_def != NULL && field->sub.type_def != message->message_def->type) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"TLObject\" (TODO: show exact type)");
                return false;
            }
            if(((Py_TLObject*)item)->readonly) {
                PyErr_SetString(PyExc_TypeError, "setting read-only object in regular object is not allowed");
                return false;
            }

            utl_Message* old = utl_Message_swapMessage(self->message, field, message);
            // TODO: do a decref if userdata is not null?
            if(old != NULL && old->userdata == NULL) {
                utl_Message_free(old);
            }

            _UTL_LOG("Set field %d (%.*s) of object %p to object %p", (int)field->num, (int)field->name.size, field->name.data, self, message);
            break;
        }
        case VECTOR: {
            if(!PyList_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"list\"");
                return false;
            }

            const ssize_t len_ssize = PyList_Size(item);
            if(len_ssize >= INT32_MAX) {
                PyErr_SetString(PyExc_ValueError, "number of vector elements must fit in int32 number");
                return false;
            }

            utl_MessageDefVector* def = field->sub.vector_def;

            const int32_t len = (int32_t)len_ssize;
            utl_Vector* vector = utl_Vector_new(def, len);
            for(ssize_t i = 0; i < len; i++) {
                if(!Py_TLVector_item_set(vector, PyList_GetItem(item, i), -1)) {
                    utl_Vector_free(vector);
                    return false;
                }
            }

            PyObject* result_obj = tlvector_type->tp_alloc(tlvector_type, 0);
            Py_TLVector* result_vec = (Py_TLVector*)result_obj;
            Py_TLVector_init_message(result_vec, vector);

            if(def->type == TLOBJECT) {
                for(int32_t i = 0; i < len; i++) {
                    const utl_Message* vec_item = utl_Vector_getMessage(vector, i);
                    if(vec_item != NULL) {
                        Py_XINCREF(vec_item->userdata);
                        result_vec->out_refs[i] = vec_item->userdata;
                    }
                }
            } else if(def->type == VECTOR) {
                for(int32_t i = 0; i < len; i++) {
                    const utl_Vector* vec_item = utl_Vector_getVector(vector, i);
                    if(vec_item != NULL) {
                        Py_XINCREF(vec_item->userdata);
                        result_vec->out_refs[i] = vec_item->userdata;
                    }
                }
            }

            utl_Vector* old = utl_Message_swapVector(self->message, field, vector);
            // TODO: do a decref if userdata is not null?
            if(old != NULL && old->userdata == NULL) {
                utl_Vector_free(old);
            }

            _UTL_LOG("Set field %d (%.*s) of object %p to vector %p", (int)field->num, (int)field->name.size, field->name.data, self, vector);

            item = result_obj;
            break;
        }

        case STATIC_FIELDS_END: return NULL;
    }

    if(item == Py_True || item == Py_False)
        return true;

    PyObject* old_ref = self->out_refs[field->num];

    if(item == Py_None) {
        self->out_refs[field->num] = NULL;
    } else {
        self->out_refs[field->num] = item;
        if(field->type != VECTOR)
            // Vectors are only accepted as lists.
            // New pyutl.Vector object is always created when list is assigned to vector field.
            // So doing incref here is leaking memory.
            Py_INCREF(item);
    }

    Py_XDECREF(old_ref);

    return true;
}

void pyutl_internal_tlobject_free_recursive(utl_MessageHeader* obj, const bool is_readonly) {
    if(obj == NULL || obj->userdata != NULL)
        return;

    const utl_MessageDef* def = obj->message_def;
    const uint16_t obj_fields_num = def->objects_num;
    const uint16_t vec_fields_num = def->vectors_num;
    utl_FieldDef** obj_fields = def->object_fields;
    utl_FieldDef** vec_fields = def->vector_fields;

    if(is_readonly) {
        utl_RoMessage* tlobj = (utl_RoMessage*)obj;
        for(uint16_t i = 0; i < obj_fields_num; ++i)
            pyutl_internal_tlobject_free_recursive((utl_MessageHeader*)utl_RoMessage_getMessage(tlobj, obj_fields[i]), true);
        for(uint16_t i = 0; i < vec_fields_num; ++i)
            pyutl_internal_tlvector_free_recursive((utl_VectorHeader*)utl_RoMessage_getVector(tlobj, vec_fields[i]), true);
        utl_RoMessage_free(tlobj);
    } else {
        utl_Message* tlobj = (utl_Message*)obj;
        for(uint16_t i = 0; i < obj_fields_num; ++i)
            pyutl_internal_tlobject_free_recursive((utl_MessageHeader*)utl_Message_getMessage(tlobj, obj_fields[i]), false);
        for(uint16_t i = 0; i < vec_fields_num; ++i)
            pyutl_internal_tlvector_free_recursive((utl_VectorHeader*)utl_Message_getVector(tlobj, vec_fields[i]), false);
        utl_Message_free(tlobj);
    }
}

static void Py_TLObject_dealloc(Py_TLObject* self) {
    if(self->message == NULL) {
        ((PyObject*)self)->ob_type->tp_free(self);
        return;
    }

    const utl_MessageDef def = *self->message_hdr->message_def;
    const size_t fields_count = def.fields_num;
    const bool is_readonly = self->readonly;

    for (size_t i = 0; i < fields_count + is_readonly; ++i) {
        Py_XDECREF(self->out_refs[i]);

        if(self->out_refs[i] == NULL) {
            utl_FieldDef field = def.fields[i];
            if(field.type == TLOBJECT) {
                utl_MessageHeader* hdr = is_readonly
                                             ? (utl_MessageHeader*)utl_RoMessage_getMessage(self->ro_message, &field)
                                             : (utl_MessageHeader*)utl_Message_getMessage(self->message, &field);
                pyutl_internal_tlobject_free_recursive(hdr, is_readonly);
            } else if(field.type == VECTOR) {
                utl_VectorHeader* hdr = is_readonly
                                             ? (utl_VectorHeader*)utl_RoMessage_getVector(self->ro_message, &field)
                                             : (utl_VectorHeader*)utl_Message_getVector(self->message, &field);
                pyutl_internal_tlvector_free_recursive(hdr, is_readonly);
            }
        }
    }

    self->message_hdr->userdata = NULL;

    if(is_readonly) {
        utl_RoMessage_free(self->ro_message);
    } else {
        utl_Message_free(self->message);
    }

    free(self->out_refs);
    ((PyObject*)self)->ob_type->tp_free(self);
}

void Py_TLObject_init_message(Py_TLObject* self, utl_MessageDef* def, utl_Message* message) {
    if(message != NULL)
        self->message = message;
    else
        self->message = utl_Message_new(def);

    self->message->userdata = self;

    self->out_refs = calloc(self->message->message_def->fields_num, sizeof(void*));
}

void Py_TLObject_init_message_ro(Py_TLObject* self, utl_RoMessage* message) {
    self->readonly = true;
    self->ro_message = message;
    self->ro_message->userdata = self;

    self->out_refs = calloc(self->message->message_def->fields_num + 1, sizeof(void*));
}

static PyObject* Py_TLObject_new(PyTypeObject* cls, PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyObject* result = PyObject_GetAttrString((PyObject*)cls, "__message_def__");
    if(!result) {
        PyErr_SetString(PyExc_NotImplementedError, "Object of type \"TLObject\" cannot be instantiated.");
        return NULL;
    }

    PyObject* self = cls->tp_alloc(cls, 0);
    utl_MessageDef* def = PyCapsule_GetPointer(result, NULL);
    Py_TLObject_init_message((Py_TLObject*)self, def, NULL);

    return self;
}

static int Py_TLObject_init(Py_TLObject* self, PyObject* Py_UNUSED(args), PyObject* kwargs) {
    for(size_t i = 0; i < self->message->message_def->fields_num; ++i) {
        utl_FieldDef field = self->message->message_def->fields[i];
        if(field.type == FLAGS) {
            // TODO: raise an error?
            continue;
        }

        PyObject* item = PyDict_GetItemString(kwargs, field.name.data);

        if(!item) {
            if(!field.flag_num) {
                PyErr_SetString(PyExc_TypeError, "missing required keyword-only argument");
                return -1;
            }
            continue;
        }
        if(Py_IsNone(item)) {
            if(!field.flag_num) {
                PyErr_SetString(PyExc_TypeError, "field is not optional");
                return -1;
            }
            utl_Message_clearField(self->message, &field, false);
            continue;
        }

        if(!Py_TLObject_setitem(self, &field, item)) {
            return -1;
        }
    }

    return 0;
}

static PyObject* Py_TLObject_getattro(Py_TLObject* self, PyObject* attr) {
    const utl_MessageDef* def = ((utl_MessageHeader*)self->message)->message_def;

    pyutl_MessageDef* cached = Py_DefPool_get_or_create_cached_def(def);
    if(!cached)
        return NULL;

    ssize_t len;
    char *buf = (char*)PyUnicode_AsUTF8AndSize(attr, &len);
    if(!buf) {
        return NULL;
    }

    const int field_index = binary_search_str(cached->field_names, def->fields_num, buf, len);
    if(field_index < 0) {
        _UTL_LOG("Failed to find field \"%.*s\" wtf", (int)len, buf);
        return PyObject_GenericGetAttr((PyObject*)self, attr);
    }

    const utl_FieldDef* field = def->fields + cached->field_nums[field_index];

    if(!(self->readonly ? utl_RoMessage_hasField(self->ro_message, field) : utl_Message_hasField(self->message, field))) {
        if(field->type == BIT_BOOL)
            Py_RETURN_FALSE;
        Py_RETURN_NONE;
    }

    return Py_TLObject_getitem(self, field);
}

static int Py_TLObject_setattro(Py_TLObject* self, PyObject* attr, PyObject* value) {
    if(self->readonly) {
        PyErr_SetString(PyExc_AttributeError, "Object is read-only");
        return -1;
    }

    const utl_MessageDef* def = self->message->message_def;
    pyutl_MessageDef* cached = Py_DefPool_get_or_create_cached_def(def);
    if(!cached)
        return -1;

    ssize_t len;
    char *buf = (char*)PyUnicode_AsUTF8AndSize(attr, &len);
    if(!buf) {
        return -1;
    }

    const int field_index = binary_search_str(cached->field_names, def->fields_num, buf, len);
    if(field_index < 0) {
        return -1;
    }

    const utl_FieldDef* field = def->fields + cached->field_nums[field_index];

    return Py_TLObject_setitem(self, field, value) ? 0 : -1;
}

static PyObject* Py_TLObject_repr(Py_TLObject* self) {
    const bool readonly = self->readonly;
    const utl_MessageDef* def = ((utl_MessageHeader*)self->message)->message_def;

    const size_t alloc_size = def->name.size + def->fields_num * 16;
    utl_EncodeBuf repr_buf = {
        .data = malloc(alloc_size),
        .pos = 0,
        .size = alloc_size,
    };
    char* tmp;

    if(def->namespace_.size) {
        tmp = utl_EncodeBuf_alloc(&repr_buf, def->namespace_.size);
        memcpy(tmp, def->namespace_.data, def->namespace_.size);
        tmp = utl_EncodeBuf_alloc(&repr_buf, 1);
        *tmp = '.';
    }

    tmp = utl_EncodeBuf_alloc(&repr_buf, def->name.size);
    memcpy(tmp, def->name.data, def->name.size);
    tmp = utl_EncodeBuf_alloc(&repr_buf, 1);
    *tmp = '(';

    for(size_t i = 0; i < def->fields_num; i++) {
        utl_FieldDef field = def->fields[i];
        tmp = utl_EncodeBuf_alloc(&repr_buf, field.name.size);
        memcpy(tmp, field.name.data, field.name.size);
        tmp = utl_EncodeBuf_alloc(&repr_buf, 1);
        *tmp = '=';

        PyObject* value;
        if(readonly ? !utl_RoMessage_hasField(self->ro_message, &field) : !utl_Message_hasField(self->message, &field)) {
            value = Py_None;
        } else {
            value = Py_TLObject_getitem(self, &field);
        }

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

        if(i != def->fields_num - 1) {
            tmp = utl_EncodeBuf_alloc(&repr_buf, 2);
            tmp[0] = ',';
            tmp[1] = ' ';
        }
    }

    tmp = utl_EncodeBuf_alloc(&repr_buf, 1);
    *tmp = ')';

    PyObject* result = PyUnicode_FromStringAndSize(repr_buf.data, repr_buf.pos);
    free(repr_buf.data);
    return result;
}

static PyObject* Py_TLObject_compare(const Py_TLObject* self, PyObject* other_, const int op) {
    if(op != Py_EQ && op != Py_NE) {
        return Py_NotImplemented;
    }

    if(!PyObject_TypeCheck(other_, tlobject_type)) {
        return Py_False;
    }

    const Py_TLObject* other = (Py_TLObject*)other_;
    const bool this_ro = self->readonly;
    const bool other_ro = other->readonly;

    bool eq;
    if (this_ro != other_ro)
        eq = false;
    else
        eq = this_ro
                 ? utl_RoMessage_equals(self->ro_message, other->ro_message)
                 : utl_Message_equals(self->message, other->message);

    if (op == Py_NE)
        eq = !eq;

    if(eq)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* Py_TLObject_read(PyTypeObject* cls, uint8_t* buf, size_t buf_len, size_t* bytes_read, const bool read_only) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();

    PyObject* result = PyObject_GetAttrString((PyObject*)cls, "__message_def__");
    if(!result) {
        PyObject* exc = PyErr_Occurred();
        if(!exc) {
            PyErr_SetString(PyExc_RuntimeError, "Getting __message_def__ from class failed, but exception is not set");
            return NULL;
        }

        if(!PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_SetString(PyExc_RuntimeError, "Getting __message_def__ from class failed, but exception is not a \"AttributeError\"");
            return NULL;
        }

        PyErr_Clear();

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

        pyutl_MessageDef* cached_def = Py_DefPool_get_or_create_cached_def(def);
        if(!cached_def)
            return NULL;

        cls = cached_def->python_cls;
        buf += 4;
        buf_len -= 4;
    }

    if(read_only) {
        PyObject* def_capsule = PyObject_GetAttrString((PyObject*)cls, "__message_def__");
        if(!def_capsule) {
            PyErr_SetString(PyExc_NotImplementedError, "Object of type \"TLObject\" cannot be instantiated.");
            return 0;
        }
        utl_MessageDef* def = PyCapsule_GetPointer(def_capsule, NULL);

        utl_RoMessage* message = utl_RoMessage_new(def, state->c_def_pool, buf, buf_len, bytes_read);
        if(!message) {
            PyErr_SetString(PyExc_TypeError, "Failed to read object (TODO: exact error)"); // TODO
            return NULL;
        }

        Py_TLObject* obj = (Py_TLObject*)cls->tp_alloc(cls, 0);
        Py_TLObject_init_message_ro(obj, message);

        return (PyObject*)obj;
    } else {
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
}

static PyObject* Py_TLObject_read_bytesio(PyTypeObject* cls, PyObject* args, PyObject* kwargs) {
    PyObject* bio;
    int read_only = false;

    static char *kwlist[] = {"buf", "read_only", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|p", kwlist, bytesio_type, &bio, &read_only)) {
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
    PyObject* result = Py_TLObject_read(cls, view->buf, view->len, &read, read_only);
    if(read_only) {
        const Py_TLObject* tl_result = (Py_TLObject*)result;
        tl_result->out_refs[tl_result->ro_message->message_def->fields_num] = memoryview;
    } else {
        Py_XDECREF(memoryview);
    }

    if(result) {
        Py_XDECREF(PyObject_CallMethod(bio, "seek", "ki", read, 1)); // SEEK_CUR
    }

    return result;
}

static PyObject* Py_TLObject_read_bytes(PyTypeObject* cls, PyObject* args, PyObject* kwargs) {
    uint8_t* buf;
    size_t buf_len;
    int read_only = 0;

    static char *kwlist[] = {"buf", "read_only", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y#|p", kwlist, &buf, &buf_len, &read_only)) {
        return NULL;
    }

    PyObject* result = Py_TLObject_read(cls, buf, buf_len, NULL, read_only);
    if(result && read_only) {
        PyObject* bytes;
        if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|p", kwlist, &PyBytes_Type, &bytes, &read_only)) {
            return NULL;
        }

        const Py_TLObject* tl_result = (Py_TLObject*)result;
        tl_result->out_refs[tl_result->ro_message->message_def->fields_num] = bytes;
        Py_INCREF(bytes);
    }

    return result;
}

static PyObject* Py_TLObject_write(const Py_TLObject* self, PyObject* Py_UNUSED(args)) {
    PyObject* result;

    if(self->readonly) {
        result = PyBytes_FromStringAndSize(NULL, self->ro_message->size + 4);
        char* result_buf = PyBytes_AsString(result);
        memcpy(result_buf, &self->ro_message->message_def->id, 4);
        memcpy(result_buf + 4, self->ro_message->data, self->ro_message->size);
    } else {
        size_t written_bytes;
        char* bytes = utl_encode(self->message, &written_bytes);
        result = PyBytes_FromStringAndSize(bytes, (ssize_t)written_bytes);
        free(bytes);
    }

    return result;
}

static PyMethodDef Py_TLObject_methods[] = {
    {"read", (PyCFunction)Py_TLObject_read_bytesio, METH_VARARGS | METH_KEYWORDS | METH_CLASS, 0,},
    {"read_bytes", (PyCFunction)Py_TLObject_read_bytes, METH_VARARGS | METH_KEYWORDS | METH_CLASS, 0,},
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

PyType_Spec pyutl_TLObjectType_spec = {
    "_pyutl.TLObject",
    sizeof(Py_TLObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_IS_ABSTRACT,
    Py_TLObject_slots,
};

PyObject* Py_TLObject_createType(const utl_MessageDef* message_def) {
    const size_t alloc_size = 7 + (message_def->namespace_.size ? message_def->namespace_.size + 1 : 0) + message_def->name.size;
    char* name = malloc(alloc_size + 1);
    name[alloc_size] = '\0';
    memcpy(name, "_pyutl.", 7);
    if(message_def->namespace_.size) {
        memcpy(name + 7, message_def->namespace_.data, message_def->namespace_.size);
        name[7 + message_def->namespace_.size] = '.';
    }
    memcpy(name + 7 + message_def->namespace_.size + 1, message_def->name.data, message_def->name.size);

    PyType_Slot slots[] = {
        {Py_tp_base, tlobject_type},
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
    // TODO: i have no idea what to do with this memory, if i free it - it breaks type name in python, if dont - then it is a memory leak
    // free(name);
    if(!new_type) {
        free(name);
        return 0;
    }

    PyObject* msgdef_capsule = PyCapsule_New((void*)message_def, NULL, NULL);
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