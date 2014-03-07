/*=============================================================================
    Copyright (c) 2004 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_dynamic.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <iostream>
#include <boost/detail/lightweight_test.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;
int the_var_to_be_tested = 0;

namespace local
{
    template <typename T>
    struct var_wrapper : public ::boost::reference_wrapper<T>
    {
        typedef ::boost::reference_wrapper<T> parent;

        explicit inline var_wrapper(T& t) : parent(t) {}

        inline T& operator()() const { return parent::get(); }
    };

    template <typename T>
    inline var_wrapper<T>
    var(T& t)
    {
        return var_wrapper<T>(t);
    }
}

struct test_grammar : public grammar <test_grammar>
{
    template <typename ScannerT>
    
    struct definition
    {
        rule <ScannerT, parser_tag <0> > test;
        
        definition(const test_grammar& self)
        {
            test
                = if_p(local::var(the_var_to_be_tested))
                    [
                        real_p
                    ];

        }
        const rule <ScannerT, parser_tag<0> >& start() const {return test;}
    };
};

int main()
{
    test_grammar gram;
    tree_parse_info <const char*> result;

    //predictably fails
    the_var_to_be_tested = 0;
    result = ast_parse("1.50", gram, space_p);
    std::cout << "Testing if_p against: " << the_var_to_be_tested << std::endl;
    std::cout << "success: " << result.full << std::endl;
    BOOST_TEST(!result.full);

    //predicatably succeeds
    the_var_to_be_tested = 1;
    result = ast_parse("1.50", gram, space_p);
    std::cout << "Testing if_p against: " << the_var_to_be_tested << std::endl;
    std::cout << "success: " << result.full << std::endl;
    BOOST_TEST(result.full);

    //should succeed
    the_var_to_be_tested = 2;
    result = ast_parse("1.50", gram, space_p);
    std::cout << "Testing if_p against: " << the_var_to_be_tested << std::endl;
    std::cout << "success: " << result.full << std::endl;
    BOOST_TEST(result.full);

    return boost::report_errors();
}

