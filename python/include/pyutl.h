#pragma once

#include <Python.h>
#include "def_pool.h"

typedef struct pyutl_ModuleState {
    PyTypeObject* def_pool_type;
    utl_DefPool* default_c_def_pool;
    PyObject* default_def_pool;
    PyTypeObject* tlobject_type;
} pyutl_ModuleState;

PyMODINIT_FUNC PyInit__pyutl(void);

pyutl_ModuleState* pyutl_ModuleState_get();
