// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


// Boost.Bimap Example
//-----------------------------------------------------------------------------
// Hashed indices can be used as an alternative to ordered indices when fast
// lookup is needed and sorting information is of no interest. The example
// features a word counter where duplicate entries are checked by means of a 
// hashed index.

#include <boost/config.hpp>

//[ code_mi_to_b_path_hashed_indices

#include <iostream>
#include <iomanip>

#include <boost/tokenizer.hpp>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/support/lambda.hpp>

using namespace boost::bimaps;

struct word        {};
struct occurrences {};

typedef bimap
<
    
     multiset_of< tagged<unsigned int,occurrences>, std::greater<unsigned int> >, 
unordered_set_of< tagged< std::string,       word>                             >

> word_counter;

typedef boost::tokenizer<boost::char_separator<char> > text_tokenizer;

int main()
{

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
    unsigned int   total_occurrences = 0;

    for( text_tokenizer::const_iterator it = tok.begin(), it_end = tok.end();
         it != it_end ; ++it )
    {
        ++total_occurrences;

        word_counter::map_by<occurrences>::iterator wit =
            wc.by<occurrences>().insert(
                 word_counter::map_by<occurrences>::value_type(0,*it)
            ).first;

        wc.by<occurrences>().modify_key( wit, ++_key);
    }

    // list words by frequency of appearance

    std::cout << std::fixed << std::setprecision(2);

    for( word_counter::map_by<occurrences>::const_iterator
            wit     = wc.by<occurrences>().begin(),
            wit_end = wc.by<occurrences>().end();

         wit != wit_end; ++wit )
    {
        std::cout << std::setw(15) << wit->get<word>() << ": "
                  << std::setw(5)
                  << 100.0 * wit->get<occurrences>() / total_occurrences << "%"
                  << std::endl;
    }

    return 0;
}
//]
