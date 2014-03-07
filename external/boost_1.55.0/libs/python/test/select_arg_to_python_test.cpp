// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/converter/arg_to_python.hpp>
#include <boost/python/type_id.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/object.hpp>
#include <iostream>

// gcc 2.95.x and MIPSpro 7.3.1.3 linker seem to demand this definition
#if ((defined(__GNUC__) && __GNUC__ < 3)) \
 || (defined(__sgi) && defined(__EDG_VERSION__) && (__EDG_VERSION__ == 238))
namespace boost { namespace python {
BOOST_PYTHON_DECL bool handle_exception_impl(function0<void>)
{
    return true;
}
}}
#endif

int result;

#define ASSERT_SAME(T1,T2) assert_same< T1,T2 >()

template <class T, class U>
void assert_same(U* = 0, T* = 0)
{
    BOOST_STATIC_ASSERT((boost::is_same<T,U>::value));
    
}


int main()
{
    using namespace boost::python::converter::detail;
    using namespace boost::python::converter;
    using namespace boost::python;
    using namespace boost;


    ASSERT_SAME(
        select_arg_to_python<int>::type, value_arg_to_python<int>
        );

    ASSERT_SAME(
        select_arg_to_python<reference_wrapper<int> >::type, reference_arg_to_python<int>
        );
    
    ASSERT_SAME(
        select_arg_to_python<pointer_wrapper<int> >::type, pointer_shallow_arg_to_python<int>
        );
    
    ASSERT_SAME(
        select_arg_to_python<int*>::type, pointer_deep_arg_to_python<int*>
        );
    
    ASSERT_SAME(
        select_arg_to_python<handle<> >::type, object_manager_arg_to_python<handle<> >
        );

    ASSERT_SAME(
        select_arg_to_python<object>::type, object_manager_arg_to_python<object>
        );

    ASSERT_SAME(
        select_arg_to_python<char[20]>::type, arg_to_python<char const*>
        );

    return result;
}
