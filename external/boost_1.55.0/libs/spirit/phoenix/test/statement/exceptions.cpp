/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <stdexcept>
#include <string>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include <boost/detail/lightweight_test.hpp>

int main()
{
    using namespace boost::phoenix;
    using namespace boost::phoenix::arg_names;
    using namespace std;

    {
        try
        {
            throw_(runtime_error("error"))();
            BOOST_ERROR("exception should have been thrown");
        }
        catch(runtime_error& err)
        {
            BOOST_TEST(err.what() == string("error"));
        }
    }

    {
        try
        {
            try
            {
                throw runtime_error("error");
            }
            catch(exception&)
            {
                throw_()();
                BOOST_ERROR("exception should have been rethrown");
            }
        }
        catch(exception& err)
        {
            BOOST_TEST(err.what() == string("error"));
        }
    }

    {
        bool caught_exception = false;

        try_
        [ throw_(runtime_error("error")) ]
        .catch_<exception>()
        [ ref(caught_exception) = true ]();

        BOOST_TEST(caught_exception);
    }

    {
        bool caught_exception = false;
        try_
        [ throw_(runtime_error("error")) ]
        .catch_all
        [ ref(caught_exception) = true ]();
        BOOST_TEST(caught_exception);
    }

    {
        bool caught_correct_exception = false;
        try_
            [ throw_(runtime_error("error")) ]
        .catch_<string>()
            [ ref(caught_correct_exception) = false ]
        .catch_<exception>()
            [ ref(caught_correct_exception) = true]();

        BOOST_TEST(caught_correct_exception);
    }

    {
        bool caught_correct_exception = false;
        try_
            [ throw_(runtime_error("error")) ]
        .catch_<string>()
            [ ref(caught_correct_exception) = false ]
        .catch_all
            [ ref(caught_correct_exception) = true]();

        BOOST_TEST(caught_correct_exception);
    }

    return boost::report_errors();
}
