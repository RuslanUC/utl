#pragma once

#include <Python.h>
#include "vector.h"
#include "vector_ro.h"

typedef struct Py_TLVector {
    PyObject_HEAD;
    union {
        utl_VectorHeader* vector_hdr;
        utl_Vector* vector;
        utl_RoVector* ro_vector;
    };
    // If object is read-only, then actual out_refs size is <elements_count>+1,
    //  extra one for "bytes" object from which vector was created
    PyObject** out_refs;
    bool readonly;
} Py_TLVector;

extern PyType_Spec pyutl_TLVectorType_spec;

bool Py_TLVector_item_set(utl_Vector* vector, PyObject* item, int32_t index);
void Py_TLVector_init_message(Py_TLVector* self, utl_Vector* vector);
void Py_TLVector_init_message_ro(Py_TLVector* self, utl_RoVector* vector);
PyObject* Py_TLVector_createType(utl_MessageDefVector* message_def);

void pyutl_internal_tlvector_free_recursive(utl_VectorHeader* obj, bool is_readonly);