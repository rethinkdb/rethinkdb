// Boost.Assign library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/assign/
//


#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#  pragma warn -8091 // supress warning in Boost.Test
#  pragma warn -8057 // unused argument argc/argv in Boost.Test
#endif

#include <boost/assign/list_of.hpp>
#include <boost/assign/list_inserter.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <cstddef>
#include <ostream>
#include <string>

using namespace boost;
using namespace boost::multi_index;
namespace ba =  boost::assign;

//
// Define a classical multi_index_container for employees
//
struct employee
{
  int         id;
  std::string name;
  int         age;

  employee(int id_,std::string name_,int age_):id(id_),name(name_),age(age_){}

  bool operator==(const employee& x)const
  {
    return id==x.id&&name==x.name&&age==x.age;
  }

  bool operator<(const employee& x)const
  {
    return id<x.id;
  }

  bool operator!=(const employee& x)const{return !(*this==x);}
  bool operator> (const employee& x)const{return x<*this;}
  bool operator>=(const employee& x)const{return !(*this<x);}
  bool operator<=(const employee& x)const{return !(x<*this);}

  struct comp_id
  {
    bool operator()(int x,const employee& e2)const{return x<e2.id;}
    bool operator()(const employee& e1,int x)const{return e1.id<x;}
  };

  friend std::ostream& operator<<(std::ostream& os,const employee& e)
  {
    os<<e.id<<" "<<e.name<<" "<<e.age<<std::endl;
    return os;
  }
};

struct name{};
struct by_name{};
struct age{};
struct as_inserted{};

typedef
  multi_index_container<
    employee,
    indexed_by<
      ordered_unique<
        identity<employee> >,
      ordered_non_unique<
        tag<name,by_name>,
        BOOST_MULTI_INDEX_MEMBER(employee,std::string,name)>,
      ordered_non_unique<
        tag<age>,
        BOOST_MULTI_INDEX_MEMBER(employee,int,age)>,
      sequenced<
        tag<as_inserted> > > >
  employee_set;

#if defined(BOOST_NO_MEMBER_TEMPLATES)
typedef nth_index<
  employee_set,1>::type                       employee_set_by_name;
#else
typedef employee_set::nth_index<1>::type employee_set_by_name;
#endif

typedef boost::multi_index::index<
         employee_set,age>::type         employee_set_by_age;
typedef boost::multi_index::index<
         employee_set,as_inserted>::type employee_set_as_inserted;

//
// Define a multi_index_container with a list-like index and an ordered index
//
typedef multi_index_container<
  std::string,
  indexed_by<
    sequenced<>,                               // list-like index
    ordered_non_unique<identity<std::string> > // words by alphabetical order
  >
> text_container;



void test_multi_index_container()
{
    employee_set eset = ba::list_of< employee >(1,"Franz",30)(2,"Hanz",40)(3,"Ilse",50);
    BOOST_CHECK( eset.size() == 3u );
    
    //
    // This container is associative, hence we can use 'insert()' 
    // 
    
    ba::insert( eset )(4,"Kurt",55)(5,"Bjarne",77)(7,"Thorsten",24);
    BOOST_CHECK( eset.size() == 6u );
    
    employee_set_by_name& name_index = boost::multi_index::get<name>(eset);
    employee_set_by_name::iterator i = name_index.find("Ilse");
    BOOST_CHECK( i->id == 3 );
    BOOST_CHECK( i->age == 50 );
    
    text_container text = ba::list_of< std::string >("Have")("you")("ever")("wondered")("how")("much")("Boost")("rocks?!");
    BOOST_CHECK_EQUAL( text.size(), 8u );
    BOOST_CHECK_EQUAL( *text.begin(), "Have" );
    
    //
    // This container is a sequence, hence we can use 'push_back()' and 'push_font()'
    //
    
    ba::push_back( text )("Well")(",")("A")("LOT")(",")("obviously!");
    BOOST_CHECK_EQUAL( text.size(), 14u );
    BOOST_CHECK_EQUAL( *--text.end(), "obviously!" );
    
    ba::push_front( text ) = "question:", "simple", "A";
    BOOST_CHECK_EQUAL( text.size(), 17u );
    BOOST_CHECK_EQUAL( text.front(), "A" );
}



using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &test_multi_index_container ) );

    return test;
}


