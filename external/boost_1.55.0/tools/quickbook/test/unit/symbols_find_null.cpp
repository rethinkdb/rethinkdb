/*=============================================================================
    Copyright (c) 2004 Joao Abecasis
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/spirit/include/classic_scanner.hpp>
#include <boost/utility/addressof.hpp>
#include "symbols.hpp"

typedef char char_type;
typedef char const * iterator;

char_type data_[] = "whatever";

iterator begin = data_;
iterator end = data_
    + sizeof(data_)/sizeof(char_type); // Yes, this is an intentional bug ;)

int main()
{
    typedef BOOST_SPIRIT_CLASSIC_NS::scanner<> scanner;
    typedef quickbook::tst<void *, char_type> symbols;

    symbols symbols_;

    symbols_.add(begin, end - 1, (void*) boost::addressof(symbols_));

    // The symbol table parser should not choke on input containing the null
    // character.
    symbols_.find(scanner(begin, end));
}
