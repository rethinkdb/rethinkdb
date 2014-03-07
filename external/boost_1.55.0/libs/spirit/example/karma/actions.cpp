/*=============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <sstream>

// Presented are various ways to attach semantic actions
//  * Using plain function pointer
//  * Using simple function object
//  * Using boost.bind
//  * Using boost.lambda

using boost::spirit::unused_type;

//[karma_tutorial_semantic_action_functions
namespace client
{
    namespace karma = boost::spirit::karma;

    // A plain function
    void read_function(int& i)
    {
        i = 42;
    }

    // A member function
    struct reader
    {
        void print(int& i) const
        {
            i = 42;
        }
    };

    // A function object
    struct read_action
    {
        void operator()(int& i, unused_type, unused_type) const
        {
            i = 42;
        }
    };
}
//]

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using boost::spirit::karma::int_;
    using boost::spirit::karma::generate;
    using client::read_function;
    using client::reader;
    using client::read_action;

    { // example using plain functions
        using namespace boost::spirit;

        std::string generated;
        std::back_insert_iterator<std::string> outiter(generated);

        //[karma_tutorial_attach_actions1
        generate(outiter, '{' << int_[&read_function] << '}');
        //]

        std::cout << "Simple function: " << generated << std::endl;
    }

    { // example using simple function object
        using namespace boost::spirit;

        std::string generated;
        std::back_insert_iterator<std::string> outiter(generated);

        //[karma_tutorial_attach_actions2
        generate(outiter, '{' << int_[read_action()] << '}');
        //]

        std::cout << "Simple function object: " << generated << std::endl;
    }

    { // example using plain function with boost.bind
        std::string generated;
        std::back_insert_iterator<std::string> outiter(generated);

        //[karma_tutorial_attach_actions3
        generate(outiter, '{' << int_[boost::bind(&read_function, _1)] << '}');
        //]

        std::cout << "Simple function with Boost.Bind: " << generated << std::endl;
    }

    { // example using member function with boost.bind
        std::string generated;
        std::back_insert_iterator<std::string> outiter(generated);

        //[karma_tutorial_attach_actions4
        reader r;
        generate(outiter, '{' << int_[boost::bind(&reader::print, &r, _1)] << '}');
        //]

        std::cout << "Member function: " << generated << std::endl;
    }

    { // example using boost.lambda
        namespace lambda = boost::lambda;
        using namespace boost::spirit;

        std::string generated;
        std::back_insert_iterator<std::string> outiter(generated);

        //[karma_tutorial_attach_actions5
        std::stringstream strm("42");
        generate(outiter, '{' << int_[strm >> lambda::_1] << '}');
        //]

        std::cout << "Boost.Lambda: " << generated << std::endl;
    }

    return 0;
}

