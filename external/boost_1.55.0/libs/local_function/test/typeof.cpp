
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   error "variadic macros required"
#else

#include "addable.hpp"
#include <boost/local_function.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/concept_check.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <algorithm>

int main(void) {
    //[typeof
    int sum = 0, factor = 10;

    void BOOST_LOCAL_FUNCTION(const bind factor, bind& sum, int num) {
        // Type-of for concept checking.
        BOOST_CONCEPT_ASSERT((Addable<boost::remove_reference<
                BOOST_LOCAL_FUNCTION_TYPEOF(sum)>::type>));
        // Type-of for declarations.
        boost::remove_reference<BOOST_LOCAL_FUNCTION_TYPEOF(
                factor)>::type mult = factor * num;
        sum += mult;
    } BOOST_LOCAL_FUNCTION_NAME(add)

    add(6);
    //]
    BOOST_TEST(sum == 60);
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

