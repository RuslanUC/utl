#include "pyutl.h"
#include "py_def_pool.h"
#include "tlobject.h"
#include "tlvector.h"

PyMethodDef method_table[] = {
    {NULL, NULL, 0, NULL}
};

PyModuleDef _pyutl_module = {
    PyModuleDef_HEAD_INIT,
    "_pyutl",
    0,
    sizeof(pyutl_ModuleState),
    method_table,
};

pyutl_ModuleState* pyutl_ModuleState_get() {
    PyObject* module = PyState_FindModule(&_pyutl_module);
    assert(module);
    return PyModule_GetState(module);
}

PyMODINIT_FUNC PyInit__pyutl(void) {
    PyObject* pyutl_DefPoolType = NULL;
    PyObject* pyutl_TLObjectType = NULL;
    PyObject* pyutl_TLVectorType = NULL;
    PyObject* collections = NULL;
    PyObject* seq = NULL;
    PyObject* seq_ret = NULL;

    PyObject* m = PyModule_Create(&_pyutl_module);
    if(!m) {
        goto failed;
    }
    pyutl_ModuleState* state = PyModule_GetState(m);

    pyutl_DefPoolType = PyType_FromSpec(&pyutl_DefPoolType_spec);
    if(!pyutl_DefPoolType) {
        goto failed;
    }
    pyutl_TLObjectType = PyType_FromSpec(&pyutl_TLObjectType_spec);
    if(!pyutl_TLObjectType) {
        goto failed;
    }
    pyutl_TLVectorType = PyType_FromSpec(&pyutl_TLVectorType_spec);
    if(!pyutl_TLVectorType) {
        goto failed;
    }

    if(PyModule_AddObject(m, "TLObject", pyutl_TLObjectType)) {
        goto failed;
    }
    if(PyModule_AddObject(m, "TLVector", pyutl_TLVectorType)) {
        goto failed;
    }

    collections = PyImport_ImportModule("collections.abc");
    if(!collections) {
        goto failed;
    }
    seq = PyObject_GetAttrString(collections, "MutableSequence");
    if(!seq) {
        goto failed;
    }
    seq_ret = PyObject_CallMethod(seq, "register", "O", pyutl_TLVectorType);
    if(!seq_ret) {
        goto failed;
    }
    Py_XDECREF(collections);
    Py_XDECREF(seq);
    Py_XDECREF(seq_ret);

    state->def_pool_type = (PyTypeObject*)pyutl_DefPoolType;
    state->tlobject_type = (PyTypeObject*)pyutl_TLObjectType;
    state->tlvector_type = (PyTypeObject*)pyutl_TLVectorType;
    state->default_def_pool = PyObject_CallObject(pyutl_DefPoolType, 0);
    if(!state->default_def_pool || PyModule_AddObject(m, "def_pool", state->default_def_pool)) {
        goto failed;
    }
    state->default_c_def_pool = ((Py_DefPool*)state->default_def_pool)->pool;
    state->messages_cache = utl_Map_new_on_arena(state->default_c_def_pool->message_defs->buckets_num, &state->default_c_def_pool->arena);
    state->objects_cache = utl_PtrMap_new(128);

    return m;

failed:
    Py_XDECREF(collections);
    Py_XDECREF(seq);
    Py_XDECREF(seq_ret);
    Py_XDECREF(pyutl_DefPoolType);
    Py_XDECREF(pyutl_TLObjectType);
    Py_XDECREF(pyutl_TLVectorType);
    Py_XDECREF(m);
    return NULL;
}
