#pragma once

#include <Python.h>
#include "def_pool.h"
#include "message_def.h"
#include "ptr_map.h"

typedef struct pyutl_MessageDef {
    PyTypeObject* python_cls;
    utl_StaticMap* fields;
} pyutl_MessageDef;

typedef struct pyutl_ModuleState {
    PyTypeObject* def_pool_type;
    utl_DefPool* default_c_def_pool;
    PyObject* default_def_pool;
    PyTypeObject* tlobject_type;

    utl_StaticMap* messages_cache;
    utl_PtrMap* objects_cache;
} pyutl_ModuleState;

PyMODINIT_FUNC PyInit__pyutl(void);

pyutl_ModuleState* pyutl_ModuleState_get();
