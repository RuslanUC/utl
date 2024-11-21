#pragma once

#include <Python.h>
#include "parser.h"
#include "def_pool.h"

typedef struct Py_DefPool {
    PyObject_HEAD;
    utl_DefPool* pool;
} Py_DefPool;

extern PyTypeObject pyutl_DefPoolType;