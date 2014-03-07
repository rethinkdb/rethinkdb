// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/******************************************************************************
Boost.MultiIndex
******************************************************************************/

#include <boost/config.hpp>

//[ code_mi_to_b_path_mi_bidirectional_map

#include <iostream>
#include <boost/tokenizer.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/multi_index/ordered_index.hpp>

using namespace boost;
using namespace boost::multi_index;

// tags for accessing both sides of a bidirectional map

struct from {};
struct to   {};

// The class template bidirectional_map wraps the specification
// of a bidirectional map based on multi_index_container.

template<typename FromType,typename ToType>
struct bidirectional_map
{
    typedef std::pair<FromType,ToType> value_type;

    typedef multi_index_container<
        value_type,
        indexed_by
        <
            ordered_unique
            <
                tag<from>, member<value_type,FromType,&value_type::first>
            >,
            ordered_unique
            <
                tag<to>, member<value_type,ToType,&value_type::second>
            >
        >

  > type;

};

// A dictionary is a bidirectional map from strings to strings

typedef bidirectional_map<std::string,std::string>::type dictionary;

int main()
{
    dictionary d;

    // Fill up our microdictionary.
    // first members Spanish, second members English.

    d.insert(dictionary::value_type("hola","hello"));
    d.insert(dictionary::value_type("adios","goodbye"));
    d.insert(dictionary::value_type("rosa","rose"));
    d.insert(dictionary::value_type("mesa","table"));

    std::cout << "enter a word" << std::endl;
    std::string word;
    std::getline(std::cin,word);

    // search the queried word on the from index (Spanish)

    dictionary::iterator it = d.get<from>().find(word);

    if( it != d.end() )
    {
        // the second part of the element is the equivalent in English

        std::cout << word << " is said "
                  << it->second << " in English" << std::endl;
    }
    else
    {
        // word not found in Spanish, try our luck in English

        dictionary::index_iterator<to>::type it2 = d.get<to>().find(word);
        if( it2 != d.get<to>().end() )
        {
            std::cout << word << " is said "
                      << it2->first << " in Spanish" << std::endl;
        }
        else
        {
            std::cout << "No such word in the dictionary" << std::endl;
        }
    }

    return 0;
}
//]
