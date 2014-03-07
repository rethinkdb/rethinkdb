/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Thomas Bernard

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <string>
#include <vector>

#include <boost/detail/lightweight_test.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/repository/include/qi_kwd.hpp>
#include <boost/spirit/repository/include/qi_keywords.hpp>

#include <string>
#include <iostream>
#include "test.hpp"

struct x_attr
{
    
};

namespace boost { namespace spirit { namespace traits 
{
    
    
    template <>
    struct container_value<x_attr>
    {
        typedef char type; // value type of container
    };

    
    template <>
    struct push_back_container<x_attr, char>
    {
        static bool call(x_attr& /*c*/, char /*val*/)
        {
            // push back value type into container
            return true;
        }
    };
}}} 

int
main()
{
    using spirit_test::test_attr;
    using spirit_test::test;
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;
    using boost::spirit::repository::kwd;
    using boost::spirit::repository::ikwd;
    using boost::spirit::qi::inf;
    using boost::spirit::qi::omit;
    using boost::spirit::qi::int_;
    using boost::spirit::qi::lit;
    using boost::spirit::qi::_1;
    using boost::spirit::qi::lexeme;

    {
    
        // no constraints
        boost::fusion::vector<char,char,int> data;
        BOOST_TEST( test_attr("c=1 a=a", kwd("a")[ '=' > char_] / kwd("b")[ '=' > char_] / kwd("c")['=' > int_], data, space));
        BOOST_TEST( boost::fusion::at_c<0>(data) == 'a' );
        BOOST_TEST( boost::fusion::at_c<1>(data) == 0 );
        BOOST_TEST( boost::fusion::at_c<2>(data) == 1 );

        // Exact
        BOOST_TEST(test("a=a b=b c=1", kwd("a",1)[ '=' > char_] / kwd("b")[ '=' > char_] / kwd("c")['=' > int_], space)); 
        BOOST_TEST(test("a=a b=c b=e c=1", kwd("a",1)[ '=' > char_] / kwd("b",2)[ '=' > char_] / kwd("c")['=' > int_], space)); 
        BOOST_TEST(!test("b=c b=e c=1", kwd("a",1)[ '=' > char_] / kwd("b",2)[ '=' > char_] / kwd("c")['=' > int_], space)); 
        
        // Min - Max 
        BOOST_TEST(test("a=f b=c b=e c=1", kwd("a",1,2)[ '=' > char_] / kwd("b",0,2)[ '=' > char_] / kwd("c",1,2)['=' > int_], space)); 
        BOOST_TEST(!test("b=c b=e c=1", kwd("a",1,2)[ '=' > char_] / kwd("b",0,1)[ '=' > char_] / kwd("c",1,2)['=' > int_], space)); 
        BOOST_TEST(test("a=g a=f b=c b=e c=1", kwd("a",1,2)[ '=' > char_] / kwd("b",0,2)[ '=' > char_] / kwd("c",1,2)['=' > int_], space)); 
        BOOST_TEST(!test("a=f a=e b=c b=e a=p c=1", kwd("a",1,2)[ '=' > char_] / kwd("b",0,1)[ '=' > char_] / kwd("c",1,2)['=' > int_], space)); 

        // Min - inf
        BOOST_TEST(test("a=f b=c b=e c=1", kwd("a",1,inf)[ '=' > char_] / kwd("b",0,inf)[ '=' > char_] / kwd("c",1,inf)['=' > int_], space )); 
        BOOST_TEST(!test("b=c b=e c=1", kwd("a",1,inf)[ '=' > char_] / kwd("b",0,inf)[ '=' > char_] / kwd("c",1,inf)['=' > int_], space )); 
        BOOST_TEST(test("a=f a=f a=g b=c b=e c=1 a=e", kwd("a",1,inf)[ '=' > char_] / kwd("b",0,inf)[ '=' > char_] / kwd("c",1,inf)['=' > int_], space )); 
    }

    {   // Single keyword, empty string
        BOOST_TEST(test(" ", kwd("aad")[char_],space));        
        BOOST_TEST(test("aad E ", kwd("aad")[char_],space));        
        //BOOST_TEST(test("AaD E ", ikwd("aad")[char_],space));        
        
    }
    
    {
        // Vector container
        boost::fusion::vector<std::vector<int>,std::vector<int>,std::vector<int> > data;
        BOOST_TEST(test_attr(" a=1 b=2 b=5 c=3",kwd("a")[ '=' > int_] / kwd("b")[ '=' > int_] / kwd("c")['=' > int_] , data, space) 
                    && (boost::fusion::at_c<0>(data).size()==1) 
                    && (boost::fusion::at_c<0>(data)[0]==1)
    
                    &&(boost::fusion::at_c<1>(data).size()==2)
                    &&(boost::fusion::at_c<1>(data)[0]==2)
                    &&(boost::fusion::at_c<1>(data)[1]==5)
    
                    &&(boost::fusion::at_c<2>(data).size()==1)
                    &&(boost::fusion::at_c<2>(data)[0]==3)
            );
    }

    {
        // no_case test
        BOOST_TEST( test("B=a c=1 a=E", no_case[kwd("a")[ "=E" ] / kwd("b")[ '=' > char_] / kwd("c")['=' > int_]], space)); 
        BOOST_TEST( test("B=a c=1 a=e", no_case[kwd("a")[ "=E" ] / kwd("b")[ '=' > char_] / kwd("c")['=' > int_]], space)); 
        BOOST_TEST( !test("B=a c=1 A=E", no_case[kwd("a")[ '=' > char_]] / kwd("b")[ '=' > char_] / kwd("c")['=' > int_], space)); 
        BOOST_TEST( test("b=a c=1 A=E", no_case[kwd("a")[ '=' > char_]] / kwd("b")[ '=' > char_] / kwd("c")['=' > int_], space)); 
        BOOST_TEST( !test("A=a c=1 a=E", kwd("a")[ '=' > char_] / kwd("b")[ '=' > char_] / kwd("c")['=' > int_], space)); 
        BOOST_TEST( test("A=a c=1 a=E", ikwd("a")[ '=' > char_] / kwd("b")[ '=' > char_] / kwd("c")['=' > int_], space)); 
        BOOST_TEST( !test("A=a C=1 a=E", ikwd("a")[ '=' > char_] / kwd("b")[ '=' > char_] / kwd("c")['=' > int_], space)); 
    }
    
    {
        // iterator restoration
        BOOST_TEST( test("a=a c=1 ba=d", (kwd("a")[ '=' > char_] / kwd("b")[ '=' > int_] / kwd("c")['=' > int_] ) | lit("ba=") > char_, space)); 
        BOOST_TEST( test("A=a c=1 ba=d", (ikwd("a")[ '=' > char_] / kwd("b")[ '=' > int_] / kwd("c")['=' > int_] ) | lit("ba=") > char_, space)); 
    }

    { // actions
        namespace phx = boost::phoenix;

        std::vector<int> v;
        BOOST_TEST(test("b=2 c=4", kwd("b")['=' > int_][phx::ref(v)=_1] / kwd("c")[ '=' > int_ ],space) && 
            v[0] == 2 );
    }

    { // attribute customization
        
        x_attr x;
//        test_attr("a = b c = d", kwd("a")['=' > char_] / kwd("c")['=' > char_], x);
    }

    return boost::report_errors();
}

