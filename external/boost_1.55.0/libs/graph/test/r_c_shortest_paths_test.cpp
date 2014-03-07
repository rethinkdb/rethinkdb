// Copyright Michael Drexl 2005, 2006.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>

#ifdef BOOST_MSVC
#  pragma warning(disable: 4267)
#endif

#include <boost/graph/adjacency_list.hpp>
//#include <boost/graph/dijkstra_shortest_paths.hpp>

#include <boost/graph/r_c_shortest_paths.hpp>
#include <iostream>
#include <boost/test/minimal.hpp>

using namespace boost;

struct SPPRC_Example_Graph_Vert_Prop
{
  SPPRC_Example_Graph_Vert_Prop( int n = 0, int e = 0, int l = 0 ) 
  : num( n ), eat( e ), lat( l ) {}
  int num;
  // earliest arrival time
  int eat;
  // latest arrival time
  int lat;
};

struct SPPRC_Example_Graph_Arc_Prop
{
  SPPRC_Example_Graph_Arc_Prop( int n = 0, int c = 0, int t = 0 ) 
  : num( n ), cost( c ), time( t ) {}
  int num;
  // traversal cost
  int cost;
  // traversal time
  int time;
};

typedef adjacency_list<vecS, 
                       vecS, 
                       directedS, 
                       SPPRC_Example_Graph_Vert_Prop, 
                       SPPRC_Example_Graph_Arc_Prop> 
  SPPRC_Example_Graph;

// data structures for spp without resource constraints:
// ResourceContainer model
struct spp_no_rc_res_cont
{
  spp_no_rc_res_cont( int c = 0 ) : cost( c ) {};
  spp_no_rc_res_cont& operator=( const spp_no_rc_res_cont& other )
  {
    if( this == &other )
      return *this;
    this->~spp_no_rc_res_cont();
    new( this ) spp_no_rc_res_cont( other );
    return *this;
  }
  int cost;
};

bool operator==( const spp_no_rc_res_cont& res_cont_1, 
                 const spp_no_rc_res_cont& res_cont_2 )
{
  return ( res_cont_1.cost == res_cont_2.cost );
}

bool operator<( const spp_no_rc_res_cont& res_cont_1, 
                const spp_no_rc_res_cont& res_cont_2 )
{
  return ( res_cont_1.cost < res_cont_2.cost );
}

// ResourceExtensionFunction model
class ref_no_res_cont
{
public:
  inline bool operator()( const SPPRC_Example_Graph& g, 
                          spp_no_rc_res_cont& new_cont, 
                          const spp_no_rc_res_cont& old_cont, 
                          graph_traits
                            <SPPRC_Example_Graph>::edge_descriptor ed ) const
  {
    new_cont.cost = old_cont.cost + g[ed].cost;
    return true;
  }
};

// DominanceFunction model
class dominance_no_res_cont
{
public:
  inline bool operator()( const spp_no_rc_res_cont& res_cont_1, 
                          const spp_no_rc_res_cont& res_cont_2 ) const
  {
    // must be "<=" here!!!
    // must NOT be "<"!!!
    return res_cont_1.cost <= res_cont_2.cost;
    // this is not a contradiction to the documentation
    // the documentation says:
    // "A label $l_1$ dominates a label $l_2$ if and only if both are resident 
    // at the same vertex, and if, for each resource, the resource consumption 
    // of $l_1$ is less than or equal to the resource consumption of $l_2$, 
    // and if there is at least one resource where $l_1$ has a lower resource 
    // consumption than $l_2$."
    // one can think of a new label with a resource consumption equal to that 
    // of an old label as being dominated by that old label, because the new 
    // one will have a higher number and is created at a later point in time, 
    // so one can implicitly use the number or the creation time as a resource 
    // for tie-breaking
  }
};
// end data structures for spp without resource constraints:

