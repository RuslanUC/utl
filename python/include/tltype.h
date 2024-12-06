#pragma once

#include <Python.h>
#include "type_def.h"

typedef struct Py_TLType {
    PyObject_HEAD;
} Py_TLType;

extern PyType_Spec pyutl_TLTypeType_spec;

PyObject* Py_TLType_createType(utl_TypeDef* type_def);
