// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/object/life_support.hpp>
#include <boost/python/detail/none.hpp>
#include <boost/python/refcount.hpp>

namespace boost { namespace python { namespace objects { 

struct life_support
{
    PyObject_HEAD
    PyObject* patient;
};

extern "C"
{
    static void
    life_support_dealloc(PyObject* self)
    {
        Py_XDECREF(((life_support*)self)->patient);
        self->ob_type->tp_free(self);
    }

    static PyObject *
    life_support_call(PyObject *self, PyObject *arg, PyObject * /*kw*/)
    {
        // Let the patient die now
        Py_XDECREF(((life_support*)self)->patient);
        ((life_support*)self)->patient = 0;
        // Let the weak reference die. This probably kills us.
        Py_XDECREF(PyTuple_GET_ITEM(arg, 0));
        return ::boost::python::detail::none();
    }
}

PyTypeObject life_support_type = {
    PyVarObject_HEAD_INIT(NULL, 0)//(&PyType_Type)
    const_cast<char*>("Boost.Python.life_support"),
    sizeof(life_support),
    0,
    life_support_dealloc,               /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0, //(reprfunc)func_repr,                   /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    life_support_call,                  /* tp_call */
    0,                                  /* tp_str */
    0, // PyObject_GenericGetAttr,            /* tp_getattro */
    0, // PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT /* | Py_TPFLAGS_HAVE_GC */,/* tp_flags */
    0,                                  /* tp_doc */
    0, // (traverseproc)func_traverse,          /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0, //offsetof(PyLife_SupportObject, func_weakreflist), /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0, // func_memberlist,                      /* tp_members */
    0, //func_getsetlist,                       /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0, //offsetof(PyLife_SupportObject, func_dict),      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    0,                                      /* tp_new */
    0,                                      /* tp_free */
    0,                                      /* tp_is_gc */
    0,                                      /* tp_bases */
    0,                                      /* tp_mro */
    0,                                      /* tp_cache */
    0,                                      /* tp_subclasses */
    0,                                      /* tp_weaklist */
#if PYTHON_API_VERSION >= 1012
    0                                       /* tp_del */
#endif
};

PyObject* make_nurse_and_patient(PyObject* nurse, PyObject* patient)
{
    if (nurse == Py_None || nurse == patient)
        return nurse;
    
    if (Py_TYPE(&life_support_type) == 0)
    {
        Py_TYPE(&life_support_type) = &PyType_Type;
        PyType_Ready(&life_support_type);
    }
    
    life_support* system = PyObject_New(life_support, &life_support_type);
    if (!system)
        return 0;

    system->patient = 0;
    
    // We're going to leak this reference, but don't worry; the
    // life_support system decrements it when the nurse dies.
    PyObject* weakref = PyWeakref_NewRef(nurse, (PyObject*)system);

    // weakref has either taken ownership, or we have to release it
    // anyway
    Py_DECREF(system);
    if (!weakref)
        return 0;
    
    system->patient = patient;
    Py_XINCREF(patient); // hang on to the patient until death
    return weakref;
}

}}} // namespace boost::python::objects
