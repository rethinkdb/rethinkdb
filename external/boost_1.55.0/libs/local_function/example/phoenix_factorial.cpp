
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/detail/lightweight_test.hpp>

//[phoenix_factorial
struct factorial_impl { // Phoenix function from global functor.
    template<typename Sig>
    struct result;

    template<typename This, typename Arg>
    struct result<This (Arg)> : result<This (Arg const&)> {};

    template<typename This, typename Arg>
    struct result<This (Arg&)> { typedef Arg type; };

    template<typename Arg> // Polymorphic.
    Arg operator()(Arg n) const {
        return (n <= 0) ? 1 : n * (*this)(n - 1);
    }
};

int main(void) {
    using boost::phoenix::arg_names::arg1;
    
    boost::phoenix::function<factorial_impl> factorial;
    
    int i = 4;
    BOOST_TEST(factorial(i)() == 24);      // Call.
    BOOST_TEST(factorial(arg1)(i) == 24);  // Lazy call.
    return boost::report_errors();
}
//]

