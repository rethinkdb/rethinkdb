//  Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/converter/registry.hpp>
#include <boost/python/converter/registrations.hpp>
#include <boost/python/converter/builtin_converters.hpp>

#include <set>
#include <stdexcept>

#if defined(__APPLE__) && defined(__MACH__) && defined(__GNUC__) \
 && __GNUC__ == 3 && __GNUC_MINOR__ <= 4 && !defined(__APPLE_CC__)
# define BOOST_PYTHON_CONVERTER_REGISTRY_APPLE_MACH_WORKAROUND
#endif

#if defined(BOOST_PYTHON_TRACE_REGISTRY) \
 || defined(BOOST_PYTHON_CONVERTER_REGISTRY_APPLE_MACH_WORKAROUND)
# include <iostream>
#endif

namespace boost { namespace python { namespace converter { 
BOOST_PYTHON_DECL PyTypeObject const* registration::expected_from_python_type() const
{
    if (this->m_class_object != 0)
        return this->m_class_object;

    std::set<PyTypeObject const*> pool;

    for(rvalue_from_python_chain* r = rvalue_chain; r ; r=r->next)
        if(r->expected_pytype)
            pool.insert(r->expected_pytype());

    //for now I skip the search for common base
    if (pool.size()==1)
        return *pool.begin();

    return 0;

}

BOOST_PYTHON_DECL PyTypeObject const* registration::to_python_target_type() const
{
    if (this->m_class_object != 0)
        return this->m_class_object;

    if (this->m_to_python_target_type != 0)
        return this->m_to_python_target_type();

    return 0;
}

BOOST_PYTHON_DECL PyTypeObject* registration::get_class_object() const
{
    if (this->m_class_object == 0)
    {
        ::PyErr_Format(
            PyExc_TypeError
            , const_cast<char*>("No Python class registered for C++ class %s")
            , this->target_type.name());
    
        throw_error_already_set();
    }
    
    return this->m_class_object;
}
  
BOOST_PYTHON_DECL PyObject* registration::to_python(void const volatile* source) const
{
    if (this->m_to_python == 0)
    {
        handle<> msg(
#if PY_VERSION_HEX >= 0x3000000
            ::PyUnicode_FromFormat
#else
            ::PyString_FromFormat
#endif
            (
                "No to_python (by-value) converter found for C++ type: %s"
                , this->target_type.name()
                )
            );
            
        PyErr_SetObject(PyExc_TypeError, msg.get());

        throw_error_already_set();
    }
        
    return source == 0
        ? incref(Py_None)
        : this->m_to_python(const_cast<void*>(source));
}

namespace
{
  template< typename T >
  void delete_node( T* node )
  {
      if( !!node && !!node->next )
          delete_node( node->next );
      delete node;
  }
}

registration::~registration()
{
  delete_node(lvalue_chain);
  delete_node(rvalue_chain);
}


namespace // <unnamed>
{
  typedef registration entry;
  
  typedef std::set<entry> registry_t;
  
#ifndef BOOST_PYTHON_CONVERTER_REGISTRY_APPLE_MACH_WORKAROUND
  registry_t& entries()
  {
      static registry_t registry;

# ifndef BOOST_PYTHON_SUPPRESS_REGISTRY_INITIALIZATION
      static bool builtin_converters_initialized = false;
      if (!builtin_converters_initialized)
      {
          // Make this true early because registering the builtin
          // converters will cause recursion.
          builtin_converters_initialized = true;
          
          initialize_builtin_converters();
      }
#  ifdef BOOST_PYTHON_TRACE_REGISTRY
      std::cout << "registry: ";
      for (registry_t::iterator p = registry.begin(); p != registry.end(); ++p)
      {
          std::cout << p->target_type << "; ";
      }
      std::cout << '\n';
#  endif 
# endif 
      return registry;
  }
#else
  registry_t& static_registry()
  {
    static registry_t result;
    return result;
  }

  bool static_builtin_converters_initialized()
  {
    static bool result = false;
    if (result == false) {
      result = true;
      std::cout << std::flush;
      return false;
    }
    return true;
  }

  registry_t& entries()
  {
# ifndef BOOST_PYTHON_SUPPRESS_REGISTRY_INITIALIZATION
      if (!static_builtin_converters_initialized())
      {
          initialize_builtin_converters();
      }
#  ifdef BOOST_PYTHON_TRACE_REGISTRY
      std::cout << "registry: ";
      for (registry_t::iterator p = static_registry().begin(); p != static_registry().end(); ++p)
      {
          std::cout << p->target_type << "; ";
      }
      std::cout << '\n';
#  endif 
# endif 
      return static_registry();
  }
#endif // BOOST_PYTHON_CONVERTER_REGISTRY_APPLE_MACH_WORKAROUND

