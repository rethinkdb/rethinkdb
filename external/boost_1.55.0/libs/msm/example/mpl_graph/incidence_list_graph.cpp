// Copyright 2008-2010 Gordon Woodhull
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// mplgraph.cpp : Defines the entry point for the console application.
//

#include <boost/msm/mpl_graph/incidence_list_graph.hpp>
#include <boost/msm/mpl_graph/mpl_utils.hpp>

namespace mpl_graph = boost::msm::mpl_graph;
namespace mpl_utils = mpl_graph::mpl_utils;
namespace mpl = boost::mpl;
/* 
    test graph:
    A -> B -> C -\--> D
           \     |--> E
            \    \--> F
             \-----/
*/           

// vertices
struct A{}; struct B{}; struct C{}; struct D{}; struct E{}; struct F{};

// edges
struct A_B{}; struct B_C{}; struct C_D{}; struct C_E{}; struct C_F{}; struct B_F{};

typedef mpl::vector<mpl::vector<A_B,A,B>,
               mpl::vector<B_C,B,C>,
               mpl::vector<C_D,C,D>,
               mpl::vector<C_E,C,E>,
               mpl::vector<C_F,C,F>,
               mpl::vector<B_F,B,F> >
    some_incidence_list;
typedef mpl_graph::incidence_list_graph<some_incidence_list> some_graph;

BOOST_MPL_ASSERT(( boost::is_same<mpl_graph::source<B_C,some_graph>::type, B> ));
BOOST_MPL_ASSERT(( boost::is_same<mpl_graph::source<C_D,some_graph>::type, C> ));

BOOST_MPL_ASSERT(( boost::is_same<mpl_graph::target<C_D,some_graph>::type, D> ));
BOOST_MPL_ASSERT(( boost::is_same<mpl_graph::target<B_F,some_graph>::type, F> ));

BOOST_MPL_ASSERT(( mpl_utils::set_equal<mpl_graph::out_edges<C,some_graph>::type, mpl::vector<C_D,C_E,C_F> > ));
BOOST_MPL_ASSERT(( mpl_utils::set_equal<mpl_graph::out_edges<B,some_graph>::type, mpl::vector<B_F,B_C> > ));

BOOST_MPL_ASSERT_RELATION( (mpl_graph::out_degree<B,some_graph>::value), ==, 2 );
BOOST_MPL_ASSERT_RELATION( (mpl_graph::out_degree<C,some_graph>::value), ==, 3 );

BOOST_MPL_ASSERT(( mpl_utils::set_equal<mpl_graph::in_edges<C,some_graph>::type, mpl::vector<B_C> > ));
BOOST_MPL_ASSERT(( mpl_utils::set_equal<mpl_graph::in_edges<F,some_graph>::type, mpl::vector<C_F,B_F> > ));

BOOST_MPL_ASSERT_RELATION( (mpl_graph::in_degree<A,some_graph>::value), ==, 0 );
BOOST_MPL_ASSERT_RELATION( (mpl_graph::in_degree<F,some_graph>::value), ==, 2 );

BOOST_MPL_ASSERT_RELATION( (mpl_graph::degree<A,some_graph>::value), ==, 1 );
BOOST_MPL_ASSERT_RELATION( (mpl_graph::degree<C,some_graph>::value), ==, 4 );

BOOST_MPL_ASSERT(( mpl_utils::set_equal<mpl_graph::adjacent_vertices<A,some_graph>::type, mpl::vector<B> > ));
BOOST_MPL_ASSERT(( mpl_utils::set_equal<mpl_graph::adjacent_vertices<C,some_graph>::type, mpl::vector<D,E,F> > ));

BOOST_MPL_ASSERT(( mpl_utils::set_equal<mpl_graph::vertices<some_graph>::type, mpl::vector<A,B,C,D,E,F> > ));

BOOST_MPL_ASSERT_RELATION( mpl_graph::num_vertices<some_graph>::value, ==, 6 );

BOOST_MPL_ASSERT_RELATION( mpl_graph::num_edges<some_graph>::value, ==, 6 );


int main(int argc, char* argv[])
{
    return 0;
}

