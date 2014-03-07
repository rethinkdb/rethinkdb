// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef TO_PYTHON_VALUE_DWA200221_HPP
# define TO_PYTHON_VALUE_DWA200221_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/refcount.hpp>
# include <boost/python/tag.hpp>
# include <boost/python/handle.hpp>

# include <boost/python/converter/registry.hpp>
# include <boost/python/converter/registered.hpp>
# include <boost/python/converter/builtin_converters.hpp>
# include <boost/python/converter/object_manager.hpp>
# include <boost/python/converter/shared_ptr_to_python.hpp>

# include <boost/python/detail/value_is_shared_ptr.hpp>
# include <boost/python/detail/value_arg.hpp>

# include <boost/type_traits/transform_traits.hpp>

# include <boost/mpl/if.hpp>
# include <boost/mpl/or.hpp>
# include <boost/type_traits/is_const.hpp>

namespace boost { namespace python { 

namespace detail
{
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES

template <bool is_const_ref>
struct object_manager_get_pytype
{
   template <class U>
   static PyTypeObject const* get( U& (*)() =0)
   {
      return converter::object_manager_traits<U>::get_pytype();
   }
};

template <>
struct object_manager_get_pytype<true>
{
   template <class U>
   static PyTypeObject const* get( U const& (*)() =0)
   {
      return converter::object_manager_traits<U>::get_pytype();
   }
};

#endif

  template <class T>
  struct object_manager_to_python_value
  {
      typedef typename value_arg<T>::type argument_type;
    
      PyObject* operator()(argument_type) const;
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
      typedef boost::mpl::bool_<is_handle<T>::value> is_t_handle;
      typedef boost::detail::indirect_traits::is_reference_to_const<T> is_t_const;
      PyTypeObject const* get_pytype() const {
          return get_pytype_aux((is_t_handle*)0);
      }

      inline static PyTypeObject const* get_pytype_aux(mpl::true_*) {return converter::object_manager_traits<T>::get_pytype();}
      
      inline static PyTypeObject const* get_pytype_aux(mpl::false_* ) 
      {
          return object_manager_get_pytype<is_t_const::value>::get((T(*)())0);
      }
      
#endif 

      // This information helps make_getter() decide whether to try to
      // return an internal reference or not. I don't like it much,
      // but it will have to serve for now.
      BOOST_STATIC_CONSTANT(bool, uses_registry = false);
  };

  
  template <class T>
  struct registry_to_python_value
  {
      typedef typename value_arg<T>::type argument_type;
    
      PyObject* operator()(argument_type) const;
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
      PyTypeObject const* get_pytype() const {return converter::registered<T>::converters.to_python_target_type();}
#endif

      // This information helps make_getter() decide whether to try to
      // return an internal reference or not. I don't like it much,
      // but it will have to serve for now.
      BOOST_STATIC_CONSTANT(bool, uses_registry = true);
  };

  template <class T>
  struct shared_ptr_to_python_value
  {
      typedef typename value_arg<T>::type argument_type;
    
      PyObject* operator()(argument_type) const;
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
      PyTypeObject const* get_pytype() const {return get_pytype((boost::type<argument_type>*)0);}
#endif 
      // This information helps make_getter() decide whether to try to
      // return an internal reference or not. I don't like it much,
      // but it will have to serve for now.
      BOOST_STATIC_CONSTANT(bool, uses_registry = false);
  private:
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
      template <class U>
      PyTypeObject const* get_pytype(boost::type<shared_ptr<U> &> *) const {return converter::registered<U>::converters.to_python_target_type();}
      template <class U>
      PyTypeObject const* get_pytype(boost::type<const shared_ptr<U> &> *) const {return converter::registered<U>::converters.to_python_target_type();}
#endif
  };
}

template <class T>
struct to_python_value
    : mpl::if_<
          detail::value_is_shared_ptr<T>
        , detail::shared_ptr_to_python_value<T>
        , typename mpl::if_<
              mpl::or_<
                  converter::is_object_manager<T>
                , converter::is_reference_to_object_manager<T>
              >
            , detail::object_manager_to_python_value<T>
            , detail::registry_to_python_value<T>
          >::type
      >::type
{
};

//
// implementation 
//
namespace detail
{
  template <class T>
  inline PyObject* registry_to_python_value<T>::operator()(argument_type x) const
  {
      typedef converter::registered<argument_type> r;
# if BOOST_WORKAROUND(__GNUC__, < 3)
      // suppresses an ICE, somehow
      (void)r::converters;
# endif 
      return converter::registered<argument_type>::converters.to_python(&x);
  }

  template <class T>
  inline PyObject* object_manager_to_python_value<T>::operator()(argument_type x) const
  {
      return python::upcast<PyObject>(
          python::xincref(
              get_managed_object(x, tag))
          );
  }

  template <class T>
  inline PyObject* shared_ptr_to_python_value<T>::operator()(argument_type x) const
  {
      return converter::shared_ptr_to_python(x);
  }
}

}} // namespace boost::python

#endif // TO_PYTHON_VALUE_DWA200221_HPP
