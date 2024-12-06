#pragma once

#include <Python.h>
#include "parser.h"
#include "def_pool.h"

typedef struct Py_DefPool {
    PyObject_HEAD;
    utl_DefPool* pool;
} Py_DefPool;

PyObject* Py_DefPool_parse(const Py_DefPool* self, PyObject* args);
PyObject* Py_DefPool_has_type(const Py_DefPool* self, PyObject* args);
PyObject* Py_DefPool_has_constructor(const Py_DefPool* self, PyObject* args);
PyObject* Py_DefPool_get_constructor(const Py_DefPool* self, PyObject* args);
PyObject* Py_DefPool_create_type(const Py_DefPool* self, PyObject* args);
PyObject* Py_DefPool_get_type(const Py_DefPool* self, PyObject* args);

extern PyType_Spec pyutl_DefPoolType_spec;