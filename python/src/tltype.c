#include "stb_ds.h"
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
    pyutl_ModuleState* state = pyutl_ModuleState_get();

    const ptrdiff_t cached_def_idx = hmgeti(state->messages_cache, (intptr_t)type_def);
    if(cached_def_idx >= 0)
        return (PyObject*)state->messages_cache[cached_def_idx].value->python_cls;

    const size_t alloc_size = 7 + type_def->name.size;
    char* name = malloc(alloc_size + 1);
    name[alloc_size] = '\0';
    memcpy(name, "_pyutl.", 7);
    memcpy(name + 7, type_def->name.data, type_def->name.size);

    PyType_Slot slots[] = {
        {Py_tp_base, tltype_type},
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
    // TODO: i have no idea what to do with this memory, if i free it - it breaks type name in python, if dont - then it is a memory leak
    // free(name);
    if(!new_type) {
        return 0;
    }

    PyObject* typedef_capsule = PyCapsule_New(type_def, NULL, NULL);
    if (!typedef_capsule) {
        goto failed;
    }

    if (PyObject_SetAttrString(new_type, "__type_def__", typedef_capsule) < 0) {
        goto failed;
    }

    Py_DECREF(typedef_capsule);

    pyutl_MessageDef*cached_def = utl_Arena_alloc(&c_def_pool->arena, sizeof(pyutl_MessageDef));
    cached_def->python_cls = (PyTypeObject*)new_type;
    cached_def->field_names_buf = NULL;
    cached_def->field_names = NULL;
    cached_def->field_nums = NULL;
    hmput(state->messages_cache, (intptr_t)type_def, cached_def);

    return new_type;

failed:
    Py_XDECREF(typedef_capsule);
    Py_XDECREF(new_type);
    return NULL;
}
