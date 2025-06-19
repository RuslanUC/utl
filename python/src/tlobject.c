#include "pyutl.h"
#include "tlobject.h"
#include "tlvector.h"
#include "encoder.h"
#include "decoder.h"
#include "constants.h"

static bool bitmap_bit_get(const uint64_t bitmap[4], const size_t bit_num) {
    const size_t byte_num = bit_num / 64;
    return (byte_num >= 4) ? 0 : bitmap[byte_num] & ((uint64_t)1 << (bit_num % 64));
}

static void bitmap_bit_set(uint64_t bitmap[4], const size_t bit_num) {
    const size_t byte_num = bit_num / 64;
    if(byte_num < 4)
        bitmap[byte_num] |= (uint64_t)1 << (bit_num % 64);
}

static void bitmap_bit_clr(uint64_t bitmap[4], const size_t bit_num) {
    const size_t byte_num = bit_num / 64;
    if(byte_num < 4)
        bitmap[byte_num] &= ~((uint64_t)1 << (bit_num % 64));
}

#define tlobject_is_readonly(OBJECT_PTR) (((OBJECT_PTR)->refs_bitmap[(sizeof((OBJECT_PTR)->refs_bitmap) / sizeof((OBJECT_PTR)->refs_bitmap[0])) - 1]) & ((uint64_t)1 << 63))

