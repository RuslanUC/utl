#pragma once

#include <Python.h>
#include "parser.h"
#include "message.h"

typedef struct Py_TLVector {
    PyObject_HEAD;
    utl_Vector* vector;
} Py_TLVector;

extern PyType_Spec pyutl_TLVectorType_spec;

static void Py_TLVector_init_message(Py_TLVector* self, utl_Vector* message);
PyObject* Py_TLVector_createType(utl_MessageDefVector* message_def);