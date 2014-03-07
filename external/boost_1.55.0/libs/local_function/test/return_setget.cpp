
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
#include <boost/function.hpp>
#include <boost/typeof/std/string.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <string>

boost::function<void (const std::string&)> set;
boost::function<const std::string& (void)> get;

void action(void) {
    // State `message` hidden behind access functions from here.
    BOOST_TEST(get() == "abc");
    set("xyz");
    BOOST_TEST(get() == "xyz");
}

int main(void) {
    std::string message = "abc"; // Reference valid where closure used.
    
    void BOOST_LOCAL_FUNCTION(bind& message, const std::string& text) {
        message = text;
    } BOOST_LOCAL_FUNCTION_NAME(s)
    set = s;

    const std::string& BOOST_LOCAL_FUNCTION(const bind& message) {
        return message;
    } BOOST_LOCAL_FUNCTION_NAME(g)
    get = g;
    
    action();
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

