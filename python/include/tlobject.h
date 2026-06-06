#pragma once

#include <Python.h>
#include "message.h"
#include "message_ro.h"

typedef struct Py_TLObject {
    PyObject_HEAD;
    union {
        utl_MessageHeader* message_hdr;
        utl_Message* message;
        utl_RoMessage* ro_message;
    };
    // If object is read-only, then actual out_refs size is <number of message fields>+1,
    //  extra one for "bytes" object from which object was created
    PyObject** out_refs;
    bool readonly;
} Py_TLObject;

extern PyType_Spec pyutl_TLObjectType_spec;

void Py_TLObject_init_message(Py_TLObject* self, utl_MessageDef* def, utl_Message* message);
void Py_TLObject_init_message_ro(Py_TLObject* self, utl_RoMessage* message);
PyObject* Py_TLObject_createType(const utl_MessageDef* message_def);

void pyutl_internal_tlobject_free_recursive(utl_MessageHeader* obj, bool is_readonly);
