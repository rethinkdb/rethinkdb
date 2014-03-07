// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  VC++ 8.0 warns on usage of certain Standard Library and API functions that
//  can be cause buffer overruns or other possible security issues if misused.
//  See http://msdn.microsoft.com/msdnmag/issues/05/05/SafeCandC/default.aspx
//  But the wording of the warning is misleading and unsettling, there are no
//  portable alternative functions, and VC++ 8.0's own libraries use the
//  functions in question. So turn off the warnings.
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------

#include <boost/config.hpp>

#include <iostream>
#include <boost/tokenizer.hpp>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/list_of.hpp>

using namespace boost::bimaps;

struct counter {
    counter() : c(0) {}
    counter& operator++() { ++c; return *this; }
    unsigned int operator++(int) { return c++; }
    operator const unsigned int() const { return c; }
    private:
    unsigned int c;
};

int main()
{
    //[ code_repetitions_counter

    typedef bimap
    <
        unordered_set_of< std::string >,
        list_of< counter > /*< `counter` is an integer that is initialized 
                                in zero in the constructor >*/

    > word_counter;

    typedef boost::tokenizer<boost::char_separator<char> > text_tokenizer;

    std::string text=
        "Relations between data in the STL are represented with maps."
        "A map is a directed relation, by using it you are representing "
        "a mapping. In this directed relation, the first type is related to "
        "the second type but it is not true that the inverse relationship "
        "holds. This is useful in a lot of situations, but there are some "
        "relationships that are bidirectional by nature.";

    // feed the text into the container
    word_counter   wc;
    text_tokenizer tok(text,boost::char_separator<char>(" \t\n.,;:!?'\"-"));

    for( text_tokenizer::const_iterator it = tok.begin(), it_end = tok.end();
         it != it_end ; ++it )
    {
        /*<< Because the right collection type is `list_of`, the right data 
             is not used a key and can be modified in the same way as with
             standard maps. >>*/
        ++ wc.left[*it];
    }

    // list words with counters by order of appearance
    /*<< When we insert the elements using the left map view, the element 
         is inserted at the end of the list. >>*/
    for( word_counter::right_const_iterator
            wit = wc.right.begin(), wit_end = wc.right.end();

         wit != wit_end; ++wit )
    {
        std::cout << wit->second << ": " << wit->first;
    }
    //]

    return 0;
}


