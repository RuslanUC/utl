#include "py_def_pool.h"

static void Py_DefPool_dealloc(PyObject* self) {
    utl_DefPool_free(((Py_DefPool*)self)->pool);
    self->ob_type->tp_free(self);
}

static PyObject* Py_DefPool_new(PyTypeObject* cls, PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyObject* self = cls->tp_alloc(cls, 0);
    ((Py_DefPool*)self)->pool = utl_DefPool_new();

    return self;
}

static int Py_DefPool_init(PyObject* self, PyObject* args, PyObject* kwds) {
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

    return Py_BuildValue("K", (uint64_t)message_def);
}

PyMethodDef Py_DefPool_methods[] = {
    {"parse", (PyCFunction)Py_DefPool_parse, METH_VARARGS, 0,},
    {NULL}
};

PyTypeObject pyutl_DefPoolType = {
    PyObject_HEAD_INIT(NULL)
    "_pyutl.DefPool",
    sizeof(Py_DefPool),
    0,
    (destructor)Py_DefPool_dealloc,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    Py_DefPool_methods,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    (initproc)Py_DefPool_init,
    0,
    Py_DefPool_new,
    0,
};