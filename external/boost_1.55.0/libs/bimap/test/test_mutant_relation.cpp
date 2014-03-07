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

// std
#include <string>

// Boost.Test
#include <boost/test/minimal.hpp>

// Boost.MPL
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

// Boost.Bimap
#include <boost/bimap/detail/test/check_metadata.hpp>
#include <boost/bimap/tags/tagged.hpp>

// Boost.Bimap.Relation
#include <boost/bimap/relation/mutant_relation.hpp>
#include <boost/bimap/relation/member_at.hpp>
#include <boost/bimap/relation/support/get.hpp>
#include <boost/bimap/relation/support/pair_by.hpp>
#include <boost/bimap/relation/support/pair_type_by.hpp>
#include <boost/bimap/relation/support/value_type_of.hpp>
#include <boost/bimap/relation/support/member_with_tag.hpp>
#include <boost/bimap/relation/support/is_tag_of_member_at.hpp>

// Bimap Test Utilities
#include "test_relation.hpp"

BOOST_BIMAP_TEST_STATIC_FUNCTION( untagged_static_test )
{
    using namespace boost::bimaps::relation::member_at;
    using namespace boost::bimaps::relation;
    using namespace boost::bimaps::tags;

    struct left_data  {};
    struct right_data {};

    typedef mutant_relation< left_data, right_data > rel;

    BOOST_BIMAP_CHECK_METADATA(rel,left_value_type ,left_data);
    BOOST_BIMAP_CHECK_METADATA(rel,right_value_type,right_data);

    BOOST_BIMAP_CHECK_METADATA(rel,left_tag ,left );
    BOOST_BIMAP_CHECK_METADATA(rel,right_tag,right);

    typedef tagged<left_data ,left > desired_tagged_left_type;
    BOOST_BIMAP_CHECK_METADATA(rel,tagged_left_type,desired_tagged_left_type);

    typedef tagged<right_data,right> desired_tagged_right_type;
    BOOST_BIMAP_CHECK_METADATA(rel,tagged_right_type,desired_tagged_right_type);

}

BOOST_BIMAP_TEST_STATIC_FUNCTION( tagged_static_test)
{
    using namespace boost::bimaps::relation::member_at;
    using namespace boost::bimaps::relation;
    using namespace boost::bimaps::tags;

    struct left_data  {};
    struct right_data {};

    struct left_tag   {};
    struct right_tag  {};

    typedef mutant_relation<
        tagged<left_data,left_tag>, tagged<right_data,right_tag> > rel;

    BOOST_BIMAP_CHECK_METADATA(rel,left_value_type ,left_data);
    BOOST_BIMAP_CHECK_METADATA(rel,right_value_type,right_data);

    BOOST_BIMAP_CHECK_METADATA(rel,left_tag ,left_tag  );
    BOOST_BIMAP_CHECK_METADATA(rel,right_tag,right_tag );

    typedef tagged<left_data ,left_tag > desired_tagged_left_type;
    BOOST_BIMAP_CHECK_METADATA(rel,tagged_left_type,desired_tagged_left_type);

    typedef tagged<right_data,right_tag> desired_tagged_right_type;
    BOOST_BIMAP_CHECK_METADATA(rel,tagged_right_type,desired_tagged_right_type);
}

struct mutant_relation_builder
{
    template< class LeftType, class RightType >
    struct build
    {
        typedef boost::bimaps::relation::
            mutant_relation<LeftType,RightType,::boost::mpl::na,true> type;
    };
};

// Complex classes

class cc1
{
    public:
    cc1(int s = 0) : a(s+100), b(s+101) {}
    static int sd;
    int a;
    const int b;
};

bool operator==(const cc1 & da, const cc1 & db)
{
    return da.a == db.a && da.b == db.b; 
}

int cc1::sd = 102;

class cc2_base
{
    public:
    cc2_base(int s) : a(s+200) {}
    int a;
};

class cc2 : public cc2_base
{
    public:
    cc2(int s = 0) : cc2_base(s), b(s+201) {}
    int b;
};

bool operator==(const cc2 & da, const cc2 & db)
{
    return da.a == db.a && da.b == db.b; 
}

class cc3_base
{
    public:
    cc3_base(int s = 0) : a(s+300) {}
    const int a;
};

class cc3 : virtual public cc3_base
{
   public:
   cc3(int s = 0) : cc3_base(s), b(s+301) {}
   int b;
};

bool operator==(const cc3 & da, const cc3 & db)
{
    return da.a == db.a && da.b == db.b; 
}

class cc4_base
{
    public:
    cc4_base(int s) : a(s+400) {}
    virtual ~cc4_base() {}
    const int a;
};

class cc4 : public cc4_base
{
   public:
   cc4(int s = 0) : cc4_base(s), b(s+401) {}
   int b;
};

bool operator==(const cc4 & da, const cc4 & db)
{
    return da.a == db.a && da.b == db.b; 
}

class cc5 : public cc1, public cc3, public cc4
{
    public:
    cc5(int s = 0) : cc1(s), cc3(s), cc4(s) {}
};

bool operator==(const cc5 & da, const cc5 & db)
{
    return da.cc1::a == db.cc1::a && da.cc1::b == db.cc1::b &&
           da.cc3::a == db.cc3::a && da.cc3::b == db.cc3::b &&
           da.cc4::a == db.cc4::a && da.cc4::b == db.cc4::b;
}

class cc6
{
    public:
    cc6(int s = 0) : a(s+600), b(a) {}
    int a;
    int & b;
};

bool operator==(const cc6 & da, const cc6 & db)
{
    return da.a == db.a && da.b == db.b;
}

void test_mutant_relation()
{
    test_relation< mutant_relation_builder, char  , double >( 'l', 2.5 );
    test_relation< mutant_relation_builder, double, char   >( 2.5, 'r' );

    test_relation<mutant_relation_builder, int   , int    >(  1 ,   2 );

    test_relation<mutant_relation_builder, std::string, int* >("left value",0);

    test_relation<mutant_relation_builder, cc1, cc2>(0,0);
    test_relation<mutant_relation_builder, cc2, cc3>(0,0);
    test_relation<mutant_relation_builder, cc3, cc4>(0,0);
    test_relation<mutant_relation_builder, cc4, cc5>(0,0);
}

int test_main( int, char* [] )
{

    // Test metadata correctness with untagged relation version
    BOOST_BIMAP_CALL_TEST_STATIC_FUNCTION( tagged_static_test   );

    // Test metadata correctness with tagged relation version
    BOOST_BIMAP_CALL_TEST_STATIC_FUNCTION( untagged_static_test );

    // Test basic
    test_mutant_relation();

    return 0;
}

