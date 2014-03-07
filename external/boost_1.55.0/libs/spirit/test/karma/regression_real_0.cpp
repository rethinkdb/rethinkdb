//  Copyright (c) 2012 Agustin K-ballo Berge
//  Copyright (c) 2001-2012 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <iterator>
#include <sstream>
#include <string>

int main( int argc, char** argv )
{
    namespace karma = boost::spirit::karma;
    std::string output;
    std::back_insert_iterator< std::string > sink = std::back_inserter( output );
    karma::generate( sink, karma::double_, 0 ); // prints ¨inf¨ instead of ¨0.0¨
    BOOST_TEST((output == "0.0"));
    return boost::report_errors();
}

