#include "pyutl.h"
#include "py_def_pool.h"
#include "tlobject.h"
#include "tlvector.h"
#include "tltype.h"

PyObject* pyutl_parse_tl(PyObject* Py_UNUSED(self), PyObject* args) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    return Py_DefPool_parse((const Py_DefPool*)state->py_def_pool, args);
}

PyObject* pyutl_has_type(PyObject* Py_UNUSED(self), PyObject* args) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    return Py_DefPool_has_type((const Py_DefPool*)state->py_def_pool, args);
}

PyObject* pyutl_get_type(PyObject* Py_UNUSED(self), PyObject* args) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    return Py_DefPool_get_type((const Py_DefPool*)state->py_def_pool, args);
}

PyObject* pyutl_create_type(PyObject* Py_UNUSED(self), PyObject* args) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    return Py_DefPool_create_type((const Py_DefPool*)state->py_def_pool, args);
}

PyObject* pyutl_has_constructor(PyObject* Py_UNUSED(self), PyObject* args) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    return Py_DefPool_has_constructor((const Py_DefPool*)state->py_def_pool, args);
}

PyObject* pyutl_get_constructor(PyObject* Py_UNUSED(self), PyObject* args) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    return Py_DefPool_get_constructor((const Py_DefPool*)state->py_def_pool, args);
}

PyMethodDef method_table[] = {
    {"parse_tl", (PyCFunction)pyutl_parse_tl, METH_VARARGS, 0,},
    {"has_type", (PyCFunction)pyutl_has_type, METH_VARARGS, 0,},
    {"get_type", (PyCFunction)pyutl_get_type, METH_VARARGS, 0,},
    {"create_type", (PyCFunction)pyutl_create_type, METH_VARARGS, 0,},
    {"has_constructor", (PyCFunction)pyutl_has_constructor, METH_VARARGS, 0,},
    {"get_constructor", (PyCFunction)pyutl_get_constructor, METH_VARARGS, 0,},
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
    PyObject* pyutl_TLTypeType = NULL;
    PyObject* collections = NULL;
    PyObject* seq = NULL;
    PyObject* seq_ret = NULL;
    PyObject* io = NULL;

    PyObject* m = PyModule_Create(&_pyutl_module);
    if(!m) {
        goto failed;
    }

    pyutl_ModuleState* state = PyModule_GetState(m);
    state->bytesio_type = NULL;

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
    pyutl_TLTypeType = PyType_FromSpec(&pyutl_TLTypeType_spec);
    if(!pyutl_TLTypeType) {
        goto failed;
    }

    if(PyModule_AddObject(m, "TLObject", pyutl_TLObjectType)) {
        goto failed;
    }
    if(PyModule_AddObject(m, "TLVector", pyutl_TLVectorType)) {
        goto failed;
    }
    if(PyModule_AddObject(m, "TLType", pyutl_TLTypeType)) {
        goto failed;
    }

    io = PyImport_ImportModule("io");
    if(!io) {
        goto failed;
    }
    state->bytesio_type = PyObject_GetAttrString(io, "BytesIO");
    if(!state->bytesio_type) {
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
    state->tltype_type = (PyTypeObject*)pyutl_TLTypeType;
    state->py_def_pool = PyObject_CallObject(pyutl_DefPoolType, 0);
    if(!state->py_def_pool || PyModule_AddObject(m, "def_pool", state->py_def_pool)) {
        goto failed;
    }
    state->c_def_pool = ((Py_DefPool*)state->py_def_pool)->pool;
    state->messages_cache = utl_Map_new_on_arena(state->c_def_pool->message_defs->buckets_num, &state->c_def_pool->arena);

    return m;

failed:
    Py_XDECREF(state->bytesio_type);
    Py_XDECREF(io);
    Py_XDECREF(collections);
    Py_XDECREF(seq);
    Py_XDECREF(seq_ret);
    Py_XDECREF(pyutl_DefPoolType);
    Py_XDECREF(pyutl_TLObjectType);
    Py_XDECREF(pyutl_TLVectorType);
    Py_XDECREF(pyutl_TLTypeType);
    Py_XDECREF(m);
    return NULL;
}
