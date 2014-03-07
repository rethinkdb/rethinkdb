/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <string>
#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/object.hpp>

using std::string;

struct T
{
    string foo() { return "T"; }
};

struct U : T
{
    string foo() { return "U"; }
};

struct VT
{
    virtual string foo() { return "T"; }
};

struct VU : VT
{
    virtual string foo() { return "U"; }
};

int
main()
{
    using boost::phoenix::arg_names::arg1;
    using boost::phoenix::const_cast_;
    using boost::phoenix::dynamic_cast_;
    using boost::phoenix::reinterpret_cast_;
    using boost::phoenix::static_cast_;

    {
        U u;
        BOOST_TEST(arg1(u).foo() == "U");
        BOOST_TEST(static_cast_<T&>(arg1)(u).foo() == "T");
    }

    {
        U const u = U();
        BOOST_TEST(const_cast_<U&>(arg1)(u).foo() == "U");
    }

    {
        VU u;
        VT* tp = &u;
        BOOST_TEST(arg1(tp)->foo() == "U");
        BOOST_TEST(dynamic_cast_<VU*>(arg1)(tp) != 0);
    }

    {
        void* p = 0;
        reinterpret_cast_<VU*>(arg1)(p); // compile test only
    }

    return boost::report_errors();
}
