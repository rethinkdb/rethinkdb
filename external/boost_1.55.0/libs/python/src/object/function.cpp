// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/docstring_options.hpp>
#include <boost/python/object/function_object.hpp>
#include <boost/python/object/function_handle.hpp>
#include <boost/python/object/function_doc_signature.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/str.hpp>
#include <boost/python/object_attributes.hpp>
#include <boost/python/args.hpp>
#include <boost/python/refcount.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/list.hpp>
#include <boost/python/ssize_t.hpp>

#include <boost/python/detail/signature.hpp>
#include <boost/python/detail/none.hpp>
#include <boost/mpl/vector/vector10.hpp>

#include <boost/bind.hpp>

#include <algorithm>
#include <cstring>

#if BOOST_PYTHON_DEBUG_ERROR_MESSAGES
# include <cstdio>
#endif

namespace boost { namespace python {
  volatile bool docstring_options::show_user_defined_ = true;
  volatile bool docstring_options::show_cpp_signatures_ = true;
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
  volatile bool docstring_options::show_py_signatures_ = true;
#else
  volatile bool docstring_options::show_py_signatures_ = false;
#endif
}}

namespace boost { namespace python { namespace objects { 

py_function_impl_base::~py_function_impl_base()
{
}

unsigned py_function_impl_base::max_arity() const
{
    return this->min_arity();
}

extern PyTypeObject function_type;

function::function(
    py_function const& implementation
#if BOOST_WORKAROUND(__EDG_VERSION__, == 245)
    , python::detail::keyword const*       names_and_defaults
#else
    , python::detail::keyword const* const names_and_defaults
#endif
    , unsigned num_keywords
    )
    : m_fn(implementation)
    , m_nkeyword_values(0)
{
    if (names_and_defaults != 0)
    {
        unsigned int max_arity = m_fn.max_arity();
        unsigned int keyword_offset
            = max_arity > num_keywords ? max_arity - num_keywords : 0;


        ssize_t tuple_size = num_keywords ? max_arity : 0;
        m_arg_names = object(handle<>(PyTuple_New(tuple_size)));

        if (num_keywords != 0)
        {
            for (unsigned j = 0; j < keyword_offset; ++j)
                PyTuple_SET_ITEM(m_arg_names.ptr(), j, incref(Py_None));
        }
        
        for (unsigned i = 0; i < num_keywords; ++i)
        {
            tuple kv;

            python::detail::keyword const* const p = names_and_defaults + i;
            if (p->default_value)
            {
                kv = make_tuple(p->name, p->default_value);
                ++m_nkeyword_values;
            }
            else
            {
                kv = make_tuple(p->name);
            }

            PyTuple_SET_ITEM(
                m_arg_names.ptr()
                , i + keyword_offset
                , incref(kv.ptr())
                );
        }
    }
    
    PyObject* p = this;
    if (Py_TYPE(&function_type) == 0)
    {
        Py_TYPE(&function_type) = &PyType_Type;
        ::PyType_Ready(&function_type);
    }
    
    (void)(     // warning suppression for GCC
        PyObject_INIT(p, &function_type)
    );
}

function::~function()
{
}

PyObject* function::call(PyObject* args, PyObject* keywords) const
{
    std::size_t n_unnamed_actual = PyTuple_GET_SIZE(args);
    std::size_t n_keyword_actual = keywords ? PyDict_Size(keywords) : 0;
    std::size_t n_actual = n_unnamed_actual + n_keyword_actual;
    
    function const* f = this;

    // Try overloads looking for a match
    do
    {
        // Check for a plausible number of arguments
        unsigned min_arity = f->m_fn.min_arity();
        unsigned max_arity = f->m_fn.max_arity();

        if (n_actual + f->m_nkeyword_values >= min_arity
            && n_actual <= max_arity)
        {
            // This will be the args that actually get passed
            handle<>inner_args(allow_null(borrowed(args)));

            if (n_keyword_actual > 0      // Keyword arguments were supplied
                 || n_actual < min_arity) // or default keyword values are needed
            {                            
                if (f->m_arg_names.is_none())
                {
                    // this overload doesn't accept keywords
                    inner_args = handle<>();
                }
                else
                {
                    // "all keywords are none" is a special case
                    // indicating we will accept any number of keyword
                    // arguments
                    if (PyTuple_Size(f->m_arg_names.ptr()) == 0)
                    {
                        // no argument preprocessing
                    }
                    else if (n_actual > max_arity)
                    {
                        // too many arguments
                        inner_args = handle<>();
                    }
                    else
                    {
                        // build a new arg tuple, will adjust its size later
                        assert(max_arity <= static_cast<std::size_t>(ssize_t_max));
                        inner_args = handle<>(
                            PyTuple_New(static_cast<ssize_t>(max_arity)));

                        // Fill in the positional arguments
                        for (std::size_t i = 0; i < n_unnamed_actual; ++i)
                            PyTuple_SET_ITEM(inner_args.get(), i, incref(PyTuple_GET_ITEM(args, i)));

                        // Grab remaining arguments by name from the keyword dictionary
                        std::size_t n_actual_processed = n_unnamed_actual;
                
                        for (std::size_t arg_pos = n_unnamed_actual; arg_pos < max_arity ; ++arg_pos)
                        {
                            // Get the keyword[, value pair] corresponding
                            PyObject* kv = PyTuple_GET_ITEM(f->m_arg_names.ptr(), arg_pos);

                            // If there were any keyword arguments,
                            // look up the one we need for this
                            // argument position
                            PyObject* value = n_keyword_actual
                                ? PyDict_GetItem(keywords, PyTuple_GET_ITEM(kv, 0))
                                : 0;

                            if (!value)
                            {
                                // Not found; check if there's a default value
                                if (PyTuple_GET_SIZE(kv) > 1)
                                    value = PyTuple_GET_ITEM(kv, 1);
                        
                                if (!value)
                                {
                                    // still not found; matching fails
                                    PyErr_Clear();
                                    inner_args = handle<>();
                                    break;
                                }
                            }
                            else
                            {
                                ++n_actual_processed;
                            }

                            PyTuple_SET_ITEM(inner_args.get(), arg_pos, incref(value));
                        }

                        if (inner_args.get())
                        {
                            //check if we proccessed all the arguments
                            if(n_actual_processed < n_actual)
                                inner_args = handle<>();
                        }
                    }
                }
            }
            
            // Call the function.  Pass keywords in case it's a
            // function accepting any number of keywords
            PyObject* result = inner_args ? f->m_fn(inner_args.get(), keywords) : 0;
            
            // If the result is NULL but no error was set, m_fn failed
            // the argument-matching test.

            // This assumes that all other error-reporters are
            // well-behaved and never return NULL to python without
            // setting an error.
            if (result != 0 || PyErr_Occurred())
                return result;
        }
        f = f->m_overloads.get();
    }
    while (f);
    // None of the overloads matched; time to generate the error message
    argument_error(args, keywords);
    return 0;
}

object function::signature(bool show_return_type) const
{
    py_function const& impl = m_fn;
    
