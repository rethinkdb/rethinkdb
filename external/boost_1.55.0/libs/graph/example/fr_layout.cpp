// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#include <boost/graph/fruchterman_reingold.hpp>
#include <boost/graph/random_layout.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topology.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <boost/random/linear_congruential.hpp>
#include <boost/progress.hpp>
#include <boost/shared_ptr.hpp>

using namespace boost;

void usage()
{
  std::cerr << "Usage: fr_layout [options] <width> <height>\n"
            << "Arguments:\n"
            << "\t<width>\tWidth of the display area (floating point)\n"
            << "\t<Height>\tHeight of the display area (floating point)\n\n"
            << "Options:\n"
            << "\t--iterations n\tNumber of iterations to execute.\n" 
            << "\t\t\tThe default value is 100.\n"
            << "Input:\n"
            << "  Input is read from standard input as a list of edges, one per line.\n"
            << "  Each edge contains two string labels (the endpoints) separated by a space.\n\n"
            << "Output:\n"
            << "  Vertices and their positions are written to standard output with the label,\n  x-position, and y-position of a vertex on each line, separated by spaces.\n";
}

typedef boost::rectangle_topology<> topology_type;
typedef topology_type::point_type point_type;

typedef adjacency_list<listS, vecS, undirectedS, 
                       property<vertex_name_t, std::string> > Graph;

typedef graph_traits<Graph>::vertex_descriptor Vertex;

typedef std::map<std::string, Vertex> NameToVertex;

Vertex get_vertex(const std::string& name, Graph& g, NameToVertex& names)
{
  NameToVertex::iterator i = names.find(name);
  if (i == names.end())
    i = names.insert(std::make_pair(name, add_vertex(name, g))).first;
  return i->second;
}

class progress_cooling : public linear_cooling<double>
{
  typedef linear_cooling<double> inherited;

 public:
  explicit progress_cooling(std::size_t iterations) : inherited(iterations) 
  {
    display.reset(new progress_display(iterations + 1, std::cerr));
  }

  double operator()()
  {
    ++(*display);
    return inherited::operator()();
  }

 private:
  shared_ptr<boost::progress_display> display;
};

int main(int argc, char* argv[])
{
  int iterations = 100;

  if (argc < 3) { usage(); return -1; }

  double width = 0;
  double height = 0;

  for (int arg_idx = 1; arg_idx < argc; ++arg_idx) {
    std::string arg = argv[arg_idx];
    if (arg == "--iterations") {
      ++arg_idx;
      if (arg_idx >= argc) { usage(); return -1; }
      iterations = lexical_cast<int>(argv[arg_idx]);
    } else {
      if (width == 0.0) width = lexical_cast<double>(arg);
      else if (height == 0.0) height = lexical_cast<double>(arg);
      else {
        usage();
        return -1;
      }
    }
  }

  if (width == 0.0 || height == 0.0) {
    usage();
    return -1;
  }

  Graph g;
  NameToVertex names;

  std::string source, target;
  while (std::cin >> source >> target) {
    add_edge(get_vertex(source, g, names), get_vertex(target, g, names), g);
  }
  
  typedef std::vector<point_type> PositionVec;
  PositionVec position_vec(num_vertices(g));
  typedef iterator_property_map<PositionVec::iterator, 
                                property_map<Graph, vertex_index_t>::type>
    PositionMap;
  PositionMap position(position_vec.begin(), get(vertex_index, g));

  minstd_rand gen;
  topology_type topo(gen, -width/2, -height/2, width/2, height/2);
  random_graph_layout(g, position, topo);
  fruchterman_reingold_force_directed_layout
    (g, position, topo,
     cooling(progress_cooling(iterations)));

  graph_traits<Graph>::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    std::cout << get(vertex_name, g, *vi) << '\t'
              << position[*vi][0] << '\t' << position[*vi][1] << std::endl;
  }
  return 0;
}
