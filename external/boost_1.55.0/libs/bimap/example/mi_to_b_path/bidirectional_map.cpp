// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


// Boost.Bimap Example
//-----------------------------------------------------------------------------
// This example shows how to construct a bidirectional map with
// multi_index_container.
// By a bidirectional map we mean a container of elements of
// std::pair<const FromType,const ToType> such that no two elements exists with 
// the same first or second value (std::map only guarantees uniqueness of the
// first member).
// Fast lookup is provided for both keys. The program features a tiny
// Spanish-English dictionary with online query of words in both languages.

#include <boost/config.hpp>

//[ code_mi_to_b_path_bidirectional_map

#include <iostream>
#include <boost/tokenizer.hpp>
#include <boost/bimap/bimap.hpp>

using namespace boost::bimaps;

// A dictionary is a bidirectional map from strings to strings

typedef bimap<std::string,std::string> dictionary;
typedef dictionary::value_type translation;

int main()
{
    dictionary d;

    // Fill up our microdictionary.
    // first members Spanish, second members English.

    d.insert( translation("hola" ,"hello"  ));
    d.insert( translation("adios","goodbye"));
    d.insert( translation("rosa" ,"rose"   ));
    d.insert( translation("mesa" ,"table"  ));

    std::cout << "enter a word" << std::endl;
    std::string word;
    std::getline(std::cin,word);

    // search the queried word on the from index (Spanish)

    dictionary::left_const_iterator it = d.left.find(word);

    if( it != d.left.end() )
    {
        // the second part of the element is the equivalent in English

        std::cout << word << " is said "
                  << it->second /*< `it` is an iterator of the left view, so 
                                    `it->second` refers to the right element of
                                     the relation, the word in english >*/
                  << " in English" << std::endl;
    }
    else
    {
        // word not found in Spanish, try our luck in English

        dictionary::right_const_iterator it2 = d.right.find(word);
        if( it2 != d.right.end() )
        {
            std::cout << word << " is said "
                      << it2->second /*< `it2` is an iterator of the right view, so
                                         `it2->second` refers to the left element of
                                          the relation, the word in spanish >*/
                      << " in Spanish" << std::endl;
        }
        else
        {
            std::cout << "No such word in the dictionary" << std::endl;
        }
    }

    return 0;
}
//]
