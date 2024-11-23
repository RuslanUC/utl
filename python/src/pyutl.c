#include "pyutl.h"

PyMethodDef method_table[] = {
    {NULL, NULL, 0, NULL}
};

PyModuleDef _pyutl_module = {
    PyModuleDef_HEAD_INIT,
    "_pyutl",
    0,
    -1,
    method_table,
};

PyMODINIT_FUNC PyInit__pyutl(void) {
    PyObject* m = PyModule_Create(&_pyutl_module);
    if(!m) {
        return 0;
    }

    PyObject* pyutl_DefPoolType = PyType_FromSpec(&pyutl_DefPoolType_spec);
    if(!m) {
        Py_DECREF(m);
        return 0;
    }

    if(PyModule_AddObject(m, "DefPool", pyutl_DefPoolType)) {
        Py_DECREF(pyutl_DefPoolType);
        Py_DECREF(m);
        return 0;
    }

    return m;
}
