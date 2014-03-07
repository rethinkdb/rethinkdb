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

//[ code_mi_to_b_path_tagged_bidirectional_map

#include <iostream>

#include <boost/bimap/bimap.hpp>

using namespace boost::bimaps;

// tags

struct spanish {};
struct english {};

// A dictionary is a bidirectional map from strings to strings

typedef bimap
<
    tagged< std::string,spanish >, tagged< std::string,english >

> dictionary;

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

    // search the queried word on the from index (Spanish) */

    dictionary::map_by<spanish>::const_iterator it =
        d.by<spanish>().find(word);

    if( it != d.by<spanish>().end() )
    {
        std::cout << word << " is said " 
                  << it->get<english>() << " in English" << std::endl;
    }
    else
    {
        // word not found in Spanish, try our luck in English

        dictionary::map_by<english>::const_iterator it2 =
            d.by<english>().find(word);

        if( it2 != d.by<english>().end() )
        {
            std::cout << word << " is said "
                      << it2->get<spanish>() << " in Spanish" << std::endl;
        }
        else
        {
            std::cout << "No such word in the dictionary" << std::endl;
        }
    }

    return 0;
}
//]
