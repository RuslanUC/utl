#pragma once

#include <Python.h>
#include "parser.h"
#include "message.h"

typedef struct Py_TLVector {
    PyObject_HEAD;
    utl_Vector* vector;
} Py_TLVector;

extern PyType_Spec pyutl_TLVectorType_spec;

void Py_TLVector_dealloc_recursive(utl_Vector* vector);
void* Py_TLVector_item_to_utl(utl_Vector* vector, PyObject* item);
void Py_TLVector_init_message(Py_TLVector* self, utl_Vector* message);
PyObject* Py_TLVector_createType(utl_MessageDefVector* message_def);