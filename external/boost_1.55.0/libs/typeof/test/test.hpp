// Copyright (C) 2005 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_TEST_HPP_INCLUDED
#define BOOST_TYPEOF_TEST_HPP_INCLUDED

#include <boost/typeof/typeof.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/size_t.hpp>

namespace boost { namespace type_of {

    template<class T, class U>
    struct test_wrapper{};

    template<class T>
    T test_make(T*);

    template<class T>
    struct test
    {
        BOOST_STATIC_CONSTANT(std::size_t,value = (boost::is_same<
            BOOST_TYPEOF_TPL(test_make((test_wrapper<T, int>*)0)),
            test_wrapper<T, int>
            >::value)
        );
    };

}}

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::type_of::test_wrapper, 2)

#endif//BOOST_TYPEOF_TEST_HPP_INCLUDED
