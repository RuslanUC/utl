#pragma once

#include <Python.h>
#include "def_pool.h"

typedef struct pyutl_MessageDef {
    enum {
        PYUTL_CACHED_OBJECT,
        PYUTL_CACHED_TYPE,
    } type;

    PyTypeObject* python_cls;

    char* field_names_buf;
    char** field_names;
    uint8_t* field_nums;
} pyutl_MessageDef;

typedef struct {
    intptr_t key;
    pyutl_MessageDef* value;
} PyMessageDefPair;

typedef struct pyutl_ModuleState {
    utl_DefPool* c_def_pool;
    PyObject* py_def_pool;
    PyMessageDefPair* defs_cache;
} pyutl_ModuleState;

PyMODINIT_FUNC PyInit__pyutl(void);

pyutl_ModuleState* pyutl_ModuleState_get();

extern PyTypeObject* def_pool_type;
extern PyTypeObject* tlobject_type;
extern PyTypeObject* tlvector_type;
extern PyTypeObject* tltype_type;
extern PyObject* bytesio_type;

int binary_search_str(char** arr, int arr_length, const char* key, int key_length);
