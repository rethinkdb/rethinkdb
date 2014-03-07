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
#include <algorithm>
#include <string>
#include <functional>


// Set type specifications
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/vector_of.hpp>

// bimap container
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/support/lambda.hpp>

#include <libs/bimap/test/test_bimap.hpp>

struct  left_tag {};
struct right_tag {};


template< class Container, class Data >
void test_list_operations(Container & b, Container& c, const Data & d)
{
    c.clear() ;
    c.assign(d.begin(),d.end());
        
    BOOST_CHECK( std::equal( c.begin(), c.end(), d.begin() ) );
    c.reverse();
    BOOST_CHECK( std::equal( c.begin(), c.end(), d.rbegin() ) );

    c.sort();
    BOOST_CHECK( std::equal( c.begin(), c.end(), d.begin() ) );

    c.push_front( *d.begin() );
    BOOST_CHECK( c.size() == d.size()+1 );
    c.unique();
    BOOST_CHECK( c.size() == d.size() );
 
    c.relocate( c.begin(), ++c.begin() );
    c.relocate( c.end(), c.begin(), ++c.begin() );
        
    b.clear();
    c.clear();
    
    c.assign(d.begin(),d.end());
    b.splice(b.begin(),c);

    BOOST_CHECK( c.size() == 0 );
    BOOST_CHECK( b.size() == d.size() );

    c.splice(c.begin(),b,++b.begin());

    BOOST_CHECK( c.size() == 1 );

    c.splice(c.begin(),b,b.begin(),b.end());

    BOOST_CHECK( b.size() == 0 );

    b.assign(d.begin(),d.end());
    c.assign(d.begin(),d.end());
    b.sort();
    c.sort();
    b.merge(c);
    BOOST_CHECK( b.size() == 2*d.size() );
 
    b.unique();
}
    