    python::detail::signature_element const* return_type = impl.signature();
    python::detail::signature_element const* s = return_type + 1;
    
    list formal_params;
    if (impl.max_arity() == 0)
        formal_params.append("void");

    for (unsigned n = 0; n < impl.max_arity(); ++n)
    {
        if (s[n].basename == 0)
        {
            formal_params.append("...");
            break;
        }

        str param(s[n].basename);
        if (s[n].lvalue)
            param += " {lvalue}";
        
        if (m_arg_names) // None or empty tuple will test false
        {
            object kv(m_arg_names[n]);
            if (kv)
            {
                char const* const fmt = len(kv) > 1 ? " %s=%r" : " %s";
                param += fmt % kv;
            }
        }
        
        formal_params.append(param);
    }

    if (show_return_type)
        return "%s(%s) -> %s" % make_tuple(
            m_name, str(", ").join(formal_params), return_type->basename);
    return "%s(%s)" % make_tuple(
        m_name, str(", ").join(formal_params));
}

object function::signatures(bool show_return_type) const
{
    list result;
    for (function const* f = this; f; f = f->m_overloads.get()) {
        result.append(f->signature(show_return_type));
    }
    return result;
}

void function::argument_error(PyObject* args, PyObject* /*keywords*/) const
{
    static handle<> exception(
        PyErr_NewException(const_cast<char*>("Boost.Python.ArgumentError"), PyExc_TypeError, 0));

    object message = "Python argument types in\n    %s.%s("
        % make_tuple(this->m_namespace, this->m_name);
    
    list actual_args;
    for (ssize_t i = 0; i < PyTuple_Size(args); ++i)
    {
        char const* name = PyTuple_GetItem(args, i)->ob_type->tp_name;
        actual_args.append(str(name));
    }
    message += str(", ").join(actual_args);
    message += ")\ndid not match C++ signature:\n    ";
    message += str("\n    ").join(signatures());

#if BOOST_PYTHON_DEBUG_ERROR_MESSAGES
    std::printf("\n--------\n%s\n--------\n", extract<const char*>(message)());
#endif 
    PyErr_SetObject(exception.get(), message.ptr());
    throw_error_already_set();
}

void function::add_overload(handle<function> const& overload_)
{
    function* parent = this;
    
    while (parent->m_overloads)
        parent = parent->m_overloads.get();

    parent->m_overloads = overload_;

    // If we have no documentation, get the docs from the overload
    if (!m_doc)
        m_doc = overload_->m_doc;
}

namespace
{
  char const* const binary_operator_names[] =
  {
      "add__",
      "and__",
      "div__",
      "divmod__",
      "eq__",
      "floordiv__",
      "ge__",
      "gt__",
      "le__",
      "lshift__",
      "lt__",
      "mod__",
      "mul__",
      "ne__",
      "or__",
      "pow__",
      "radd__",
      "rand__",
      "rdiv__",
      "rdivmod__", 
      "rfloordiv__",
      "rlshift__",
      "rmod__",
      "rmul__",
      "ror__",
      "rpow__", 
      "rrshift__",
      "rshift__",
      "rsub__",
      "rtruediv__",
      "rxor__",
      "sub__",
      "truediv__", 
      "xor__"
  };

