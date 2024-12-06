#include "pyutl.h"
#include "tltype.h"


static PyType_Slot Py_TLObject_slots[] = {
    {0, NULL}
};

PyType_Spec pyutl_TLTypeType_spec = {
    "_pyutl.TLType",
    sizeof(Py_TLType),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    Py_TLObject_slots,
};

PyObject* Py_TLType_createType(utl_TypeDef* type_def) {
    const pyutl_ModuleState* state = pyutl_ModuleState_get();
    arena_t* arena = &state->c_def_pool->arena;
    const size_t real_size = arena->size;

    char* name = arena_alloc(arena, 11 + type_def->name.size);
    memcpy(name, "_pyutl._tl.", 11);
    memcpy(name + 11, type_def->name.data, type_def->name.size);

    PyType_Slot slots[] = {
        {Py_tp_base, state->tltype_type},
        {0, NULL}
    };

    PyType_Spec spec = {
        name,
        0,
        0,
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
        slots,
    };

    PyObject* new_type = PyType_FromSpec(&spec);
    if(!new_type) {
        arena->size = real_size;
        return 0;
    }

    arena->size = real_size;

    PyObject* typedef_capsule = PyCapsule_New(type_def, NULL, NULL);
    if (!typedef_capsule) {
        goto failed;
    }

    if (PyObject_SetAttrString(new_type, "__type_def__", typedef_capsule) < 0) {
        goto failed;
    }

    Py_DECREF(typedef_capsule);
    return new_type;

    failed:
        Py_XDECREF(typedef_capsule);
    Py_XDECREF(new_type);
    return NULL;
}
