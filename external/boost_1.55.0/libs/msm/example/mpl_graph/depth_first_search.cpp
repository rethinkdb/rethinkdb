// Copyright 2008-2010 Gordon Woodhull
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/msm/mpl_graph/depth_first_search.hpp>
#include <boost/msm/mpl_graph/adjacency_list_graph.hpp>
#include <boost/msm/mpl_graph/incidence_list_graph.hpp>

#include <iostream>

namespace mpl_graph = boost::msm::mpl_graph;
namespace mpl = boost::mpl;

// vertices
struct A{}; struct B{}; struct C{}; struct D{}; struct E{}; struct F{}; struct G{};

// edges
struct A_B{}; struct B_C{}; struct C_D{}; struct C_E{}; struct C_F{}; struct B_F{};



/* 
    incidence list test graph:
    A -> B -> C -\--> D
           \     |--> E
            \    \--> F
             \-----/
    G
*/           

typedef mpl::vector<mpl::vector<A_B,A,B>,
               mpl::vector<B_C,B,C>,
               mpl::vector<C_D,C,D>,
               mpl::vector<C_E,C,E>,
               mpl::vector<C_F,C,F>,
               mpl::vector<B_F,B,F> >
    some_incidence_list;
typedef mpl_graph::incidence_list_graph<some_incidence_list> some_incidence_list_graph;



/* 
    adjacency list test graph:
    A -> B -> C -\--> D
           \     |--> E
            \    \--> F
             \-----/
    G
*/           

typedef mpl::vector<
            mpl::pair<A, mpl::vector<mpl::pair<A_B, B> > >,
            mpl::pair<B, mpl::vector<mpl::pair<B_C, C>,
                                     mpl::pair<B_F, F> > >,
            mpl::pair<C, mpl::vector<mpl::pair<C_D, D>,
                                     mpl::pair<C_E, E>,
                                     mpl::pair<C_F, F> > >,
            mpl::pair<G, mpl::vector<> > >
    some_adjacency_list;
typedef mpl_graph::adjacency_list_graph<some_adjacency_list> some_adjacency_list_graph;


struct preordering_visitor : mpl_graph::dfs_default_visitor_operations {    
    template<typename Node, typename Graph, typename State>
    struct discover_vertex :
        mpl::push_back<State, Node>
    {};
};

struct postordering_visitor : mpl_graph::dfs_default_visitor_operations {    
    template<typename Node, typename Graph, typename State>
    struct finish_vertex :
        mpl::push_back<State, Node>
    {};
};
  
// adjacency list tests

// preordering, default start node (first)
typedef mpl::first<mpl_graph::
    depth_first_search_all<some_adjacency_list_graph, 
                           preordering_visitor, 
                           mpl::vector<> >::type>::type 
                    preorder_adj;
BOOST_MPL_ASSERT(( mpl::equal<preorder_adj::type, mpl::vector<A,B,C,D,E,F,G> > ));

// postordering, default start node
typedef mpl::first<mpl_graph::
    depth_first_search_all<some_adjacency_list_graph, 
                           postordering_visitor, 
                           mpl::vector<> >::type>::type 
                    postorder_adj;
BOOST_MPL_ASSERT(( mpl::equal<postorder_adj::type, mpl::vector<D,E,F,C,B,A,G> > ));

// preordering all starting at C
typedef mpl::first<mpl_graph::
    depth_first_search_all<some_adjacency_list_graph,
                           preordering_visitor, 
                           mpl::vector<>,
                           C>::type>::type 
                    preorder_adj_all_from_c;
BOOST_MPL_ASSERT(( mpl::equal<preorder_adj_all_from_c::type, mpl::vector<C,D,E,F,A,B,G> > ));

// preordering just those starting at C
typedef mpl::first<mpl_graph::
    depth_first_search<some_adjacency_list_graph,
                       preordering_visitor, 
                       mpl::vector<>,
                       C>::type>::type 
                preorder_adj_from_c;
BOOST_MPL_ASSERT(( mpl::equal<preorder_adj_from_c::type, mpl::vector<C,D,E,F> > ));


// incidence list tests

// preordering, default start node (first)
typedef mpl::first<mpl_graph::
    depth_first_search_all<some_incidence_list_graph,
                           preordering_visitor,
                           mpl::vector<> >::type>::type 
                    preorder_inc;
BOOST_MPL_ASSERT(( mpl::equal<preorder_inc::type, mpl::vector<A,B,C,D,E,F> > ));

// postordering, default start node
typedef mpl::first<mpl_graph::
    depth_first_search_all<some_incidence_list_graph, 
                           postordering_visitor,
                           mpl::vector<> >::type>::type 
                    postorder_inc;
BOOST_MPL_ASSERT(( mpl::equal<postorder_inc::type, mpl::vector<D,E,F,C,B,A> > ));

// preordering starting at C
typedef mpl::first<mpl_graph::
    depth_first_search_all<some_incidence_list_graph,
                           preordering_visitor,
                           mpl::vector<>,
                           C>::type>::type 
                    preorder_inc_all_from_c;
BOOST_MPL_ASSERT(( mpl::equal<preorder_inc_all_from_c::type, mpl::vector<C,D,E,F,A,B> > ));

// preordering starting at B
typedef mpl::first<mpl_graph::
    depth_first_search<some_incidence_list_graph,
                       preordering_visitor,
                       mpl::vector<>,
                       B>::type>::type 
                preorder_inc_from_b;
BOOST_MPL_ASSERT(( mpl::equal<preorder_inc_from_b::type, mpl::vector<B,C,D,E,F> > ));


int main() {
    return 0;
}