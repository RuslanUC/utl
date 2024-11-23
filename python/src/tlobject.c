#include "pyutl.h"
#include "tlobject.h"

static void Py_TLObject_dealloc(PyObject* self) {
    utl_Message_free(((Py_TLObject*)self)->message);
    self->ob_type->tp_free(self);
}

static PyObject* Py_TLObject_new(PyTypeObject* cls, PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    PyObject* result = PyObject_GetAttrString((PyObject*)cls, "__message_def__");
    if(!result) {
        PyErr_SetString(PyExc_NotImplementedError, "Object of type \"TLObject cannot be instantiated\".");
        return 0;
    }

    PyObject* self = cls->tp_alloc(cls, 0);
    ((Py_TLObject*)self)->message = utl_Message_new(PyCapsule_GetPointer(result, NULL));

    // TODO: set fields from kwargs

    return self;
}

static int Py_TLObject_init(PyObject* self, PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwargs)) {
    return 0;
}

PyObject* Py_TLObject_getattro(PyObject* self, PyUnicodeObject* attr) {
    return Py_BuildValue("");  // TODO
}

int Py_TLObject_setattro(PyObject* self, PyUnicodeObject* attr, PyObject* value) {
    return 0; // NOTE: -1 on error
}

static PyObject* Py_TLObject_read(PyTypeObject* cls, PyObject* args) {
    return Py_BuildValue("");  // TODO
}

static PyObject* Py_TLObject_write(PyObject* self, PyObject* args) {
    return Py_BuildValue(""); // TODO
}

static PyMethodDef Py_TLObject_methods[] = {
    {"read", (PyCFunction)Py_TLObject_read, METH_VARARGS | METH_CLASS, 0,},
    {"write", (PyCFunction)Py_TLObject_write, METH_VARARGS, 0,},
    {NULL}
};

static PyType_Slot Py_TLObject_slots[] = {
    {Py_tp_dealloc, Py_TLObject_dealloc},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {Py_tp_methods, Py_TLObject_methods},
    {Py_tp_new, Py_TLObject_new},
    {Py_tp_init, Py_TLObject_init},
    {Py_tp_getattro, Py_TLObject_getattro},
    {Py_tp_setattro, Py_TLObject_setattro},
    {0, NULL}
};

PyType_Spec pyutl_TLObjectType_spec = {
    "_pyutl.TLObject",
    sizeof(Py_TLObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    Py_TLObject_slots,
};

PyObject* Py_TLObject_createType(utl_MessageDef* message_def) {
    pyutl_ModuleState* state = pyutl_ModuleState_get();
    arena_t* arena = &state->default_c_def_pool->arena;
    size_t real_size = arena->size;

    char* name = arena_alloc(
        arena, 11 + (message_def->namespace_.size ? message_def->namespace_.size + 1 : 0) + message_def->name.size);
    memcpy(name, "_pyutl._tl.", 11);
    if(message_def->namespace_.size)
        memcpy(name+11, message_def->namespace_.data, message_def->namespace_.size);
    memcpy(name+11+message_def->namespace_.size, message_def->name.data, message_def->name.size);

    PyType_Slot slots[] = {
        {Py_tp_base, state->tlobject_type},
        {0, NULL}
    };

    PyType_Spec spec = {
        name,
        sizeof(Py_TLObject), // TODO: is structure inherited from tp_base?
        0,
        Py_TPFLAGS_DEFAULT,
        slots,
    };

    PyObject* new_type = PyType_FromSpec(&spec);
    if(!new_type) {
        arena->size = real_size;
        return 0;
    }

    arena->size = real_size;

    PyObject* msgdef_capsule = PyCapsule_New(message_def, NULL, NULL);
    if (!msgdef_capsule) {
        Py_DECREF(new_type);
        return 0;
    }

    if (PyObject_SetAttrString(new_type, "__message_def__", msgdef_capsule) < 0) {
        Py_DECREF(msgdef_capsule);
        Py_DECREF(new_type);
        return 0;
    }

    Py_DECREF(msgdef_capsule);

    return new_type;
}