// data structures for shortest path problem with time windows (spptw)
// ResourceContainer model
struct spp_spptw_res_cont
{
  spp_spptw_res_cont( int c = 0, int t = 0 ) : cost( c ), time( t ) {}
  spp_spptw_res_cont& operator=( const spp_spptw_res_cont& other )
  {
    if( this == &other )
      return *this;
    this->~spp_spptw_res_cont();
    new( this ) spp_spptw_res_cont( other );
    return *this;
  }
  int cost;
  int time;
};

bool operator==( const spp_spptw_res_cont& res_cont_1, 
                 const spp_spptw_res_cont& res_cont_2 )
{
  return ( res_cont_1.cost == res_cont_2.cost 
           && res_cont_1.time == res_cont_2.time );
}

bool operator<( const spp_spptw_res_cont& res_cont_1, 
                const spp_spptw_res_cont& res_cont_2 )
{
  if( res_cont_1.cost > res_cont_2.cost )
    return false;
  if( res_cont_1.cost == res_cont_2.cost )
    return res_cont_1.time < res_cont_2.time;
  return true;
}

// ResourceExtensionFunction model
class ref_spptw
{
public:
  inline bool operator()( const SPPRC_Example_Graph& g, 
                          spp_spptw_res_cont& new_cont, 
                          const spp_spptw_res_cont& old_cont, 
                          graph_traits
                            <SPPRC_Example_Graph>::edge_descriptor ed ) const
  {
    const SPPRC_Example_Graph_Arc_Prop& arc_prop = 
      get( edge_bundle, g )[ed];
    const SPPRC_Example_Graph_Vert_Prop& vert_prop = 
      get( vertex_bundle, g )[target( ed, g )];
    new_cont.cost = old_cont.cost + arc_prop.cost;
    int& i_time = new_cont.time;
    i_time = old_cont.time + arc_prop.time;
    i_time < vert_prop.eat ? i_time = vert_prop.eat : 0;
    return i_time <= vert_prop.lat ? true : false;
  }
};

// DominanceFunction model
class dominance_spptw
{
public:
  inline bool operator()( const spp_spptw_res_cont& res_cont_1, 
                          const spp_spptw_res_cont& res_cont_2 ) const
  {
    // must be "<=" here!!!
    // must NOT be "<"!!!
    return res_cont_1.cost <= res_cont_2.cost 
           && res_cont_1.time <= res_cont_2.time;
    // this is not a contradiction to the documentation
    // the documentation says:
    // "A label $l_1$ dominates a label $l_2$ if and only if both are resident 
    // at the same vertex, and if, for each resource, the resource consumption 
    // of $l_1$ is less than or equal to the resource consumption of $l_2$, 
    // and if there is at least one resource where $l_1$ has a lower resource 
    // consumption than $l_2$."
    // one can think of a new label with a resource consumption equal to that 
    // of an old label as being dominated by that old label, because the new 
    // one will have a higher number and is created at a later point in time, 
    // so one can implicitly use the number or the creation time as a resource 
    // for tie-breaking
  }
};
// end data structures for shortest path problem with time windows (spptw)

