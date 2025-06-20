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
    utl_StaticMap* messages_cache;
} pyutl_ModuleState;

PyMODINIT_FUNC PyInit__pyutl(void);

pyutl_ModuleState* pyutl_ModuleState_get();

extern PyTypeObject* def_pool_type;
extern PyTypeObject* tlobject_type;
extern PyTypeObject* tlvector_type;
extern PyTypeObject* tltype_type;
extern PyObject* bytesio_type;
extern utl_DefPool* c_def_pool;
extern PyObject* py_def_pool;
