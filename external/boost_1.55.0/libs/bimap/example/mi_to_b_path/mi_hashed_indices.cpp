// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


/*****************************************************************************
Boost.MultiIndex
*****************************************************************************/

#include <boost/config.hpp>

//[ code_mi_to_b_path_mi_hashed_indices

#include <iostream>
#include <iomanip>

#include <boost/tokenizer.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/lambda/lambda.hpp>

using namespace boost::multi_index;
namespace bl = boost::lambda;

// word_counter keeps the ocurrences of words inserted. A hashed
// index allows for fast checking of preexisting entries.

struct word_counter_entry
{
    std::string  word;
    unsigned int occurrences;

    word_counter_entry( std::string word_ ) : word(word_), occurrences(0) {}
};

typedef multi_index_container
<
    word_counter_entry,
    indexed_by
    <
        ordered_non_unique
        <
            BOOST_MULTI_INDEX_MEMBER(
                word_counter_entry,unsigned int,occurrences),
            std::greater<unsigned int>
        >,
        hashed_unique
        <
            BOOST_MULTI_INDEX_MEMBER(word_counter_entry,std::string,word)
        >
  >

> word_counter;

typedef boost::tokenizer<boost::char_separator<char> > text_tokenizer;

int main()
{
    std::string text=
        "En un lugar de la Mancha, de cuyo nombre no quiero acordarme... "
        "...snip..."
        "...no se salga un punto de la verdad.";

    // feed the text into the container

    word_counter   wc;
    text_tokenizer tok(text,boost::char_separator<char>(" \t\n.,;:!?'\"-"));
    unsigned int   total_occurrences = 0;

    for( text_tokenizer::iterator it = tok.begin(), it_end = tok.end();
         it != it_end ; ++it )
    {
        ++total_occurrences;
        word_counter::iterator wit = wc.insert(*it).first;
        wc.modify_key( wit, ++ bl::_1 );
    }

    // list words by frequency of appearance

    std::cout << std::fixed << std::setprecision(2);

    for( word_counter::iterator wit = wc.begin(), wit_end=wc.end();
         wit != wit_end; ++wit )
    {
        std::cout << std::setw(11) << wit->word << ": "
                  << std::setw(5)
                  << 100.0 * wit->occurrences / total_occurrences << "%"
                  << std::endl;
    }

    return 0;
}
//]
