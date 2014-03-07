// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/converter/object_manager.hpp>
#include <boost/python/borrowed.hpp>
#include <boost/static_assert.hpp>
#include <boost/python/handle.hpp>

using namespace boost::python;
using namespace boost::python::converter;

struct X {};

int main()
{
    BOOST_STATIC_ASSERT(is_object_manager<handle<> >::value);
    BOOST_STATIC_ASSERT(!is_object_manager<int>::value);
    BOOST_STATIC_ASSERT(!is_object_manager<X>::value);
    
    BOOST_STATIC_ASSERT(is_reference_to_object_manager<handle<>&>::value);
    BOOST_STATIC_ASSERT(is_reference_to_object_manager<handle<> const&>::value);
    BOOST_STATIC_ASSERT(is_reference_to_object_manager<handle<> volatile&>::value);
    BOOST_STATIC_ASSERT(is_reference_to_object_manager<handle<> const volatile&>::value);

    BOOST_STATIC_ASSERT(!is_reference_to_object_manager<handle<> >::value);
    BOOST_STATIC_ASSERT(!is_reference_to_object_manager<X>::value);
    BOOST_STATIC_ASSERT(!is_reference_to_object_manager<X&>::value);
    BOOST_STATIC_ASSERT(!is_reference_to_object_manager<X const&>::value);
    
    return 0;
}

