#pragma once

#include <Python.h>
#include "message.h"
#include "message_ro.h"

typedef struct Py_TLObject {
    PyObject_HEAD;
    union {
        utl_Message* message;
        utl_RoMessage* ro_message;
    };
    // If object is read-only, then actual out_refs size is <number of message fields>+1,
    //  extra one for "bytes" object from which object was created
    void** out_refs;
    // If a bit at <field num> is set - then out_refs[<field num>] is PyObject* (always if object is read-only),
    //  if not set - then out_refs[<field num>] is utl_Message* or utl_Vector*
    // If object is read-only, a last bit of last byte (refs_bitmap[3] & (1 << 63)) is set
    // NOTE: I think 256 bits is enough for now, since there is no tl object (yet) with >= 255 fields
    uint64_t refs_bitmap[4];
} Py_TLObject;

extern PyType_Spec pyutl_TLObjectType_spec;

void Py_TLObject_init_message(Py_TLObject* self, utl_MessageDef* def, utl_Message* message);
void Py_TLObject_init_message_ro(Py_TLObject* self, utl_RoMessage* message);
PyObject* Py_TLObject_createType(utl_MessageDef* message_def);