static PyObject* Py_TLObject_getitem(Py_TLObject* self, const utl_FieldDef* field) {
    if(self->out_refs[field->num] != NULL && bitmap_bit_get(self->refs_bitmap, field->num)) {
        PyObject* obj = self->out_refs[field->num];
        Py_INCREF(obj);
        return obj;
    }

    const bool object_is_read_only = tlobject_is_readonly(self);

    PyObject* result_obj = NULL;

    switch (field->type) {
        case FLAGS:
        case INT32: {
            result_obj = PyLong_FromLong(
                object_is_read_only
                    ? utl_RoMessage_getInt32(self->ro_message, field)
                    : utl_Message_getInt32(self->message, field)
            );
            break;
        }
        case INT64: {
            result_obj = PyLong_FromLong(
                object_is_read_only
                    ? utl_RoMessage_getInt64(self->ro_message, field)
                    : utl_Message_getInt64(self->message, field)
            );
            break;
        }
        case INT128: {
            const utl_Int128 bytes = object_is_read_only
                    ? utl_RoMessage_getInt128(self->ro_message, field)
                    : utl_Message_getInt128(self->message, field);
            result_obj = _PyLong_FromByteArray(bytes.value, 16, true, true);
            break;
        }
        case INT256: {
            const utl_Int256 bytes = object_is_read_only
                    ? utl_RoMessage_getInt256(self->ro_message, field)
                    : utl_Message_getInt256(self->message, field);
            result_obj = _PyLong_FromByteArray(bytes.value, 32, true, true);
            break;
        }
        case DOUBLE: {
            result_obj = PyFloat_FromDouble(
                object_is_read_only
                    ? utl_RoMessage_getDouble(self->ro_message, field)
                    : utl_Message_getDouble(self->message, field)
            );
            break;
        }
        case FULL_BOOL:
        case BIT_BOOL: {
            const bool res = object_is_read_only
                    ? utl_RoMessage_getBool(self->ro_message, field)
                    : utl_Message_getBool(self->message, field);
            if(res)
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        }
        case BYTES: {
            const utl_StringView bytes = object_is_read_only
                    ? utl_RoMessage_getBytes(self->ro_message, field)
                    : utl_Message_getBytes(self->message, field);
            result_obj = PyBytes_FromStringAndSize(bytes.data, bytes.size);
            break;
        }
        case STRING: {
            const utl_StringView bytes = object_is_read_only
                    ? utl_RoMessage_getString(self->ro_message, field)
                    : utl_Message_getString(self->message, field);
            result_obj = PyUnicode_FromStringAndSize(bytes.data, bytes.size);
            break;
        }
        case TLOBJECT: {
            const pyutl_ModuleState* state = pyutl_ModuleState_get();
            void* message = object_is_read_only
                                ? (void*)utl_RoMessage_getMessage(self->ro_message, field)
                                : (void*)utl_Message_getMessage(self->message, field);
            utl_MessageDef* message_def = object_is_read_only
                                              ? ((utl_RoMessage*)message)->message_def
                                              : ((utl_Message*)message)->message_def;

            pyutl_MessageDef* cached_def = utl_Map_search_uint64(state->messages_cache, (uint64_t)message_def);
            if(!cached_def) {
                PyErr_SetString(PyExc_TypeError, "object type is not found");
                return NULL;
            }

            result_obj = cached_def->python_cls->tp_alloc(cached_def->python_cls, 0);
            if(object_is_read_only) {
                Py_TLObject_init_message_ro((Py_TLObject*)result_obj, message);
                PyObject* bytes = self->out_refs[self->ro_message->message_def->fields_num];
                ((Py_TLObject*)result_obj)->out_refs[message_def->fields_num] = bytes;
                Py_INCREF(bytes);
            } else {
                Py_TLObject_init_message((Py_TLObject*)result_obj, NULL, message);
            }

            break;
        }
        case VECTOR: {
            const pyutl_ModuleState* state = pyutl_ModuleState_get();
            void* vector = object_is_read_only
                                ? (void*)utl_RoMessage_getVector(self->ro_message, field)
                                : (void*)utl_Message_getVector(self->message, field);

            result_obj = state->tlvector_type->tp_alloc(state->tlvector_type, 0);
            if(object_is_read_only) {
                Py_TLVector_init_message_ro((Py_TLVector*)result_obj, vector);
                PyObject* bytes = self->out_refs[self->ro_message->message_def->fields_num];
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
        self->out_refs[field->num] = result_obj;
        bitmap_bit_set(self->refs_bitmap, field->num);
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
            utl_Int128 bytes;
#if PY_MINOR_VERSION < 13
            _PyLong_AsByteArray((PyLongObject*)item, bytes.value, 16, true, true);
#else
            _PyLong_AsByteArray((PyLongObject*)item, bytes.value, 16, true, true, true);
#endif
            utl_Message_setInt128(self->message, field, bytes);
            break;
        }
        case INT256: {
            if(!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"int\"");
                return false;
            }
            utl_Int256 bytes = {{0}};
#if PY_MINOR_VERSION < 13
            _PyLong_AsByteArray((PyLongObject*)item, bytes.value, 32, true, true);
#else
            _PyLong_AsByteArray((PyLongObject*)item, bytes.value, 32, true, true, true);
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
            if(len > UTL_MAX_STRINT_LENGTH) {
                PyErr_SetString(PyExc_ValueError, "bytes object is too big");
                return false;
            }
            const utl_StringView bytes = {
                .size = len,
                .data = buf,
            };
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
            if(len > UTL_MAX_STRINT_LENGTH) {
                PyErr_SetString(PyExc_ValueError, "string is too big");
                return false;
            }
            const utl_StringView bytes = {
                .size = len,
                .data = (char*)buf,
            };
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
            if(tlobject_is_readonly((Py_TLObject*)item)) {
                PyErr_SetString(PyExc_TypeError, "setting read-only object in regular object is not allowed");
                return false;
            }

            utl_Message_setMessage(self->message, field, message);
            break;
        }
        case VECTOR: {
            if(!PyList_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "expected object of type \"list\"");
                return false;
            }

            const size_t len = PyList_Size(item);
            utl_Vector* vector = utl_Vector_new(field->sub.vector_def, len);
            for(size_t i = 0; i < len; i++) {
                if(!Py_TLVector_item_set(vector, PyList_GetItem(item, i), -1)) {
                    utl_Vector_free(vector);
                    return false;
                }
            }

            utl_Vector* old_vector = utl_Message_getVector(self->message, field);
            if(bitmap_bit_get(self->refs_bitmap, field->num))
                Py_XDECREF(self->out_refs[field->num]);
            else if(old_vector != NULL)
                utl_Vector_free(old_vector);

            utl_Message_setVector(self->message, field, vector);
            bitmap_bit_clr(self->refs_bitmap, field->num);
            self->out_refs[field->num] = vector;

            return true;
        }

        case STATIC_FIELDS_END: return NULL;
    }

    if(item == Py_True || item == Py_False) {
        return true;
    } else if(item == Py_None) {
        Py_XDECREF(self->out_refs[field->num]);
        self->out_refs[field->num] = NULL;
    } else {
        Py_XDECREF(self->out_refs[field->num]);
        self->out_refs[field->num] = item;
        bitmap_bit_set(self->refs_bitmap, field->num);
        Py_INCREF(item);
    }

    return true;
}

