// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_BIMAP_TEST_TEST_RELATION_HPP
#define BOOST_BIMAP_TEST_TEST_RELATION_HPP

#if defined(_MSC_VER) && (_MSC_VER>=1200)
#pragma once
#endif

#include <boost/config.hpp>

// Boost.Test
#include <boost/test/minimal.hpp>

// Boost.MPL
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

// Boost.Bimap
#include <boost/bimap/detail/test/check_metadata.hpp>
#include <boost/bimap/tags/tagged.hpp>

// Boost.Bimap.Relation
#include <boost/bimap/relation/member_at.hpp>
#include <boost/bimap/relation/support/get.hpp>
#include <boost/bimap/relation/support/pair_by.hpp>
#include <boost/bimap/relation/support/pair_type_by.hpp>
#include <boost/bimap/relation/support/value_type_of.hpp>
#include <boost/bimap/relation/support/member_with_tag.hpp>
#include <boost/bimap/relation/support/is_tag_of_member_at.hpp>



template< class Relation >
void test_relation_with_default_tags(Relation & rel,
    const typename Relation::left_value_type  & lv,
    const typename Relation::right_value_type & rv)
{

    using namespace boost::bimaps::relation::support;
    using namespace boost::bimaps::relation;
    using namespace boost::bimaps::tags;

    // It must work with normal tags

    BOOST_CHECK( pair_by<member_at::left >(rel).first  == lv );
    BOOST_CHECK( pair_by<member_at::left >(rel).second == rv );

    BOOST_CHECK( pair_by<member_at::right>(rel).first  == rv );
    BOOST_CHECK( pair_by<member_at::right>(rel).second == lv );

    BOOST_CHECK( get<member_at::left >(rel) == rel.left  );
    BOOST_CHECK( get<member_at::right>(rel) == rel.right );

    BOOST_CHECK(
        get<member_at::left >(pair_by<member_at::left >(rel)) == rel.left
    );

    BOOST_CHECK(
        get<member_at::right>(pair_by<member_at::left >(rel)) == rel.right
    );

    BOOST_CHECK(
        get<member_at::left >(pair_by<member_at::right>(rel)) == rel.left
    );

    BOOST_CHECK(
        get<member_at::right>(pair_by<member_at::right>(rel)) == rel.right
    );

}

template< class Relation, class LeftTag, class RightTag >
void test_relation_with_user_tags(Relation & rel,
                   const typename Relation::left_value_type  & lv,
                   const typename Relation::right_value_type & rv)
{

    using namespace boost::bimaps::relation::support;
    using namespace boost::bimaps::relation;
    using namespace boost::bimaps::tags;

    // And with users ones

    BOOST_CHECK( pair_by<LeftTag >(rel).first   == lv );
    BOOST_CHECK( pair_by<LeftTag >(rel).second  == rv );

    BOOST_CHECK( pair_by<RightTag>(rel).first   == rv );
    BOOST_CHECK( pair_by<RightTag>(rel).second  == lv );

    BOOST_CHECK( get<LeftTag >(rel) == rel.left  );
    BOOST_CHECK( get<RightTag>(rel) == rel.right );

    BOOST_CHECK( get<LeftTag >(pair_by<LeftTag >(rel)) == rel.left  );
    BOOST_CHECK( get<RightTag>(pair_by<LeftTag >(rel)) == rel.right );

    BOOST_CHECK( get<LeftTag >(pair_by<RightTag>(rel)) == rel.left  );
    BOOST_CHECK( get<RightTag>(pair_by<RightTag>(rel)) == rel.right );

    //----------------------------------------------------------------

    BOOST_CHECK( rel.template get<LeftTag >() == rel.left  );
    BOOST_CHECK( rel.template get<RightTag>() == rel.right );

    BOOST_CHECK( pair_by<LeftTag >(rel).template get<LeftTag >()== rel.left );
    BOOST_CHECK( pair_by<LeftTag >(rel).template get<RightTag>()== rel.right);

    BOOST_CHECK( pair_by<RightTag>(rel).template get<LeftTag >()== rel.left );
    BOOST_CHECK( pair_by<RightTag>(rel).template get<RightTag>()== rel.right);
}

struct  left_user_tag {};
struct right_user_tag {};

template< class RelationBuilder, class LeftData, class RightData >
void test_relation(const LeftData & lv, const RightData & rv)
{
    using namespace boost::bimaps::relation::support;
    using namespace boost::bimaps::relation;
    using boost::bimaps::tags::tagged;

    // Untagged test
    {
        typedef typename RelationBuilder::template build
        <
            LeftData,
            RightData

        >::type rel_type;

        rel_type rel( lv, rv );

        test_relation_with_default_tags( rel, lv, rv);
    }

    // Tagged test
    {
        typedef typename RelationBuilder::template build
        <
            tagged<LeftData , left_user_tag  >,
            tagged<RightData, right_user_tag >

        >::type rel_type;

        rel_type rel( lv, rv );

        test_relation_with_default_tags(rel, lv, rv );
        test_relation_with_user_tags
        <
            rel_type,
            left_user_tag,right_user_tag

        >(rel,lv,rv);
    }

    // Default Constructor, Constructor from views and some operators
    {
/*
        typedef typename RelationBuilder::template build
        <
            tagged<LeftData , left_user_tag  >,
            tagged<RightData, right_user_tag >

        >::type rel_type;

        typedef typename pair_type_by< left_user_tag,rel_type>::type  left_pair;
        typedef typename pair_type_by<right_user_tag,rel_type>::type right_pair;

        rel_type rel_from_left (  left_pair(lv,rv) );
        rel_type rel_from_right( right_pair(rv,lv) );

        BOOST_CHECK( rel_from_left == rel_from_right  );
        BOOST_CHECK( rel_from_left == rel_type(lv,rv) );

        rel_type rel;

        rel = rel_from_left;

        BOOST_CHECK( rel == rel_from_left );
*/
    }

}

#endif // BOOST_BIMAP_TEST_TEST_RELATION_HPP
