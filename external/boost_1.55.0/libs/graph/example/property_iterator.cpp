
// (C) Copyright Francois Faure, iMAGIS-GRAVIR / UJF, 2001. 

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History:
// 03 May 2001   Jeremy Siek
//      Moved property iterator code to headers. 
// 02 May 2001   Francois Faure
//     Initial version.

#include <boost/graph/adjacency_list_io.hpp>
#include <boost/graph/property_iter_range.hpp>
#include <fstream>
#include <algorithm>


using namespace boost;

//======== vertex properties
struct toto_t {
  enum { num = 23063};
  typedef vertex_property_tag kind;
};
typedef property< toto_t, double > Toto;

struct radius_t {
  enum { num = 23062};
  typedef vertex_property_tag kind;
};
typedef property< radius_t, double, Toto > Radius;

struct mass_t {
  enum { num = 23061};
  typedef vertex_property_tag kind;
};
typedef property< mass_t, int, Radius > Mass;


//====== edge properties
struct stiff_t {
  enum { num = 23064};
  typedef edge_property_tag kind;
};
typedef property<stiff_t, double> Stiff;



//===== graph type
typedef Mass VertexProperty;
typedef Stiff EdgeProperty;
typedef adjacency_list<vecS, setS, bidirectionalS, 
  VertexProperty, EdgeProperty> Graph;


//===== utilities
struct Print
{
  template<class T>
  Print& operator() (const T& t) {
    std::cout << t << " "; 
    return (*this);
  }
};

template<class T>
struct Set
{
  T value;
  
  Set( const T& t ):value(t){}
  
  Set& operator() (T& t) {
    t=value; 
    return (*this);
  }
};


//===== program
int main(int argc, char* argv[])
{
  if (argc < 2) {
    std::cerr<<"args: file"<<std::endl; 
    return EXIT_FAILURE;
  }
  
  std::ifstream readFile(argv[1]);
  
  Graph graph;
  readFile >> read( graph );
  std::cout << write( graph );
  
  std::cout << "radii:" << std::endl;
  graph_property_iter_range<Graph,radius_t>::type 
    seqRadius = get_property_iter_range(graph,radius_t());
  std::for_each( seqRadius.first, seqRadius.second, Print() ); 
  std::cout << std::endl;
  
  std::cout << "stiff:" << std::endl;
  graph_property_iter_range<Graph,stiff_t>::type
    seqStiff = get_property_iter_range(graph, stiff_t());
  std::for_each( seqStiff.first, seqStiff.second, Print() ); 
  std::cout << std::endl;
  
  seqStiff = get_property_iter_range(graph, stiff_t());
  std::for_each( seqStiff.first, seqStiff.second, Set<double>(2.4) );
  
  std::cout << "new stiff:" << std::endl;
  seqStiff = get_property_iter_range(graph,stiff_t());
  std::for_each( seqStiff.first, seqStiff.second, Print() ); 
  std::cout << std::endl;
  
  return 0;
}