static void Py_TLObject_dealloc(Py_TLObject* self) {
    const bool object_is_read_only = tlobject_is_readonly(self);
    const utl_MessageDef* def = object_is_read_only ? self->ro_message->message_def : self->message->message_def;
    const size_t fields_count = def->fields_num;

    if(object_is_read_only) {
        for (size_t i = 0; i < fields_count + 1; ++i) {
            if(self->out_refs[i] != NULL)
                Py_DECREF(self->out_refs[i]);
        }
        utl_RoMessage_free(self->ro_message);
    } else {
        for (size_t i = 0; i < fields_count; ++i) {
            if(self->out_refs[i] == NULL)
                continue;
            if(bitmap_bit_get(self->refs_bitmap, i))
                Py_DECREF(self->out_refs[i]);
            else if(def->fields[i].type == TLOBJECT)
                utl_Message_free(self->out_refs[i]);
            else if(def->fields[i].type == VECTOR)
                utl_Vector_free(self->out_refs[i]);
        }
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

    const size_t out_refs_bytes = sizeof(void*) * self->message->message_def->fields_num;
    self->out_refs = malloc(out_refs_bytes);
    memset(self->out_refs, 0, out_refs_bytes);
    memset(self->refs_bitmap, 0, sizeof(self->refs_bitmap));
}

void Py_TLObject_init_message_ro(Py_TLObject* self, utl_RoMessage* message) {
    self->ro_message = message;

    const utl_MessageDef* def = message->message_def;
    const size_t out_refs_bytes = sizeof(void*) * (def->fields_num + 1);
    self->out_refs = malloc(out_refs_bytes);
    memset(self->out_refs, 0, out_refs_bytes);
    memset(self->refs_bitmap, 0xff, sizeof(self->refs_bitmap));
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

static int Py_TLObject_init(Py_TLObject* self, PyObject* Py_UNUSED(args), PyObject* kwargs) {
    for(size_t i = 0; i < self->message->message_def->fields_num; ++i) {
        utl_FieldDef field = self->message->message_def->fields[i];
        if(field.type == FLAGS) {
            continue;
        }

        PyObject* item = PyDict_GetItemString(kwargs, field.name.data);

        if(!item) {
            if(!field.flag_info) {
                PyErr_SetString(PyExc_TypeError, "missing required keyword-only argument");
                return -1;
            }
            continue;
        }
        if(Py_IsNone(item)) {
            if(!field.flag_info) {
                PyErr_SetString(PyExc_TypeError, "field is not optional");
                return -1;
            }
            utl_Message_clearField(self->message, &field);
            continue;
        }

        if(!Py_TLObject_setitem(self, &field, item)) {
            return -1;
        }
    }

    return 0;
}

static PyObject* Py_TLObject_getattro(Py_TLObject* self, PyObject* attr) {
    const bool readonly = tlobject_is_readonly(self);
    const utl_MessageDef* def = readonly ? self->ro_message->message_def : self->message->message_def;

    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    pyutl_MessageDef* cached = utl_Map_search_uint64(state->messages_cache, (uint64_t)def);
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

    if(!(readonly ? utl_RoMessage_hasField(self->ro_message, field) : utl_Message_hasField(self->message, field))) {
        if(field->type == BIT_BOOL)
            Py_RETURN_FALSE;
        Py_RETURN_NONE;
    }

    return Py_TLObject_getitem(self, field);
}

static int Py_TLObject_setattro(Py_TLObject* self, PyObject* attr, PyObject* value) {
    if(tlobject_is_readonly(self)) {
        PyErr_SetString(PyExc_AttributeError, "Object is read-only");
        return -1;
    }

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

static PyObject* Py_TLObject_repr(Py_TLObject* self) {
    const bool readonly = tlobject_is_readonly(self);
    const utl_MessageDef* def = readonly ? self->ro_message->message_def : self->message->message_def;

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

    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    // TODO: replace with `self->ob_base.ob_type`?
    if(!PyObject_TypeCheck(other_, state->tlobject_type)) {
        return Py_False;
    }

    const Py_TLObject* other = (Py_TLObject*)other_;
    const bool this_ro = tlobject_is_readonly(self);
    const bool other_ro = tlobject_is_readonly(other);

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

    if(read_only) {
        PyObject* def_capsule = PyObject_GetAttrString((PyObject*)cls, "__message_def__");
        if(!def_capsule) {
            PyErr_SetString(PyExc_NotImplementedError, "Object of type \"TLObject\" cannot be instantiated.");
            return 0;
        }
        utl_MessageDef* def = PyCapsule_GetPointer(def_capsule, NULL);

        Py_TLObject* obj = (Py_TLObject*)cls->tp_alloc(cls, 0);
        obj->ro_message = utl_RoMessage_new(def, state->c_def_pool, buf, buf_len, bytes_read);
        if(!obj->ro_message) {
            Py_DECREF(obj);
            PyErr_SetString(PyExc_TypeError, "Failed to read object (TODO: exact error)"); // TODO
            return NULL;
        }

        const size_t out_refs_bytes = sizeof(void*) * (def->fields_num + 1);
        obj->out_refs = malloc(out_refs_bytes);
        memset(obj->out_refs, 0, out_refs_bytes);
        memset(obj->refs_bitmap, 0xff, sizeof(obj->refs_bitmap));

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

static PyObject* Py_TLObject_read_bytesio(PyTypeObject* cls, PyObject* args) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();

    PyObject* bio;
    bool read_only = false;
    if (!PyArg_ParseTuple(args, "O!|p", state->bytesio_type, &bio, &read_only)) {
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

static PyObject* Py_TLObject_read_bytes(PyTypeObject* cls, PyObject* args) {
    uint8_t* buf;
    size_t buf_len;
    bool read_only = false;
    if (!PyArg_ParseTuple(args, "y#|p", &buf, &buf_len, &read_only)) {
        return NULL;
    }

    PyObject* result = Py_TLObject_read(cls, buf, buf_len, NULL, read_only);
    if(read_only) {
        PyObject* bytes;
        if (!PyArg_ParseTuple(args, "O!|p", PyBytes_Type, &bytes)) {
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

    if(tlobject_is_readonly(self)) {
        uint8_t* data = malloc(self->ro_message->size + 4);
        memcpy(data, &self->ro_message->message_def->id, 4);
        memcpy(data + 4, self->ro_message->data, self->ro_message->size);
        result = PyBytes_FromStringAndSize((char*)data, self->ro_message->size + 4);
        free(data);
    } else {
        size_t written_bytes;
        char* bytes = utl_encode(self->message, &written_bytes);
        result = PyBytes_FromStringAndSize(bytes, written_bytes);
        free(bytes);
    }

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

PyType_Spec pyutl_TLObjectType_spec = {
    "_pyutl.TLObject",
    sizeof(Py_TLObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_IS_ABSTRACT,
    Py_TLObject_slots,
};

PyObject* Py_TLObject_createType(utl_MessageDef* message_def) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();

    const size_t alloc_size = 7 + (message_def->namespace_.size ? message_def->namespace_.size + 1 : 0) + message_def->name.size;
    char* name = malloc(alloc_size + 1);
    name[alloc_size] = '\0';
    memcpy(name, "_pyutl.", 7);
    if(message_def->namespace_.size)
        memcpy(name + 7, message_def->namespace_.data, message_def->namespace_.size);
    memcpy(name + 7 + message_def->namespace_.size, message_def->name.data, message_def->name.size);

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
    // TODO: i have no idea what to do with this memory, if i free it - it breaks type name in python, if dont - then it is a memory leak
    // free(name);
    if(!new_type) {
        return 0;
    }

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