  struct less_cstring
  {
      bool operator()(char const* x, char const* y) const
      {
          return BOOST_CSTD_::strcmp(x,y) < 0;
      }
  };
  
  inline bool is_binary_operator(char const* name)
  {
      return name[0] == '_'
          && name[1] == '_'
          && std::binary_search(
              &binary_operator_names[0]
              , binary_operator_names + sizeof(binary_operator_names)/sizeof(*binary_operator_names)
              , name + 2
              , less_cstring()
              );
  }

  // Something for the end of the chain of binary operators
  PyObject* not_implemented(PyObject*, PyObject*)
  {
      Py_INCREF(Py_NotImplemented);
      return Py_NotImplemented;
  }
  
  handle<function> not_implemented_function()
  {
      
      static object keeper(
          function_object(
              py_function(&not_implemented, mpl::vector1<void>(), 2)
            , python::detail::keyword_range())
          );
      return handle<function>(borrowed(downcast<function>(keeper.ptr())));
  }
}

void function::add_to_namespace(
    object const& name_space, char const* name_, object const& attribute)
{
    add_to_namespace(name_space, name_, attribute, 0);
}

namespace detail
{
    extern char py_signature_tag[];
    extern char cpp_signature_tag[];
}

void function::add_to_namespace(
    object const& name_space, char const* name_, object const& attribute, char const* doc)
{
    str const name(name_);
    PyObject* const ns = name_space.ptr();
    
    if (attribute.ptr()->ob_type == &function_type)
    {
        function* new_func = downcast<function>(attribute.ptr());
        handle<> dict;
        
#if PY_VERSION_HEX < 0x03000000
        // Old-style class gone in Python 3
        if (PyClass_Check(ns))
            dict = handle<>(borrowed(((PyClassObject*)ns)->cl_dict));
        else
#endif        
        if (PyType_Check(ns))
            dict = handle<>(borrowed(((PyTypeObject*)ns)->tp_dict));
        else    
            dict = handle<>(PyObject_GetAttrString(ns, const_cast<char*>("__dict__")));

        if (dict == 0)
            throw_error_already_set();

        handle<> existing(allow_null(::PyObject_GetItem(dict.get(), name.ptr())));
        
        if (existing)
        {
            if (existing->ob_type == &function_type)
            {
                new_func->add_overload(
                    handle<function>(
                        borrowed(
                            downcast<function>(existing.get())
                        )
                    )
                );
            }
            else if (existing->ob_type == &PyStaticMethod_Type)
            {
                char const* name_space_name = extract<char const*>(name_space.attr("__name__"));
                
                ::PyErr_Format(
                    PyExc_RuntimeError
                    , "Boost.Python - All overloads must be exported "
                      "before calling \'class_<...>(\"%s\").staticmethod(\"%s\")\'"
                    , name_space_name
                    , name_
                    );
                throw_error_already_set();
            }
        }
        else if (is_binary_operator(name_))
        {
            // Binary operators need an additional overload which
            // returns NotImplemented, so that Python will try the
            // __rxxx__ functions on the other operand. We add this
            // when no overloads for the operator already exist.
            new_func->add_overload(not_implemented_function());
        }

        // A function is named the first time it is added to a namespace.
        if (new_func->name().is_none())
            new_func->m_name = name;

        handle<> name_space_name(
            allow_null(::PyObject_GetAttrString(name_space.ptr(), const_cast<char*>("__name__"))));
        
        if (name_space_name)
            new_func->m_namespace = object(name_space_name);
    }

