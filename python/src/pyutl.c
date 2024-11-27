#include "pyutl.h"
#include "py_def_pool.h"
#include "tlobject.h"

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
    PyObject* m = PyModule_Create(&_pyutl_module);
    if(!m) {
        return 0;
    }
    pyutl_ModuleState* state = PyModule_GetState(m);

    PyObject* pyutl_DefPoolType = PyType_FromSpec(&pyutl_DefPoolType_spec);
    if(!pyutl_DefPoolType) {
        Py_DECREF(m);
        return 0;
    }
    PyObject* pyutl_TLObjectType = PyType_FromSpec(&pyutl_TLObjectType_spec);
    if(!pyutl_TLObjectType) {
        Py_DECREF(pyutl_DefPoolType);
        Py_DECREF(m);
        return 0;
    }

    if(PyModule_AddObject(m, "DefPool", pyutl_DefPoolType)) {
        Py_DECREF(pyutl_DefPoolType);
        Py_DECREF(pyutl_TLObjectType);
        Py_DECREF(m);
        return 0;
    }
    if(PyModule_AddObject(m, "TLObject", pyutl_TLObjectType)) {
        Py_DECREF(pyutl_DefPoolType);
        Py_DECREF(pyutl_TLObjectType);
        Py_DECREF(m);
        return 0;
    }

    state->def_pool_type = (PyTypeObject*)pyutl_DefPoolType;
    state->tlobject_type = (PyTypeObject*)pyutl_TLObjectType;
    state->default_def_pool = PyObject_CallObject(pyutl_DefPoolType, 0);
    if(!state->default_def_pool || PyModule_AddObject(m, "default_def_pool", state->default_def_pool)) {
        Py_DECREF(pyutl_DefPoolType);
        Py_DECREF(pyutl_TLObjectType);
        Py_DECREF(m);
        return 0;
    }
    state->default_c_def_pool = ((Py_DefPool*)state->default_def_pool)->pool;
    state->messages_cache = utl_Map_new_on_arena(state->default_c_def_pool->message_defs->buckets_num, &state->default_c_def_pool->arena);
    state->objects_cache = utl_PtrMap_new(128);

    return m;
}
