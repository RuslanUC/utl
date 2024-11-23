#pragma once

#include <Python.h>
#include "parser.h"
#include "message.h"

typedef struct Py_TLObject {
    PyObject_HEAD;
    utl_Message* message;
} Py_TLObject;

extern PyType_Spec pyutl_TLObjectType_spec;

PyObject* Py_TLObject_createType(utl_MessageDef* message_def);