    // The PyObject_GetAttrString() or PyObject_GetItem calls above may
    // have left an active error
    PyErr_Clear();
    if (PyObject_SetAttr(ns, name.ptr(), attribute.ptr()) < 0)
        throw_error_already_set();

    object mutable_attribute(attribute);
/*
    if (doc != 0 && docstring_options::show_user_defined_)
    {
        // Accumulate documentation
        
        if (
            PyObject_HasAttrString(mutable_attribute.ptr(), "__doc__")
            && mutable_attribute.attr("__doc__"))
        {
            mutable_attribute.attr("__doc__") += "\n\n";
            mutable_attribute.attr("__doc__") += doc;
        }
        else {
            mutable_attribute.attr("__doc__") = doc;
        }
    }

    if (docstring_options::show_signatures_)
    {
        if (   PyObject_HasAttrString(mutable_attribute.ptr(), "__doc__")
            && mutable_attribute.attr("__doc__")) {
            mutable_attribute.attr("__doc__") += (
              mutable_attribute.attr("__doc__")[-1] != "\n" ? "\n\n" : "\n");
        }
        else {
            mutable_attribute.attr("__doc__") = "";
        }
        function* f = downcast<function>(attribute.ptr());
        mutable_attribute.attr("__doc__") += str("\n    ").join(make_tuple(
          "C++ signature:", f->signature(true)));
    }
    */
    str _doc;

    if (docstring_options::show_py_signatures_)
    {
        _doc += str(const_cast<const char*>(detail::py_signature_tag));
    }
    if (doc != 0 && docstring_options::show_user_defined_)
        _doc += doc;

    if (docstring_options::show_cpp_signatures_)
    {
        _doc += str(const_cast<const char*>(detail::cpp_signature_tag));
    }
    if(_doc)
    {    
        object mutable_attribute(attribute);
        mutable_attribute.attr("__doc__")= _doc;
    }
}

BOOST_PYTHON_DECL void add_to_namespace(
    object const& name_space, char const* name, object const& attribute)
{
    function::add_to_namespace(name_space, name, attribute, 0);
}

BOOST_PYTHON_DECL void add_to_namespace(
    object const& name_space, char const* name, object const& attribute, char const* doc)
{
    function::add_to_namespace(name_space, name, attribute, doc);
}


namespace
{
  struct bind_return
  {
      bind_return(PyObject*& result, function const* f, PyObject* args, PyObject* keywords)
          : m_result(result)
            , m_f(f)
            , m_args(args)
            , m_keywords(keywords)
      {}

      void operator()() const
      {
          m_result = m_f->call(m_args, m_keywords);
      }
      
