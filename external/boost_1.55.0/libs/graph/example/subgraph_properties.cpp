//  (C) Copyright Jeremy Siek 2004 
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

/*
  Sample output:

  After initializing properties for G1:
          Global and local properties for vertex G0[C]...
                  G0[C]= A1
                  G1[A1]= A1
          Global and local properties for vertex G0[E]...
                  G0[E]= B1
                  G1[B1]= B1
          Global and local properties for vertex G0[F]...
                  G0[F]= C1
                  G1[C1]= C1


  After initializing properties for G2:
          Global and local properties for vertex G0[A]
                  G0[A]= A2
                  G2[A2]= A2
          Global and local properties for vertex G0[C]...
                  G0[C]= B2
                  G1[A1]= B2
                  G2[B2]= B2

 */

#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/subgraph.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>

int main(int,char*[])
{
  using namespace boost;
  //typedef adjacency_list_traits<vecS, vecS, directedS> Traits;// Does nothing?
  typedef property<   vertex_color_t, int,
    property< vertex_name_t, std::string > > VertexProperty;
  
  typedef subgraph< adjacency_list<  vecS, vecS, directedS,
    VertexProperty, property<edge_index_t, int> > > Graph;
  
  const int N = 6;
  Graph G0(N);
  enum { A, B, C, D, E, F};     // for conveniently refering to vertices in G0
  
  property_map<Graph, vertex_name_t>::type name = get(vertex_name_t(), G0);
  name[A] = "A";
  name[B] = "B";
  name[C] = "C";
  name[D] = "D";
  name[E] = "E";
  name[F] = "F";
  
  Graph& G1 = G0.create_subgraph();
  enum { A1, B1, C1 };          // for conveniently refering to vertices in G1
  
  add_vertex(C, G1); // global vertex C becomes local A1 for G1
  add_vertex(E, G1); // global vertex E becomes local B1 for G1
  add_vertex(F, G1); // global vertex F becomes local C1 for G1
  
  property_map<Graph, vertex_name_t>::type name1 = get(vertex_name_t(), G1);
  name1[A1] = "A1";
  
  std::cout << std::endl << "After initializing properties for G1:" << std::endl;
  std::cout << "  Global and local properties for vertex G0[C]..." << std::endl;
  std::cout << "    G0[C]= " << boost::get(vertex_name, G0, vertex(C, G0)) << std::endl;// prints: "G0[C]= C"
  std::cout << "    G1[A1]= " << boost::get(vertex_name, G1, vertex(A1, G1)) << std::endl;// prints: "G1[A1]= A1"
  
  name1[B1] = "B1";
  
  std::cout << "  Global and local properties for vertex G0[E]..." << std::endl;
  std::cout << "    G0[E]= " << boost::get(vertex_name, G0, vertex(E, G0)) << std::endl;// prints: "G0[E]= E"
  std::cout << "    G1[B1]= " << boost::get(vertex_name, G1, vertex(B1, G1)) << std::endl;// prints: "G1[B1]= B1"
  
  name1[C1] = "C1";
  
  std::cout << "  Global and local properties for vertex G0[F]..." << std::endl;
  std::cout << "    G0[F]= " << boost::get(vertex_name, G0, vertex(F, G0)) << std::endl;// prints: "G0[F]= F"
  std::cout << "    G1[C1]= " << boost::get(vertex_name, G1, vertex(C1, G1)) << std::endl;// prints: "G1[C1]= C1"
  
  Graph& G2 = G0.create_subgraph();
  enum { A2, B2 };              // for conveniently refering to vertices in G2
  
  add_vertex(A, G2); // global vertex A becomes local A2 for G2
  add_vertex(C, G2); // global vertex C becomes local B2 for G2
  
  property_map<Graph, vertex_name_t>::type name2 = get(vertex_name_t(), G2);
  name2[A2] = "A2";
  
  std::cout << std::endl << std::endl << "After initializing properties for G2:" << std::endl;
  std::cout << "  Global and local properties for vertex G0[A]" << std::endl;
  std::cout << "    G0[A]= " << boost::get(vertex_name, G0, vertex(A, G0)) << std::endl;// prints: "G0[A]= A"
  std::cout << "    G2[A2]= " << boost::get(vertex_name, G2, vertex(A2, G2)) << std::endl;// prints: "G2[A2]= A2"
  
  name2[B2] = "B2";
  
  std::cout << "  Global and local properties for vertex G0[C]..." << std::endl;
  std::cout << "    G0[C]= " << boost::get(vertex_name, G0, vertex(C, G0)) << std::endl;// prints: "G0[C]= C"
  std::cout << "    G1[A1]= " << boost::get(vertex_name, G1, vertex(A1, G1)) << std::endl;// prints: "G1[A1]= A1"
  std::cout << "    G2[B2]= " << boost::get(vertex_name, G2, vertex(B2, G2)) << std::endl;// prints: "G2[B2]= B2"
  
  return 0;
}
