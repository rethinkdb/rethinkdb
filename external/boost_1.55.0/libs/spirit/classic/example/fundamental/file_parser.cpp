/*=============================================================================
    Copyright (c) 2002 Jeff Westfahl
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A parser that echoes a file
//  See the "File Iterator" chapter in the User's Guide.
//
//  [ JMW 8/05/2002 ]
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_file_iterator.hpp>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
using namespace BOOST_SPIRIT_CLASSIC_NS;

////////////////////////////////////////////////////////////////////////////
//
//  Types
//
////////////////////////////////////////////////////////////////////////////
typedef char                    char_t;
typedef file_iterator<char_t>   iterator_t;
typedef scanner<iterator_t>     scanner_t;
typedef rule<scanner_t>         rule_t;

////////////////////////////////////////////////////////////////////////////
//
//  Actions
//
////////////////////////////////////////////////////////////////////////////
void echo(iterator_t first, iterator_t const& last)
{
    while (first != last)
        std::cout << *first++;
}

////////////////////////////////////////////////////////////////////////////
//
//  Main program
//
////////////////////////////////////////////////////////////////////////////
int
main(int argc, char* argv[])
{
    if (2 > argc)
    {
        std::cout << "Must specify a filename!\n";
        return -1;
    }

    // Create a file iterator for this file
    iterator_t first(argv[1]);

    if (!first)
    {
        std::cout << "Unable to open file!\n";
        return -1;
    }

    // Create an EOF iterator
    iterator_t last = first.make_end();

    // A simple rule
    rule_t r = *(anychar_p);

    // Parse
    parse_info <iterator_t> info = parse(
        first,
        last,
        r[&echo]
    );

    // This really shouldn't fail...
    if (info.full)
        std::cout << "Parse succeeded!\n";
    else
        std::cout << "Parse failed!\n";

   return 0;
}