   private:
      PyObject*& m_result;
      function const* m_f;
      PyObject* m_args;
      PyObject* m_keywords;
  };
}

extern "C"
{
    // Stolen from Python's funcobject.c
    static PyObject *
    function_descr_get(PyObject *func, PyObject *obj, PyObject *type_)
    {
#if PY_VERSION_HEX >= 0x03000000
        // The implement is different in Python 3 because of the removal of unbound method
        if (obj == Py_None || obj == NULL) {
            Py_INCREF(func);
            return func;
        }
        return PyMethod_New(func, obj);
#else
        if (obj == Py_None)
            obj = NULL;
        return PyMethod_New(func, obj, type_);
#endif
    }

    static void
    function_dealloc(PyObject* p)
    {
        delete static_cast<function*>(p);
    }

    static PyObject *
    function_call(PyObject *func, PyObject *args, PyObject *kw)
    {
        PyObject* result = 0;
        handle_exception(bind_return(result, static_cast<function*>(func), args, kw));
        return result;
    }

    //
    // Here we're using the function's tp_getset rather than its
    // tp_members to set up __doc__ and __name__, because tp_members
    // really depends on having a POD object type (it relies on
    // offsets). It might make sense to reformulate function as a POD
    // at some point, but this is much more expedient.
    //
    static PyObject* function_get_doc(PyObject* op, void*)
    {
        function* f = downcast<function>(op);
        list signatures = function_doc_signature_generator::function_doc_signatures(f);
        if(!signatures) return python::detail::none();
        signatures.reverse();
        return python::incref( str("\n").join(signatures).ptr());
    }
    
    static int function_set_doc(PyObject* op, PyObject* doc, void*)
    {
        function* f = downcast<function>(op);
        f->doc(doc ? object(python::detail::borrowed_reference(doc)) : object());
        return 0;
    }
    
    static PyObject* function_get_name(PyObject* op, void*)
    {
        function* f = downcast<function>(op);
        if (f->name().is_none())
#if PY_VERSION_HEX >= 0x03000000
            return PyUnicode_InternFromString("<unnamed Boost.Python function>");
#else
            return PyString_InternFromString("<unnamed Boost.Python function>");
#endif
        else
            return python::incref(f->name().ptr());
    }

    // We add a dummy __class__ attribute in order to fool PyDoc into
    // treating these as built-in functions and scanning their
    // documentation
    static PyObject* function_get_class(PyObject* /*op*/, void*)
    {
        return python::incref(upcast<PyObject>(&PyCFunction_Type));
    }

    static PyObject* function_get_module(PyObject* op, void*)
    {
        function* f = downcast<function>(op);
        object const& ns = f->get_namespace();
        if (!ns.is_none()) {
            return python::incref(ns.ptr());
        }
        PyErr_SetString(
            PyExc_AttributeError, const_cast<char*>(
                "Boost.Python function __module__ unknown."));
        return 0;
    }
}

static PyGetSetDef function_getsetlist[] = {
    {const_cast<char*>("__name__"), (getter)function_get_name, 0, 0, 0 },
    {const_cast<char*>("func_name"), (getter)function_get_name, 0, 0, 0 },
    {const_cast<char*>("__module__"), (getter)function_get_module, 0, 0, 0 },
    {const_cast<char*>("func_module"), (getter)function_get_module, 0, 0, 0 },
    {const_cast<char*>("__class__"), (getter)function_get_class, 0, 0, 0 },    // see note above
    {const_cast<char*>("__doc__"), (getter)function_get_doc, (setter)function_set_doc, 0, 0},
    {const_cast<char*>("func_doc"), (getter)function_get_doc, (setter)function_set_doc, 0, 0},
    {NULL, 0, 0, 0, 0} /* Sentinel */
};

PyTypeObject function_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    const_cast<char*>("Boost.Python.function"),
    sizeof(function),
    0,
    (destructor)function_dealloc,               /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0, //(reprfunc)func_repr,                   /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    function_call,                              /* tp_call */
    0,                                  /* tp_str */
    0, // PyObject_GenericGetAttr,            /* tp_getattro */
    0, // PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT /* | Py_TPFLAGS_HAVE_GC */,/* tp_flags */
    0,                                  /* tp_doc */
    0, // (traverseproc)func_traverse,          /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0, //offsetof(PyFunctionObject, func_weakreflist), /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0, // func_memberlist,              /* tp_members */
    function_getsetlist,                /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    function_descr_get,                 /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0, //offsetof(PyFunctionObject, func_dict),      /* tp_dictoffset */
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

object function_object(
    py_function const& f
    , python::detail::keyword_range const& keywords)
{
    return python::object(
        python::detail::new_non_null_reference(
            new function(
                f, keywords.first, keywords.second - keywords.first)));
}

object function_object(py_function const& f)
{
    return function_object(f, python::detail::keyword_range());
}


handle<> function_handle_impl(py_function const& f)
{
    return python::handle<>(
        allow_null(
            new function(f, 0, 0)));
}

} // namespace objects

namespace detail
{
  object BOOST_PYTHON_DECL make_raw_function(objects::py_function f)
  {
      static keyword k;
    
      return objects::function_object(
          f
          , keyword_range(&k,&k));
  }
  void BOOST_PYTHON_DECL pure_virtual_called()
  {
      PyErr_SetString(
          PyExc_RuntimeError, const_cast<char*>("Pure virtual function called"));
      throw_error_already_set();
  }
}

}} // namespace boost::python
