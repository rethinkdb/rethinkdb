// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_is_subconcept.cpp 80882 2012-10-06 00:08:26Z steven_watanabe $

#include <boost/type_erasure/is_subconcept.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/assert.hpp>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;

BOOST_MPL_ASSERT((is_subconcept<typeid_<>, typeid_<> >));
BOOST_MPL_ASSERT_NOT((is_subconcept<typeid_<>, incrementable<> >));
BOOST_MPL_ASSERT_NOT((is_subconcept<mpl::vector<typeid_<>, incrementable<> >, typeid_<> >));
BOOST_MPL_ASSERT_NOT((is_subconcept<mpl::vector<typeid_<>, incrementable<> >, incrementable<> >));
BOOST_MPL_ASSERT((is_subconcept<typeid_<>, mpl::vector<typeid_<>, incrementable<> > >));
BOOST_MPL_ASSERT((is_subconcept<incrementable<>, mpl::vector<typeid_<>, incrementable<> > >));
BOOST_MPL_ASSERT((is_subconcept<mpl::vector<typeid_<>, incrementable<> >, mpl::vector<incrementable<>, typeid_<> > >));

BOOST_MPL_ASSERT((is_subconcept<typeid_<_a>, typeid_<_b>, mpl::map<mpl::pair<_a, _b> > >));
BOOST_MPL_ASSERT_NOT((is_subconcept<typeid_<_a>, incrementable<_b>, mpl::map<mpl::pair<_a, _b> > >));
BOOST_MPL_ASSERT_NOT((is_subconcept<mpl::vector<typeid_<_a>, incrementable<_a> >, typeid_<_b>, mpl::map<mpl::pair<_a, _b> > >));
BOOST_MPL_ASSERT_NOT((is_subconcept<mpl::vector<typeid_<_a>, incrementable<_a> >, incrementable<_b>, mpl::map<mpl::pair<_a, _b> > >));
BOOST_MPL_ASSERT((is_subconcept<typeid_<_a>, mpl::vector<typeid_<_b>, incrementable<_b> >, mpl::map<mpl::pair<_a, _b> > >));
BOOST_MPL_ASSERT((is_subconcept<incrementable<_a>, mpl::vector<typeid_<_b>, incrementable<_b> >, mpl::map<mpl::pair<_a, _b> > >));
BOOST_MPL_ASSERT((is_subconcept<mpl::vector<typeid_<_a>, incrementable<_a> >, mpl::vector<incrementable<_b>, typeid_<_b> >, mpl::map<mpl::pair<_a, _b> > >));
