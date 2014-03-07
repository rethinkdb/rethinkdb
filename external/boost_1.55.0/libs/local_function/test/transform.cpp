
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   error "variadic macros required"
#else

#include <boost/local_function.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <algorithm>
#include <vector>

int main(void) {
    //[transform
    int offset = 5;
    std::vector<int> v;
    std::vector<int> w;

    for(int i = 1; i <= 2; ++i) v.push_back(i * 10);
    BOOST_TEST(v[0] == 10); BOOST_TEST(v[1] == 20);
    w.resize(v.size());

    int BOOST_LOCAL_FUNCTION(const bind& offset, int i) {
        return ++i + offset;
    } BOOST_LOCAL_FUNCTION_NAME(inc)
    
    std::transform(v.begin(), v.end(), w.begin(), inc);
    BOOST_TEST(w[0] == 16); BOOST_TEST(w[1] == 26);

    int BOOST_LOCAL_FUNCTION(bind& inc, int i, int j) {
        return inc(i + j); // Call the other bound local function.
    } BOOST_LOCAL_FUNCTION_NAME(inc_sum)
    
    offset = 0;
    std::transform(v.begin(), v.end(), w.begin(), v.begin(), inc_sum);
    BOOST_TEST(v[0] == 27); BOOST_TEST(v[1] == 47);
    //]
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

