#pragma once

#include <Python.h>
#include "vector.h"
#include "vector_ro.h"

typedef struct Py_TLVector {
    PyObject_HEAD;
    union {
        utl_Vector* vector;
        utl_RoVector* ro_vector;
    };
    // If object is read-only, then actual out_refs size is <elements_count>+1,
    //  extra one for "bytes" object from which vector was created
    void** out_refs;
    // If a bit at <index> is set - then out_refs[<index>] is PyObject* (always if vector is read-only),
    //  if not set - then out_refs[<index>] is utl_Message* or utl_Vector*
    // If vector is read-only, a last bit of last byte (refs_bitmap[refs_bitmap_bytes - 1] & (1 << 63)) is set
    uint64_t* refs_bitmap;
    size_t refs_bitmap_bytes;
} Py_TLVector;

extern PyType_Spec pyutl_TLVectorType_spec;

bool Py_TLVector_item_set(utl_Vector* vector, PyObject* item, ssize_t index);
void Py_TLVector_init_message(Py_TLVector* self, utl_Vector* vector);
void Py_TLVector_init_message_ro(Py_TLVector* self, utl_RoVector* vector);
PyObject* Py_TLVector_createType(utl_MessageDefVector* message_def);