int test_main(int, char*[])
{
  SPPRC_Example_Graph g;
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 0, 0, 1000000000 ), g );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 1, 56, 142 ), g );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 2, 0, 1000000000 ), g );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 3, 89, 178 ), g );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 4, 0, 1000000000 ), g );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 5, 49, 76 ), g );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 6, 0, 1000000000 ), g );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 7, 98, 160 ), g );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 8, 0, 1000000000 ), g );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 9, 90, 158 ), g );
  add_edge( 0, 7, SPPRC_Example_Graph_Arc_Prop( 6, 33, 2 ), g );
  add_edge( 0, 6, SPPRC_Example_Graph_Arc_Prop( 5, 31, 6 ), g );
  add_edge( 0, 4, SPPRC_Example_Graph_Arc_Prop( 3, 14, 4 ), g );
  add_edge( 0, 1, SPPRC_Example_Graph_Arc_Prop( 0, 43, 8 ), g );
  add_edge( 0, 4, SPPRC_Example_Graph_Arc_Prop( 4, 28, 10 ), g );
  add_edge( 0, 3, SPPRC_Example_Graph_Arc_Prop( 1, 31, 10 ), g );
  add_edge( 0, 3, SPPRC_Example_Graph_Arc_Prop( 2, 1, 7 ), g );
  add_edge( 0, 9, SPPRC_Example_Graph_Arc_Prop( 7, 25, 9 ), g );
  add_edge( 1, 0, SPPRC_Example_Graph_Arc_Prop( 8, 37, 4 ), g );
  add_edge( 1, 6, SPPRC_Example_Graph_Arc_Prop( 9, 7, 3 ), g );
  add_edge( 2, 6, SPPRC_Example_Graph_Arc_Prop( 12, 6, 7 ), g );
  add_edge( 2, 3, SPPRC_Example_Graph_Arc_Prop( 10, 13, 7 ), g );
  add_edge( 2, 3, SPPRC_Example_Graph_Arc_Prop( 11, 49, 9 ), g );
  add_edge( 2, 8, SPPRC_Example_Graph_Arc_Prop( 13, 47, 5 ), g );
  add_edge( 3, 4, SPPRC_Example_Graph_Arc_Prop( 17, 5, 10 ), g );
  add_edge( 3, 1, SPPRC_Example_Graph_Arc_Prop( 15, 47, 1 ), g );
  add_edge( 3, 2, SPPRC_Example_Graph_Arc_Prop( 16, 26, 9 ), g );
  add_edge( 3, 9, SPPRC_Example_Graph_Arc_Prop( 21, 24, 10 ), g );
  add_edge( 3, 7, SPPRC_Example_Graph_Arc_Prop( 20, 50, 10 ), g );
  add_edge( 3, 0, SPPRC_Example_Graph_Arc_Prop( 14, 41, 4 ), g );
  add_edge( 3, 6, SPPRC_Example_Graph_Arc_Prop( 19, 6, 1 ), g );
  add_edge( 3, 4, SPPRC_Example_Graph_Arc_Prop( 18, 8, 1 ), g );
  add_edge( 4, 5, SPPRC_Example_Graph_Arc_Prop( 26, 38, 4 ), g );
  add_edge( 4, 9, SPPRC_Example_Graph_Arc_Prop( 27, 32, 10 ), g );
  add_edge( 4, 3, SPPRC_Example_Graph_Arc_Prop( 24, 40, 3 ), g );
  add_edge( 4, 0, SPPRC_Example_Graph_Arc_Prop( 22, 7, 3 ), g );
  add_edge( 4, 3, SPPRC_Example_Graph_Arc_Prop( 25, 28, 9 ), g );
  add_edge( 4, 2, SPPRC_Example_Graph_Arc_Prop( 23, 39, 6 ), g );
  add_edge( 5, 8, SPPRC_Example_Graph_Arc_Prop( 32, 6, 2 ), g );
  add_edge( 5, 2, SPPRC_Example_Graph_Arc_Prop( 30, 26, 10 ), g );
  add_edge( 5, 0, SPPRC_Example_Graph_Arc_Prop( 28, 38, 9 ), g );
  add_edge( 5, 2, SPPRC_Example_Graph_Arc_Prop( 31, 48, 10 ), g );
  add_edge( 5, 9, SPPRC_Example_Graph_Arc_Prop( 33, 49, 2 ), g );
  add_edge( 5, 1, SPPRC_Example_Graph_Arc_Prop( 29, 22, 7 ), g );
  add_edge( 6, 1, SPPRC_Example_Graph_Arc_Prop( 34, 15, 7 ), g );
  add_edge( 6, 7, SPPRC_Example_Graph_Arc_Prop( 35, 20, 3 ), g );
  add_edge( 7, 9, SPPRC_Example_Graph_Arc_Prop( 40, 1, 3 ), g );
  add_edge( 7, 0, SPPRC_Example_Graph_Arc_Prop( 36, 23, 5 ), g );
  add_edge( 7, 6, SPPRC_Example_Graph_Arc_Prop( 38, 36, 2 ), g );
  add_edge( 7, 6, SPPRC_Example_Graph_Arc_Prop( 39, 18, 10 ), g );
  add_edge( 7, 2, SPPRC_Example_Graph_Arc_Prop( 37, 2, 1 ), g );
  add_edge( 8, 5, SPPRC_Example_Graph_Arc_Prop( 46, 36, 5 ), g );
  add_edge( 8, 1, SPPRC_Example_Graph_Arc_Prop( 42, 13, 10 ), g );
  add_edge( 8, 0, SPPRC_Example_Graph_Arc_Prop( 41, 40, 5 ), g );
  add_edge( 8, 1, SPPRC_Example_Graph_Arc_Prop( 43, 32, 8 ), g );
  add_edge( 8, 6, SPPRC_Example_Graph_Arc_Prop( 47, 25, 1 ), g );
  add_edge( 8, 2, SPPRC_Example_Graph_Arc_Prop( 44, 44, 3 ), g );
  add_edge( 8, 3, SPPRC_Example_Graph_Arc_Prop( 45, 11, 9 ), g );
  add_edge( 9, 0, SPPRC_Example_Graph_Arc_Prop( 48, 41, 5 ), g );
  add_edge( 9, 1, SPPRC_Example_Graph_Arc_Prop( 49, 44, 7 ), g );
  
  // spp without resource constraints

  std::vector
    <std::vector
      <graph_traits<SPPRC_Example_Graph>::edge_descriptor> > 
        opt_solutions;
  std::vector<spp_no_rc_res_cont> pareto_opt_rcs_no_rc;
  std::vector<int> i_vec_opt_solutions_spp_no_rc;
  //std::cout << "r_c_shortest_paths:" << std::endl;
  for( int s = 0; s < 10; ++s )
  {
    for( int t = 0; t < 10; ++t )
    {
      r_c_shortest_paths
      ( g, 
        get( &SPPRC_Example_Graph_Vert_Prop::num, g ), 
        get( &SPPRC_Example_Graph_Arc_Prop::num, g ), 
        s, 
        t, 
        opt_solutions, 
        pareto_opt_rcs_no_rc, 
        spp_no_rc_res_cont( 0 ), 
        ref_no_res_cont(), 
        dominance_no_res_cont(), 
        std::allocator
          <r_c_shortest_paths_label
            <SPPRC_Example_Graph, spp_no_rc_res_cont> >(), 
        default_r_c_shortest_paths_visitor() );
      i_vec_opt_solutions_spp_no_rc.push_back( pareto_opt_rcs_no_rc[0].cost );
      //std::cout << "From " << s << " to " << t << ": ";
      //std::cout << pareto_opt_rcs_no_rc[0].cost << std::endl;
    }
  }

  //std::vector<graph_traits<SPPRC_Example_Graph>::vertex_descriptor> 
  //  p( num_vertices( g ) );
  //std::vector<int> d( num_vertices( g ) );
  //std::vector<int> i_vec_dijkstra_distances;
  //std::cout << "Dijkstra:" << std::endl;
  //for( int s = 0; s < 10; ++s )
  //{
  //  dijkstra_shortest_paths( g, 
  //                           s, 
  //                           &p[0], 
  //                           &d[0], 
  //                           get( &SPPRC_Example_Graph_Arc_Prop::cost, g ), 
  //                           get( &SPPRC_Example_Graph_Vert_Prop::num, g ), 
  //                           std::less<int>(), 
  //                           closed_plus<int>(), 
  //                           (std::numeric_limits<int>::max)(), 
  //                           0, 
  //                           default_dijkstra_visitor() );
  //  for( int t = 0; t < 10; ++t )
  //  {
  //    i_vec_dijkstra_distances.push_back( d[t] );
  //    std::cout << "From " << s << " to " << t << ": " << d[t] << std::endl;
  //  }
  //}

  std::vector<int> i_vec_correct_solutions;
  i_vec_correct_solutions.push_back( 0 );
  i_vec_correct_solutions.push_back( 22 );
  i_vec_correct_solutions.push_back( 27 );
  i_vec_correct_solutions.push_back( 1 );
  i_vec_correct_solutions.push_back( 6 );
  i_vec_correct_solutions.push_back( 44 );
  i_vec_correct_solutions.push_back( 7 );
  i_vec_correct_solutions.push_back( 27 );
  i_vec_correct_solutions.push_back( 50 );
  i_vec_correct_solutions.push_back( 25 );
  i_vec_correct_solutions.push_back( 37 );
  i_vec_correct_solutions.push_back( 0 );
  i_vec_correct_solutions.push_back( 29 );
  i_vec_correct_solutions.push_back( 38 );
  i_vec_correct_solutions.push_back( 43 );
  i_vec_correct_solutions.push_back( 81 );
  i_vec_correct_solutions.push_back( 7 );
  i_vec_correct_solutions.push_back( 27 );
  i_vec_correct_solutions.push_back( 76 );
  i_vec_correct_solutions.push_back( 28 );
  i_vec_correct_solutions.push_back( 25 );
  i_vec_correct_solutions.push_back( 21 );
  i_vec_correct_solutions.push_back( 0 );
  i_vec_correct_solutions.push_back( 13 );
  i_vec_correct_solutions.push_back( 18 );
  i_vec_correct_solutions.push_back( 56 );
  i_vec_correct_solutions.push_back( 6 );
  i_vec_correct_solutions.push_back( 26 );
  i_vec_correct_solutions.push_back( 47 );
  i_vec_correct_solutions.push_back( 27 );
  i_vec_correct_solutions.push_back( 12 );
  i_vec_correct_solutions.push_back( 21 );
  i_vec_correct_solutions.push_back( 26 );
  i_vec_correct_solutions.push_back( 0 );
  i_vec_correct_solutions.push_back( 5 );
  i_vec_correct_solutions.push_back( 43 );
  i_vec_correct_solutions.push_back( 6 );
  i_vec_correct_solutions.push_back( 26 );
  i_vec_correct_solutions.push_back( 49 );
  i_vec_correct_solutions.push_back( 24 );
  i_vec_correct_solutions.push_back( 7 );
  i_vec_correct_solutions.push_back( 29 );
  i_vec_correct_solutions.push_back( 34 );
  i_vec_correct_solutions.push_back( 8 );
  i_vec_correct_solutions.push_back( 0 );
  i_vec_correct_solutions.push_back( 38 );
  i_vec_correct_solutions.push_back( 14 );
  i_vec_correct_solutions.push_back( 34 );
  i_vec_correct_solutions.push_back( 44 );
  i_vec_correct_solutions.push_back( 32 );
  i_vec_correct_solutions.push_back( 29 );
  i_vec_correct_solutions.push_back( 19 );
  i_vec_correct_solutions.push_back( 26 );
  i_vec_correct_solutions.push_back( 17 );
  i_vec_correct_solutions.push_back( 22 );
  i_vec_correct_solutions.push_back( 0 );
  i_vec_correct_solutions.push_back( 23 );
  i_vec_correct_solutions.push_back( 43 );
  i_vec_correct_solutions.push_back( 6 );
  i_vec_correct_solutions.push_back( 41 );
  i_vec_correct_solutions.push_back( 43 );
  i_vec_correct_solutions.push_back( 15 );
  i_vec_correct_solutions.push_back( 22 );
  i_vec_correct_solutions.push_back( 35 );
  i_vec_correct_solutions.push_back( 40 );
  i_vec_correct_solutions.push_back( 78 );
  i_vec_correct_solutions.push_back( 0 );
  i_vec_correct_solutions.push_back( 20 );
  i_vec_correct_solutions.push_back( 69 );
  i_vec_correct_solutions.push_back( 21 );
  i_vec_correct_solutions.push_back( 23 );
  i_vec_correct_solutions.push_back( 23 );
  i_vec_correct_solutions.push_back( 2 );
  i_vec_correct_solutions.push_back( 15 );
  i_vec_correct_solutions.push_back( 20 );
  i_vec_correct_solutions.push_back( 58 );
  i_vec_correct_solutions.push_back( 8 );
  i_vec_correct_solutions.push_back( 0 );
  i_vec_correct_solutions.push_back( 49 );
  i_vec_correct_solutions.push_back( 1 );
  i_vec_correct_solutions.push_back( 23 );
  i_vec_correct_solutions.push_back( 13 );
  i_vec_correct_solutions.push_back( 37 );
  i_vec_correct_solutions.push_back( 11 );
  i_vec_correct_solutions.push_back( 16 );
  i_vec_correct_solutions.push_back( 36 );
  i_vec_correct_solutions.push_back( 17 );
  i_vec_correct_solutions.push_back( 37 );
  i_vec_correct_solutions.push_back( 0 );
  i_vec_correct_solutions.push_back( 35 );
  i_vec_correct_solutions.push_back( 41 );
  i_vec_correct_solutions.push_back( 44 );
  i_vec_correct_solutions.push_back( 68 );
  i_vec_correct_solutions.push_back( 42 );
  i_vec_correct_solutions.push_back( 47 );
  i_vec_correct_solutions.push_back( 85 );
  i_vec_correct_solutions.push_back( 48 );
  i_vec_correct_solutions.push_back( 68 );
  i_vec_correct_solutions.push_back( 91 );
  i_vec_correct_solutions.push_back( 0 );
  BOOST_CHECK(i_vec_opt_solutions_spp_no_rc.size() == i_vec_correct_solutions.size() );
  for( int i = 0; i < static_cast<int>( i_vec_correct_solutions.size() ); ++i )
    BOOST_CHECK( i_vec_opt_solutions_spp_no_rc[i] == i_vec_correct_solutions[i] );

  // spptw
  std::vector
    <std::vector
      <graph_traits<SPPRC_Example_Graph>::edge_descriptor> > 
        opt_solutions_spptw;
  std::vector<spp_spptw_res_cont> pareto_opt_rcs_spptw;
  std::vector
    <std::vector
      <std::vector
        <std::vector
          <graph_traits<SPPRC_Example_Graph>::edge_descriptor> > > > 
            vec_vec_vec_vec_opt_solutions_spptw( 10 );

  for( int s = 0; s < 10; ++s )
  {
    for( int t = 0; t < 10; ++t )
    {
      r_c_shortest_paths
      ( g, 
        get( &SPPRC_Example_Graph_Vert_Prop::num, g ), 
        get( &SPPRC_Example_Graph_Arc_Prop::num, g ), 
        s, 
        t, 
        opt_solutions_spptw, 
        pareto_opt_rcs_spptw, 
        // be careful, do not simply take 0 as initial value for time
        spp_spptw_res_cont( 0, g[s].eat ), 
        ref_spptw(), 
        dominance_spptw(), 
        std::allocator
          <r_c_shortest_paths_label
            <SPPRC_Example_Graph, spp_spptw_res_cont> >(), 
        default_r_c_shortest_paths_visitor() );
      vec_vec_vec_vec_opt_solutions_spptw[s].push_back( opt_solutions_spptw );
      if( opt_solutions_spptw.size() )
      {
        bool b_is_a_path_at_all = false;
        bool b_feasible = false; 
        bool b_correctly_extended = false;
        spp_spptw_res_cont actual_final_resource_levels( 0, 0 );
        graph_traits<SPPRC_Example_Graph>::edge_descriptor ed_last_extended_arc;
        check_r_c_path( g, 
                        opt_solutions_spptw[0], 
                        spp_spptw_res_cont( 0, g[s].eat ), 
                        true, 
                        pareto_opt_rcs_spptw[0], 
                        actual_final_resource_levels, 
                        ref_spptw(), 
                        b_is_a_path_at_all, 
                        b_feasible, 
                        b_correctly_extended, 
                        ed_last_extended_arc );
        BOOST_CHECK(b_is_a_path_at_all && b_feasible && b_correctly_extended);
        b_is_a_path_at_all = false;
        b_feasible = false; 
        b_correctly_extended = false;
        spp_spptw_res_cont actual_final_resource_levels2( 0, 0 );
        graph_traits<SPPRC_Example_Graph>::edge_descriptor ed_last_extended_arc2;
        check_r_c_path( g, 
                        opt_solutions_spptw[0], 
                        spp_spptw_res_cont( 0, g[s].eat ), 
                        false, 
                        pareto_opt_rcs_spptw[0], 
                        actual_final_resource_levels2, 
                        ref_spptw(), 
                        b_is_a_path_at_all, 
                        b_feasible, 
                        b_correctly_extended, 
                        ed_last_extended_arc2 );
        BOOST_CHECK(b_is_a_path_at_all && b_feasible && b_correctly_extended);
      }
    }
  }

  std::vector<int> i_vec_correct_num_solutions_spptw;
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 3 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 3 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 4 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 3 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 4 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 4 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 3 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 0 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 4 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 4 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 4 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 4 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 5 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 3 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 0 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 3 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 3 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 3 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 4 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  i_vec_correct_num_solutions_spptw.push_back( 3 );
  i_vec_correct_num_solutions_spptw.push_back( 0 );
  i_vec_correct_num_solutions_spptw.push_back( 2 );
  i_vec_correct_num_solutions_spptw.push_back( 3 );
  i_vec_correct_num_solutions_spptw.push_back( 4 );
  i_vec_correct_num_solutions_spptw.push_back( 1 );
  for( int s = 0; s < 10; ++s )
    for( int t = 0; t < 10; ++t )
      BOOST_CHECK( static_cast<int>
            ( vec_vec_vec_vec_opt_solutions_spptw[s][t].size() ) == 
                   i_vec_correct_num_solutions_spptw[10 * s + t] );

  // one pareto-optimal solution
  SPPRC_Example_Graph g2;
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 0, 0, 1000000000 ), g2 );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 1, 0, 1000000000 ), g2 );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 2, 0, 1000000000 ), g2 );
  add_vertex( SPPRC_Example_Graph_Vert_Prop( 3, 0, 1000000000 ), g2 );
  add_edge( 0, 1, SPPRC_Example_Graph_Arc_Prop( 0, 1, 1 ), g2 );
  add_edge( 0, 2, SPPRC_Example_Graph_Arc_Prop( 1, 2, 1 ), g2 );
  add_edge( 1, 3, SPPRC_Example_Graph_Arc_Prop( 2, 3, 1 ), g2 );
  add_edge( 2, 3, SPPRC_Example_Graph_Arc_Prop( 3, 1, 1 ), g2 );
  std::vector<graph_traits<SPPRC_Example_Graph>::edge_descriptor> opt_solution;
  spp_spptw_res_cont pareto_opt_rc;
  r_c_shortest_paths( g2, 
                      get( &SPPRC_Example_Graph_Vert_Prop::num, g2 ), 
                      get( &SPPRC_Example_Graph_Arc_Prop::num, g2 ), 
                      0, 
                      3, 
                      opt_solution, 
                      pareto_opt_rc, 
                      spp_spptw_res_cont( 0, 0 ), 
                      ref_spptw(), 
                      dominance_spptw(), 
                      std::allocator
                        <r_c_shortest_paths_label
                          <SPPRC_Example_Graph, spp_spptw_res_cont> >(), 
                      default_r_c_shortest_paths_visitor() );

  BOOST_CHECK(pareto_opt_rc.cost == 3);

  return 0;
}
