// Copyright 2013 Antony Polukhin

// Distributed under the Boost Software License, Version 1.0.
// (See the accompanying file LICENSE_1_0.txt
// or a copy at <http://www.boost.org/LICENSE_1_0.txt>.)


//[lexical_cast_variant_to_long_double
/*`
    In this example we'll make a `to_long_double` method that converts value of the Boost.Variant to `long double`.
*/

#include <boost/lexical_cast.hpp>
#include <boost/variant.hpp>
#include <cassert>

struct to_long_double_functor: boost::static_visitor<long double> {
    template <class T>
    long double operator()(const T& v) const {
        // Lexical cast has many optimizations including optimizations for situations that usually 
        // occur in generic programming, like std::string to std::string or arithmetic type to arithmetic type conversion. 
        return boost::lexical_cast<long double>(v);
    }
};

// Throws `boost::bad_lexical_cast` if value of the variant is not convertible to `long double`
template <class Variant>
long double to_long_double(const Variant& v) {
    return boost::apply_visitor(to_long_double_functor(), v);
}

int main() {
    boost::variant<char, int, std::string> v1('0'), v2("10.0001"), v3(1);

    long double sum = to_long_double(v1) + to_long_double(v2) + to_long_double(v3);
    assert(sum > 11 && sum < 11.1);
}

//] [/lexical_cast_variant_to_long_double]
