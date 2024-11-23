#include "pyutl.h"
#include "py_def_pool.h"
#include "tlobject.h"

static void Py_DefPool_dealloc(PyObject* self) {
    utl_DefPool_free(((Py_DefPool*)self)->pool);
    self->ob_type->tp_free(self);
}

static PyObject* Py_DefPool_new(PyTypeObject* cls, PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyObject* self = cls->tp_alloc(cls, 0);
    ((Py_DefPool*)self)->pool = utl_DefPool_new();

    return self;
}

static int Py_DefPool_init(PyObject* self, PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    return 0;
}

static PyObject* Py_DefPool_parse(Py_DefPool* self, PyObject* args) {
    char* str;
    size_t str_len;

    if (!PyArg_ParseTuple(args, "s#", &str, &str_len)) {
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

    //return Py_TLObject_createType(message_def);

    return Py_BuildValue("K", (uint64_t)message_def); // TODO: return _pyutl.TLObject subclass when python types cache will be added
}

static PyObject* Py_DefPool_has_type(Py_DefPool* self, PyObject* args) {
    char* str;
    size_t str_len;

    if (!PyArg_ParseTuple(args, "s#", &str, &str_len)) {
        return NULL;
    }

    utl_StringView name = {
        .size = str_len,
        .data = str,
    };
    return PyBool_FromLong(utl_DefPool_hasType(self->pool, name));
}

static PyObject* Py_DefPool_has_constructor(Py_DefPool* self, PyObject* args) {
    uint32_t tl_id;

    if (!PyArg_ParseTuple(args, "I", &tl_id)) {
        return NULL;
    }

    return PyBool_FromLong(utl_DefPool_hasMessage(self->pool, tl_id));
}

static PyObject* Py_DefPool_get_constructor(Py_DefPool* self, PyObject* args) {
    uint32_t tl_id;

    if (!PyArg_ParseTuple(args, "I", &tl_id)) {
        return NULL;
    }

    utl_MessageDef* message_def = utl_DefPool_getMessage(self->pool, tl_id);
    if(!message_def)
        return Py_BuildValue("");

    return Py_BuildValue("K", (uint64_t)message_def); // TODO: return _pyutl.TLObject subclass when python types cache will be added
}

static PyMethodDef Py_DefPool_methods[] = {
    {"parse", (PyCFunction)Py_DefPool_parse, METH_VARARGS, 0,},
    {"has_type", (PyCFunction)Py_DefPool_has_type, METH_VARARGS, 0,},
    {"has_constructor", (PyCFunction)Py_DefPool_has_constructor, METH_VARARGS, 0,},
    {"get_constructor", (PyCFunction)Py_DefPool_get_constructor, METH_VARARGS, 0,},
    {NULL}
};

static PyType_Slot Py_DefPool_slots[] = {
    {Py_tp_dealloc, Py_DefPool_dealloc},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {Py_tp_methods, Py_DefPool_methods},
    {Py_tp_new, Py_DefPool_new},
    {Py_tp_init, Py_DefPool_init},
    {0, NULL}
};

PyType_Spec pyutl_DefPoolType_spec = {
    "_pyutl.DefPool",
    sizeof(Py_DefPool),
    0,
    Py_TPFLAGS_DEFAULT,
    Py_DefPool_slots,
};
