#pragma once

#include <Python.h>
#include "parser.h"
#include "message.h"

typedef struct Py_TLObject {
    PyObject_HEAD;
    utl_Message* message;
} Py_TLObject;

extern PyType_Spec pyutl_TLObjectType_spec;

static void Py_TLObject_init_message(Py_TLObject* self, utl_MessageDef* def, utl_Message* message);
PyObject* Py_TLObject_createType(utl_MessageDef* message_def);