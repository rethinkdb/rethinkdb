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

void untagged_version()
{
    //[ code_user_defined_names_untagged_version

    typedef bimap
    <
        multiset_of<std::string>,
        int

    > People;

    People people;

    // ...

    int user_id;
    std::cin >> user_id;

    // people.right : map<id,name>

    People::right_const_iterator id_iter = people.right.find(user_id);
    if( id_iter != people.right.end() )
    {
        // first  : id
        // second : name

        std::cout << "name: " << id_iter->second << std::endl
                  << "id:   " << id_iter->first  << std::endl;
    }
    else
    {
        std::cout << "Unknown id, users are:" << std::endl;

        // people.left : map<name,id>

        for( People::left_const_iterator
                name_iter = people.left.begin(),
                iend      = people.left.end();

             name_iter != iend; ++name_iter )
        {
            // first  : name
            // second : id

            std::cout << "name: " << name_iter->first  << std::endl
                      << "id:   " << name_iter->second << std::endl;
        }
    }
    //]
}

struct id   {};
struct name {};

void tagged_version()
{
    //[ code_user_defined_names_tagged_version

    //<-
    /*
    //->
    struct id   {}; // Tag for the identification number
    struct name {}; // Tag for the name of the person
    //<-
    */
    //->

    typedef bimap
    <
                     tagged< int        , id   >  ,
        multiset_of< tagged< std::string, name > >

    > People;

    People people;

    // ...

    int user_id;
    std::cin >> user_id;

    People::map_by<id>::const_iterator id_iter = people.by<id>().find(user_id);
    if( id_iter != people.by<id>().end() )
    {
        std::cout << "name: " << id_iter->get<name>() << std::endl
                  << "id:   " << id_iter->get<id>()   << std::endl;
    }
    else
    {
        std::cout << "Unknown id, users are:" << std::endl;

        for( People::map_by<name>::const_iterator
                name_iter = people.by<name>().begin(),
                iend      = people.by<name>().end();

            name_iter != iend; ++name_iter )
        {
            std::cout << "name: " << name_iter->get<name>() << std::endl
                      << "id:   " << name_iter->get<id>()   << std::endl;
        }
    }
    //]
}

int main()
{
    untagged_version();
    tagged_version();

    return 0;
}

