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

#include <string>
#include <iostream>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>

using namespace boost::bimaps;


void tutorial_about_info_hook()
{
    //[ code_tutorial_info_hook_first

    typedef bimap<

        multiset_of< std::string >, // author
             set_of< std::string >, // title

          with_info< std::string >  // abstract

    > bm_type;
    typedef bm_type::value_type book;

    bm_type bm;

    bm.insert(

        book( "Bjarne Stroustrup"   , "The C++ Programming Language",

              "For C++ old-timers, the first edition of this book is"
              "the one that started it all—the font of our knowledge." )
    );


    // Print the author of the bible
    std::cout << bm.right.at("The C++ Programming Language");

    // Print the abstract of this book
    bm_type::left_iterator i = bm.left.find("Bjarne Stroustrup");
    std::cout << i->info;
    //]

    // Contrary to the two key types, the information will be mutable
    // using iterators.

    //[ code_tutorial_info_hook_mutable

    i->info += "More details about this book";
    //]

    // A new function is included in unique map views: info_at(key), that
    // mimics the standard at(key) function but returned the associated
    // information instead of the data.

    //[ code_tutorial_info_hook_info_at

    // Print the new abstract
    std::cout << bm.right.info_at("The C++ Programming Language");
    //]
}

struct author {};
struct title {};
struct abstract {};

void tutorial_about_tagged_info_hook()
{
    //[ code_tutorial_info_hook_tagged_info

    typedef bimap<

        multiset_of< tagged< std::string, author   > >,
             set_of< tagged< std::string, title    > >,

          with_info< tagged< std::string, abstract > >

    > bm_type;
    typedef bm_type::value_type book;

    bm_type bm;

    bm.insert(

        book( "Bjarne Stroustrup"   , "The C++ Programming Language",

              "For C++ old-timers, the first edition of this book is"
              "the one that started it all—the font of our knowledge." )
    );

    // Print the author of the bible
    std::cout << bm.by<title>().at("The C++ Programming Language");

    // Print the abstract of this book
    bm_type::map_by<author>::iterator i = bm.by<author>().find("Bjarne Stroustrup");
    std::cout << i->get<abstract>();

    // Contrary to the two key types, the information will be mutable
    // using iterators.

    i->get<abstract>() += "More details about this book";

    // Print the new abstract
    std::cout << bm.by<title>().info_at("The C++ Programming Language");
    //]
}


void bimap_without_an_info_hook()
{
    //[ code_tutorial_info_hook_nothing

    typedef bimap<

        multiset_of< std::string >, // author
             set_of< std::string >  // title

    > bm_type;
    typedef bm_type::value_type book;

    bm_type bm;

    bm.insert( book( "Bjarne Stroustrup"   , "The C++ Programming Language" ) );
    bm.insert( book( "Scott Meyers"        , "Effective C++"                ) );
    bm.insert( book( "Andrei Alexandrescu" , "Modern C++ Design"            ) );

    // Print the author of Modern C++
    std::cout << bm.right.at( "Modern C++ Design" );
    //]
}


int main()
{
    tutorial_about_info_hook();
    tutorial_about_tagged_info_hook();
    bimap_without_an_info_hook();

    return 0;
}


