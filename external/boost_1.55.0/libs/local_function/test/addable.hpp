
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef ADDABLE_HPP_
#define ADDABLE_HPP_

#include <boost/concept_check.hpp>

template<typename T>
struct Addable { // User-defined concept.
    BOOST_CONCEPT_USAGE(Addable) {
        return_type(x + y); // Check addition `T operator+(T x, T y)`.
    }

private:
    void return_type(T) {} // Implementation (required for some linkers).
    static T const& x;
    static T const& y;
};

#endif // #include guard

