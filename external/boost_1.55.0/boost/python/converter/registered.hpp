// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef REGISTERED_DWA2002710_HPP
# define REGISTERED_DWA2002710_HPP
# include <boost/python/type_id.hpp>
# include <boost/python/converter/registry.hpp>
# include <boost/python/converter/registrations.hpp>
# include <boost/type_traits/transform_traits.hpp>
# include <boost/type_traits/cv_traits.hpp>
# include <boost/type_traits/is_void.hpp>
# include <boost/detail/workaround.hpp>
# include <boost/python/type_id.hpp>
# include <boost/type.hpp>

namespace boost {

// You'll see shared_ptr mentioned in this header because we need to
// note which types are shared_ptrs in their registrations, to
// implement special shared_ptr handling for rvalue conversions.
template <class T> class shared_ptr;

namespace python { namespace converter { 

struct registration;

namespace detail
{
  template <class T>
  struct registered_base
  {
      static registration const& converters;
  };
}

template <class T>
struct registered
  : detail::registered_base<
        typename add_reference<
            typename add_cv<T>::type
        >::type
    >
{
};

# if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) \
    && !BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
// collapses a few more types to the same static instance.  MSVC7.1
// fails to strip cv-qualification from array types in typeid.  For
// some reason we can't use this collapse there or array converters
// will not be found.
template <class T>
struct registered<T&>
  : registered<T> {};
# endif

//
// implementations
//
namespace detail
{
  inline void
  register_shared_ptr0(...)
  {
  }
  
  template <class T>
  inline void
  register_shared_ptr0(shared_ptr<T>*)
  {
      registry::lookup_shared_ptr(type_id<shared_ptr<T> >());
  }
  
  template <class T>
  inline void
  register_shared_ptr1(T const volatile*)
  {
      detail::register_shared_ptr0((T*)0);
  }
  
  template <class T>
  inline registration const& 
  registry_lookup2(T&(*)())
  {
      detail::register_shared_ptr1((T*)0);
      return registry::lookup(type_id<T&>());
  }

  template <class T>
  inline registration const& 
  registry_lookup1(type<T>)
  {
      return registry_lookup2((T(*)())0);
  }

  inline registration const& 
  registry_lookup1(type<const volatile void>)
  {
      detail::register_shared_ptr1((void*)0);
      return registry::lookup(type_id<void>());
  }

  template <class T>
  registration const& registered_base<T>::converters = detail::registry_lookup1(type<T>());

}

}}} // namespace boost::python::converter

#endif // REGISTERED_DWA2002710_HPP
