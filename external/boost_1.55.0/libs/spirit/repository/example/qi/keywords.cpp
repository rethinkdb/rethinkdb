/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
//[reference_includes
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/repository/include/qi_kwd.hpp>
#include <boost/spirit/repository/include/qi_keywords.hpp>
#include <iostream>
#include <string>
#include <cstdlib>
#include <iterator>
//]

//[reference_test
template <typename P>
void test_parser(
    char const* input, P const& p, bool full_match = true)
{
    using boost::spirit::qi::parse;

    char const* f(input);
    char const* l(f + strlen(f));
    if (parse(f, l, p) && (!full_match || (f == l)))
        std::cout << "ok" << std::endl;
    else
        std::cout << "fail" << std::endl;
}

template <typename P>
void test_phrase_parser(
    char const* input, P const& p, bool full_match = true)
{
    using boost::spirit::qi::phrase_parse;
    using boost::spirit::qi::ascii::space;
    
    char const* f(input);
    char const* l(f + strlen(f));
    if (phrase_parse(f, l, p, space) && (!full_match || (f == l)))
        std::cout << "ok" << std::endl;
    else
        std::cout << "fail" << std::endl;
}
//]

//[reference_test_attr
template <typename P, typename T>
void test_parser_attr(
    char const* input, P const& p, T& attr, bool full_match = true)
{
    using boost::spirit::qi::parse;

    char const* f(input);
    char const* l(f + strlen(f));
    if (parse(f, l, p, attr) && (!full_match || (f == l)))
        std::cout << "ok" << std::endl;
    else
        std::cout << "fail" << std::endl;
}

template <typename P, typename T>
void test_phrase_parser_attr(
    char const* input, P const& p, T& attr, bool full_match = true)
{
    using boost::spirit::qi::phrase_parse;
    using boost::spirit::qi::ascii::space;

    char const* f(input);
    char const* l(f + strlen(f));
    if (phrase_parse(f, l, p, space, attr) && (!full_match || (f == l)))
        std::cout << "ok" << std::endl;
    else
        std::cout << "fail" << std::endl;
}
//]



//[reference_keyword_list_test_data_structure
// Data structure definitions to test the kwd directive
// and the keywords list operator

struct person {
    std::string name;
    int age;
    double size;
    std::vector<std::string> favorite_colors;
   
};

std::ostream &operator<<(std::ostream &os, const person &p)
{
    os<<"Person : "<<p.name<<", "<<p.age<<", "<<p.size<<std::endl;
    std::copy(p.favorite_colors.begin(),p.favorite_colors.end(),std::ostream_iterator<std::string>(os,"\n"));
    return os;
} 

BOOST_FUSION_ADAPT_STRUCT( person,
    (std::string, name)
    (int, age)
    (double, size)
    (std::vector<std::string>, favorite_colors)
)
//]

int
main()
{
    
     // keyword_list
    {
        //[reference_using_declarations_keyword_list
        using boost::spirit::repository::qi::kwd;
        using boost::spirit::qi::inf;
        using boost::spirit::ascii::space_type; 
        using boost::spirit::ascii::char_;
        using boost::spirit::qi::double_;
        using boost::spirit::qi::int_;
        using boost::spirit::qi::rule;
        //]
       
        //[reference_keyword_list_rule_declarations
        rule<const char *, std::string(), space_type> parse_string;
        rule<const char *, person(), space_type> no_constraint_person_rule, constraint_person_rule; 
        
         parse_string   %= '"'> *(char_-'"') > '"';
        //]
        
        //[reference_keyword_list_no_constraint_rule
        no_constraint_person_rule %= 
            kwd("name")['=' > parse_string ] 
          / kwd("age")   ['=' > int_] 
          / kwd("size")   ['=' > double_ > 'm']
          ;
        //]
        

        //[reference_keyword_list
        //`Parsing a keyword list:
        // Let's declare a small list of people for which we want to collect information.
        person John,Mary,Mike,Hellen,Johny;
        test_phrase_parser_attr(
                        "name = \"John\" \n age = 10 \n size = 1.69m "
                        ,no_constraint_person_rule 
                        ,John);  // full in orginal order
        std::cout<<John;

        test_phrase_parser_attr(
                        "age = 10 \n size = 1.69m \n name = \"Mary\""
                        ,no_constraint_person_rule 
                        ,Mary);  // keyword oder doesn't matter
        std::cout<<Mary;

        test_phrase_parser_attr(
                         "size = 1.69m \n name = \"Mike\" \n age = 10 "
                        ,no_constraint_person_rule
                        ,Mike);  // still the same result
        
        std::cout<<Mike;
        
         /*`The code above will print:[teletype]
        
                Person : John, 10, 1.69
                Person : Mary, 10, 1.69
                Person : Mike, 10, 1.69
         */
        //]

        //[reference_keyword_list_constraint_rule
        /*`The parser definition below uses the kwd directive occurence constraint variants to 
            make sure that the name and age keyword occur only once and allows the favorite color 
            entry to appear 0 or more times. */
        constraint_person_rule %=
            kwd("name",1)                 ['=' > parse_string ]
          / kwd("age"   ,1)                 ['=' > int_]
          / kwd("size"   ,1)                 ['=' > double_ > 'm']
          / kwd("favorite color",0,inf) [ '=' > parse_string ]
          ;
        //]
         
        //[reference_keyword_list_constraints
      
        // Here all the give constraint are resepected : parsing will succeed.
        test_phrase_parser_attr(
            "name = \"Hellen\" \n age = 10 \n size = 1.80m \n favorite color = \"blue\" \n favorite color = \"green\" "
            ,constraint_person_rule 
            ,Hellen);   
        std::cout<<Hellen;
        
       // Parsing this string will fail because the age and size minimum occurence requirements aren't met.
       test_phrase_parser_attr(
            "name = \"Johny\"  \n favorite color = \"blue\" \n favorite color = \"green\" "
            ,constraint_person_rule
            ,Johny );
        
        /*`Parsing the first string will succeed but fail for the second string as the 
        occurence constraints aren't met. This code should print:[teletype]
        
        Person : Hellen, 10, 1.8
        blue
        green
        */
        //]
    }
    
        
    return 0;
}