void test_bimap()
{
    using namespace boost::bimaps;

    typedef std::map<std::string,long> left_data_type;
    left_data_type left_data;
    left_data.insert( left_data_type::value_type("1",1) );
    left_data.insert( left_data_type::value_type("2",2) );
    left_data.insert( left_data_type::value_type("3",3) );
    left_data.insert( left_data_type::value_type("4",4) );

    typedef std::map<long,std::string> right_data_type;
    right_data_type right_data;
    right_data.insert( right_data_type::value_type(1,"1") );
    right_data.insert( right_data_type::value_type(2,"2") );
    right_data.insert( right_data_type::value_type(3,"3") );
    right_data.insert( right_data_type::value_type(4,"4") );


    //--------------------------------------------------------------------
    {
        typedef bimap<
            list_of< std::string >, vector_of< long >
            
        > bm_type;

        std::set< bm_type::value_type > data;
        data.insert( bm_type::value_type("1",1) );
        data.insert( bm_type::value_type("2",2) );
        data.insert( bm_type::value_type("3",3) );
        data.insert( bm_type::value_type("4",4) );

        bm_type b;

        test_bimap_init_copy_swap<bm_type>(data) ;
        test_sequence_container(b,data);
        test_sequence_container(b.left , left_data);
        test_vector_container(b.right,right_data);

        test_mapped_container(b.left );
        test_mapped_container(b.right);

        bm_type c;
        test_list_operations(b,c,data) ;
        test_list_operations(b.left,c.left,left_data) ;
        test_list_operations(b.right,c.right,right_data) ;

        c.assign(data.begin(),data.end());
        b.assign(data.begin(),data.end());
        c.remove_if(_key<=bm_type::value_type("1",1));
        c.sort(std::less<bm_type::value_type>());
        b.sort(std::less<bm_type::value_type>());
        c.merge(b,std::less<bm_type::value_type>());
        c.unique(std::equal_to<bm_type::value_type>());
        
        c.assign(data.begin(),data.end());
        b.assign(data.begin(),data.end());
        c.left.remove_if(_key<="1");
        c.left.sort(std::less<std::string>());
        b.left.sort(std::less<std::string>());
        c.left.merge(b.left,std::less<std::string>());
        c.left.unique(std::equal_to<std::string>());
        
        c.assign(data.begin(),data.end());
        b.assign(data.begin(),data.end());
        c.right.remove_if(_key<=1);
        c.right.sort(std::less<long>());
        b.right.sort(std::less<long>());
        c.right.merge(b.right,std::less<long>());
        c.right.unique(std::equal_to<long>());

        c.assign(data.begin(),data.end());
        c.right[0].first = -1;
        c.right.at(0).second = "[1]";
    }
    //--------------------------------------------------------------------

    
    //--------------------------------------------------------------------
    {
        typedef bimap
        <
            vector_of<std::string>, list_of<long>,
            vector_of_relation

        > bm_type;

        std::set< bm_type::value_type > data;
        data.insert( bm_type::value_type("1",1) );
        data.insert( bm_type::value_type("2",2) );
        data.insert( bm_type::value_type("3",3) );
        data.insert( bm_type::value_type("4",4) );
        
        bm_type b;
        
        test_bimap_init_copy_swap<bm_type>(data) ;
        test_vector_container(b,data) ;
        
        bm_type c;
        test_list_operations(b,c,data) ;
        test_list_operations(b.left,c.left,left_data) ;
        test_list_operations(b.right,c.right,right_data) ;
        
        c.assign(data.begin(),data.end());
        b.assign(data.begin(),data.end());
        c.remove_if(_key<=bm_type::value_type("1",1));
        c.sort(std::less<bm_type::value_type>());
        b.sort(std::less<bm_type::value_type>());
        c.merge(b,std::less<bm_type::value_type>());
        c.unique(std::equal_to<bm_type::value_type>());
        
        c.assign(data.begin(),data.end());
        b.assign(data.begin(),data.end());
        c.left.remove_if(_key<="1");
        c.left.sort(std::less<std::string>());
        b.left.sort(std::less<std::string>());
        c.left.merge(b.left,std::less<std::string>());
        c.left.unique(std::equal_to<std::string>());
        
        c.assign(data.begin(),data.end());
        b.assign(data.begin(),data.end());
        c.right.remove_if(_key<=1);
        c.right.sort(std::less<long>());
        b.right.sort(std::less<long>());
        c.right.merge(b.right,std::less<long>());
        c.right.unique(std::equal_to<long>());
        
        c.assign(data.begin(),data.end());
        c[0].left = "(1)";
        c.at(0).right = -1;
        c.left[0].first = "[1]";
        c.left.at(0).second = -1;
    }
    //--------------------------------------------------------------------

    
    //--------------------------------------------------------------------
    {
        typedef bimap
        <
            vector_of<std::string>, list_of<long>,
            list_of_relation

        > bm_type;

        std::set< bm_type::value_type > data;
        data.insert( bm_type::value_type("1",1) );
        data.insert( bm_type::value_type("2",2) );
        data.insert( bm_type::value_type("3",3) );
        data.insert( bm_type::value_type("4",4) );
        
        bm_type b;
        
        test_bimap_init_copy_swap<bm_type>(data) ;
        test_sequence_container(b,data) ;
        
        bm_type c;
        test_list_operations(b,c,data) ;
        test_list_operations(b.left,c.left,left_data) ;
        test_list_operations(b.right,c.right,right_data) ;

        
        c.assign(data.begin(),data.end());
        b.assign(data.begin(),data.end());
        c.remove_if(_key<=bm_type::value_type("1",1));
        c.sort(std::less<bm_type::value_type>());
        b.sort(std::less<bm_type::value_type>());
        c.merge(b,std::less<bm_type::value_type>());
        c.unique(std::equal_to<bm_type::value_type>());
        
        c.assign(data.begin(),data.end());
        c.left[0].first = "[1]";
        c.left.at(0).second = -1;
    }
    //--------------------------------------------------------------------

}


int test_main( int, char* [] )
{
    test_bimap();
    return 0;
}

