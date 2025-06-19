#pragma once

#include <Python.h>
#include "def_pool.h"

typedef struct pyutl_MessageDef {
    PyTypeObject* python_cls;
    // TODO: store attrs in sorted string array where each string index corresponds to `message_def->fields` index
    //  instead of storing fields inside map entry
    utl_StaticMap* fields;
} pyutl_MessageDef;

typedef struct pyutl_ModuleState {
    PyTypeObject* def_pool_type;
    PyTypeObject* tlobject_type;
    PyTypeObject* tlvector_type;
    PyTypeObject* tltype_type;
    PyObject* bytesio_type;
    utl_DefPool* c_def_pool;
    PyObject* py_def_pool;

    utl_StaticMap* messages_cache;
} pyutl_ModuleState;

PyMODINIT_FUNC PyInit__pyutl(void);

pyutl_ModuleState* pyutl_ModuleState_get();
