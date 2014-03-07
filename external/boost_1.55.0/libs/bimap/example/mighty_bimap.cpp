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
// This is the translator example from the tutorial.
// In this example the set type of relation is changed to allow the iteration
// of the container.

#include <boost/config.hpp>

//[ code_mighty_bimap

#include <iostream>
#include <string>
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>

struct english {};
struct spanish {};

int main()
{
    using namespace boost::bimaps;

    typedef bimap
    <
        unordered_set_of< tagged< std::string, spanish > >,
        unordered_set_of< tagged< std::string, english > >,
        list_of_relation

    > translator;

    translator trans;

    // We have to use `push_back` because the collection of relations is
    // a `list_of_relation`

    trans.push_back( translator::value_type("hola"  ,"hello"   ) );
    trans.push_back( translator::value_type("adios" ,"goodbye" ) );
    trans.push_back( translator::value_type("rosa"  ,"rose"    ) );
    trans.push_back( translator::value_type("mesa"  ,"table"   ) );

    std::cout << "enter a word" << std::endl;
    std::string word;
    std::getline(std::cin,word);

    // Search the queried word on the from index (Spanish)

    translator::map_by<spanish>::const_iterator is
        = trans.by<spanish>().find(word);

    if( is != trans.by<spanish>().end() )
    {
        std::cout << word << " is said "
                  << is->get<english>()
                  << " in English" << std::endl;
    }
    else
    {
        // Word not found in Spanish, try our luck in English

        translator::map_by<english>::const_iterator ie
            = trans.by<english>().find(word);

        if( ie != trans.by<english>().end() )
        {
            std::cout << word << " is said "
                      << ie->get<spanish>()
                      << " in Spanish" << std::endl;
        }
        else
        {
            // Word not found, show the possible translations

            std::cout << "No such word in the dictionary"      << std::endl;
            std::cout << "These are the possible translations" << std::endl;

            for( translator::const_iterator
                    i = trans.begin(),
                    i_end = trans.end();

                    i != i_end ; ++i )
            {
                std::cout << i->get<spanish>()
                          << " <---> "
                          << i->get<english>()
                          << std::endl;
            }
        }
    }
    return 0;
}
//]
