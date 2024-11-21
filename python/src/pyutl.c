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
    NULL,
    NULL,
    NULL,
    NULL,
};

PyMODINIT_FUNC PyInit__pyutl(void) {
    if (PyType_Ready(&pyutl_DefPoolType) < 0)
        return 0;

    PyObject* m = PyModule_Create(&_pyutl_module);
    Py_INCREF(&pyutl_DefPoolType);
    PyModule_AddObject(m, "DefPool", (PyObject *)&pyutl_DefPoolType);

    return m;
}