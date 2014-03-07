// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/object/class_metadata.hpp>
#include <boost/python/has_back_reference.hpp>
#include <boost/python/detail/not_specified.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/same_traits.hpp>
#include <boost/function/function0.hpp>
#include <boost/mpl/bool.hpp>
#include <memory>

struct BR {};

struct Base {};
struct Derived : Base {};

namespace boost { namespace python
{
  // specialization
  template <>
  struct has_back_reference<BR>
    : mpl::true_
  {
  };
}} // namespace boost::python

template <class T, class U>
void assert_same(U* = 0, T* = 0)
{
    BOOST_STATIC_ASSERT((boost::is_same<T,U>::value));
    
}

template <class T, class Held, class Holder>
void assert_holder(T* = 0, Held* = 0, Holder* = 0)
{
    using namespace boost::python::detail;
    using namespace boost::python::objects;
    
    typedef typename class_metadata<
       T,Held,not_specified,not_specified
           >::holder h;
    
    assert_same<Holder>(
        (h*)0
    );
}

int test_main(int, char * [])
{
    using namespace boost::python::detail;
    using namespace boost::python::objects;

    assert_holder<Base,not_specified,value_holder<Base> >();

    assert_holder<BR,not_specified,value_holder_back_reference<BR,BR> >();
    assert_holder<Base,Base,value_holder_back_reference<Base,Base> >();
    assert_holder<BR,BR,value_holder_back_reference<BR,BR> >();

    assert_holder<Base,Derived
        ,value_holder_back_reference<Base,Derived> >();

    assert_holder<Base,std::auto_ptr<Base>
        ,pointer_holder<std::auto_ptr<Base>,Base> >();
    
    assert_holder<Base,std::auto_ptr<Derived>
        ,pointer_holder_back_reference<std::auto_ptr<Derived>,Base> >();

    assert_holder<BR,std::auto_ptr<BR>
        ,pointer_holder_back_reference<std::auto_ptr<BR>,BR> > ();

    return 0;
}

