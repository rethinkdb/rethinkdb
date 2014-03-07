/*=============================================================================
    Copyright (c) 2003 Sam Nabialek
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  The Nabialek trick.
//
//  [ Sam Nabialek; Somewhere, sometime in 2003... ]    spirit1
//  [ JDG November 17, 2009 ]                           spirit2
//  [ JDG January 10, 2010 ]                            Updated to use rule pointers
//                                                      for efficiency.
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <iostream>
#include <string>

namespace client
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    ///////////////////////////////////////////////////////////////////////////////
    //  Our nabialek_trick grammar
    ///////////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct nabialek_trick : qi::grammar<
        Iterator, ascii::space_type, qi::locals<qi::rule<Iterator, ascii::space_type>*> >
    {
        nabialek_trick() : nabialek_trick::base_type(start)
        {
            using ascii::alnum;
            using qi::lexeme;
            using qi::lazy;
            using qi::_a;
            using qi::_1;

            id = lexeme[*(ascii::alnum | '_')];
            one = id;
            two = id >> ',' >> id;

            keyword.add
                ("one", &one)
                ("two", &two)
                ;

            start = *(keyword[_a = _1] >> lazy(*_a));
        }

        qi::rule<Iterator, ascii::space_type> id, one, two;
        qi::rule<Iterator, ascii::space_type, qi::locals<qi::rule<Iterator, ascii::space_type>*> > start;
        qi::symbols<char, qi::rule<Iterator, ascii::space_type>*> keyword;
    };
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using boost::spirit::ascii::space;
    typedef std::string::const_iterator iterator_type;
    typedef client::nabialek_trick<iterator_type> nabialek_trick;

    nabialek_trick g; // Our grammar

    std::string str = "one only\none again\ntwo first,second";
    std::string::const_iterator iter = str.begin();
    std::string::const_iterator end = str.end();
    bool r = phrase_parse(iter, end, g, space);

    if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
    }
    else
    {
        std::string rest(iter, end);
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "stopped at: \": " << rest << "\"\n";
        std::cout << "-------------------------\n";
    }

    return 0;
}