  entry* get(type_info type, bool is_shared_ptr = false)
  {
#  ifdef BOOST_PYTHON_TRACE_REGISTRY
      registry_t::iterator p = entries().find(entry(type));
      
      std::cout << "looking up " << type << ": "
                << (p == entries().end() || p->target_type != type
                    ? "...NOT found\n" : "...found\n");
#  endif
      std::pair<registry_t::const_iterator,bool> pos_ins
          = entries().insert(entry(type,is_shared_ptr));
      
#  if __MWERKS__ >= 0x3000
      // do a little invariant checking if a change was made
      if ( pos_ins.second )
          assert(entries().invariants());
#  endif
      return const_cast<entry*>(&*pos_ins.first);
  }
} // namespace <unnamed>

namespace registry
{
  void insert(to_python_function_t f, type_info source_t, PyTypeObject const* (*to_python_target_type)())
  {
#  ifdef BOOST_PYTHON_TRACE_REGISTRY
      std::cout << "inserting to_python " << source_t << "\n";
#  endif 
      entry* slot = get(source_t);
      
      assert(slot->m_to_python == 0); // we have a problem otherwise
      if (slot->m_to_python != 0)
      {
          std::string msg = (
              std::string("to-Python converter for ")
              + source_t.name()
              + " already registered; second conversion method ignored."
          );
          
          if ( ::PyErr_Warn( NULL, const_cast<char*>(msg.c_str()) ) )
          {
              throw_error_already_set();
          }
      }
      slot->m_to_python = f;
      slot->m_to_python_target_type = to_python_target_type;
  }

  // Insert an lvalue from_python converter
  void insert(convertible_function convert, type_info key, PyTypeObject const* (*exp_pytype)())
  {
#  ifdef BOOST_PYTHON_TRACE_REGISTRY
      std::cout << "inserting lvalue from_python " << key << "\n";
#  endif 
      entry* found = get(key);
      lvalue_from_python_chain *registration = new lvalue_from_python_chain;
      registration->convert = convert;
      registration->next = found->lvalue_chain;
      found->lvalue_chain = registration;
      
      insert(convert, 0, key,exp_pytype);
  }

  // Insert an rvalue from_python converter
  void insert(convertible_function convertible
              , constructor_function construct
              , type_info key
              , PyTypeObject const* (*exp_pytype)())
  {
#  ifdef BOOST_PYTHON_TRACE_REGISTRY
      std::cout << "inserting rvalue from_python " << key << "\n";
#  endif 
      entry* found = get(key);
      rvalue_from_python_chain *registration = new rvalue_from_python_chain;
      registration->convertible = convertible;
      registration->construct = construct;
      registration->expected_pytype = exp_pytype;
      registration->next = found->rvalue_chain;
      found->rvalue_chain = registration;
  }

  // Insert an rvalue from_python converter
  void push_back(convertible_function convertible
              , constructor_function construct
              , type_info key
              , PyTypeObject const* (*exp_pytype)())
  {
#  ifdef BOOST_PYTHON_TRACE_REGISTRY
      std::cout << "push_back rvalue from_python " << key << "\n";
#  endif 
      rvalue_from_python_chain** found = &get(key)->rvalue_chain;
      while (*found != 0)
          found = &(*found)->next;
      
      rvalue_from_python_chain *registration = new rvalue_from_python_chain;
      registration->convertible = convertible;
      registration->construct = construct;
      registration->expected_pytype = exp_pytype;
      registration->next = 0;
      *found = registration;
  }

  registration const& lookup(type_info key)
  {
      return *get(key);
  }

  registration const& lookup_shared_ptr(type_info key)
  {
      return *get(key, true);
  }

  registration const* query(type_info type)
  {
      registry_t::iterator p = entries().find(entry(type));
#  ifdef BOOST_PYTHON_TRACE_REGISTRY
      std::cout << "querying " << type
                << (p == entries().end() || p->target_type != type
                    ? "...NOT found\n" : "...found\n");
#  endif 
      return (p == entries().end() || p->target_type != type) ? 0 : &*p;
  }
} // namespace registry

}}} // namespace boost::python::converter
