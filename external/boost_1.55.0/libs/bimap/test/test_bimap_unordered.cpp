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

#include <boost/config.hpp>

#define BOOST_BIMAP_DISABLE_SERIALIZATION

// Boost.Test
#include <boost/test/minimal.hpp>

// std
#include <set>
#include <map>
#include <string>
#include <functional>

// Set type specifications
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>

// bimap container
#include <boost/bimap/bimap.hpp>

#include <libs/bimap/test/test_bimap.hpp>

struct  left_tag {};
struct right_tag {};

void test_bimap()
{
    using namespace boost::bimaps;


    typedef std::map<char,std::string> left_data_type;
    left_data_type left_data;
    left_data.insert( left_data_type::value_type('a',"a") );
    left_data.insert( left_data_type::value_type('b',"b") );
    left_data.insert( left_data_type::value_type('c',"c") );
    left_data.insert( left_data_type::value_type('d',"e") );

    typedef std::map<std::string,char> right_data_type;
    right_data_type right_data;
    right_data.insert( right_data_type::value_type("a",'a') );
    right_data.insert( right_data_type::value_type("b",'b') );
    right_data.insert( right_data_type::value_type("c",'c') );
    right_data.insert( right_data_type::value_type("d",'e') );



    //--------------------------------------------------------------------
    {
        typedef bimap<
            unordered_set_of<char>, unordered_multiset_of<std::string>

        > bm_type;

        std::set< bm_type::value_type > data;
        data.insert( bm_type::value_type('a',"a") );
        data.insert( bm_type::value_type('b',"b") );
        data.insert( bm_type::value_type('c',"c") );
        data.insert( bm_type::value_type('d',"d") );

        bm_type bm;

        test_unordered_set_unordered_multiset_bimap(
            bm,data,left_data,right_data
        );
    }
    //--------------------------------------------------------------------


    //--------------------------------------------------------------------
    {
        typedef bimap<
                 unordered_set_of< tagged< char       , left_tag  > >,
            unordered_multiset_of< tagged< std::string, right_tag > >

        > bm_type;

        std::set< bm_type::value_type > data;
        data.insert( bm_type::value_type('a',"a") );
        data.insert( bm_type::value_type('b',"b") );
        data.insert( bm_type::value_type('c',"c") );
        data.insert( bm_type::value_type('d',"d") );

        bm_type bm;

        test_unordered_set_unordered_multiset_bimap(
            bm,data,left_data,right_data
        );
        test_tagged_bimap<left_tag,right_tag>(bm,data);
    }
    //--------------------------------------------------------------------


    //--------------------------------------------------------------------
    {
        typedef bimap
        <
            set_of< char, std::greater<char> >,
            unordered_multiset_of<std::string>,
            unordered_set_of_relation<>

        > bm_type;

        std::set< bm_type::value_type > data;
        data.insert( bm_type::value_type('a',"a") );
        data.insert( bm_type::value_type('b',"b") );
        data.insert( bm_type::value_type('c',"c") );
        data.insert( bm_type::value_type('d',"d") );

        bm_type bm;

        test_basic_bimap(bm,data,left_data,right_data);
        test_associative_container(bm,data);
        test_simple_unordered_associative_container(bm,data);
    }
    //--------------------------------------------------------------------


    //--------------------------------------------------------------------
    {
                typedef bimap
        <
            unordered_multiset_of< char >,
            unordered_multiset_of< std::string >,
            unordered_multiset_of_relation<>

        > bm_type;

        std::set< bm_type::value_type > data;
        data.insert( bm_type::value_type('a',"a") );
        data.insert( bm_type::value_type('b',"b") );
        data.insert( bm_type::value_type('c',"c") );
        data.insert( bm_type::value_type('d',"d") );

        bm_type bm;

        test_basic_bimap(bm,data,left_data,right_data);
        test_associative_container(bm,data);
        test_simple_unordered_associative_container(bm,data);

    }
    //--------------------------------------------------------------------
}


int test_main( int, char* [] )
{
    test_bimap();
    return 0;
}

