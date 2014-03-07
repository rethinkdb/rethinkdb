//=======================================================================
// Copyright 2002 Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

// This is meant to be executed from the current directory

std::string container_types [] = { "vecS", "listS", "setS" };
const int N = sizeof(container_types)/sizeof(std::string);

std::string directed_types[] = { "bidirectionalS", "directedS", "undirectedS"};
const int D = sizeof(directed_types)/sizeof(std::string);

int
main()
{
  int i, j, k, ret = 0, rc, t = 0;
  for (i = 0; i < N; ++i)
    for (j = 0; j < N; ++j)
      for (k = 0; k < D; ++k, ++t) {

        std::string file_name = "graph_type.hpp";
        system("rm -f graph_type.hpp");
        std::ofstream header(file_name.c_str());
        if (!header) {
          std::cerr << "could not open file " << file_name << std::endl;
          return -1;
        }

        header << "#include <boost/graph/adjacency_list.hpp>" << std::endl
               << "typedef boost::adjacency_list<boost::" << container_types[i]
               << ", boost::" << container_types[j]
               << ", boost::" << directed_types[k]
               << ", boost::property<vertex_id_t, std::size_t>"
               << ", boost::property<edge_id_t, std::size_t> > Graph;"
               << std::endl
               << "typedef boost::property<vertex_id_t, std::size_t> VertexId;"
               << std::endl
               << "typedef boost::property<edge_id_t, std::size_t> EdgeID;"
               << std::endl;
        system("rm -f graph.exe graph.o graph.obj");
        // the following system call should be replaced by a
        // portable "compile" command.
        //const char* compile = "g++ -I.. -O2 -Wall -Wno-long-long -ftemplate-depth-30 ../libs/graph/test/graph.cpp -o graph.exe";
        const char* compile = "g++ -I/u/jsiek/boost graph.cpp -o graph.exe";
        std::cout << compile << std::endl;
        rc = system(compile);
        if (rc != 0) {
          std::cerr << "compile failed for " << container_types[i] << " "
                    << container_types[j] << " " << directed_types[k]
                    << std::endl;
          ret = -1;
        } else {
          rc = system("./graph.exe");
          if (rc != 0) {
            std::cerr << "run failed for " << container_types[i] << " "
                      << container_types[j] << " " << directed_types[k]
                      << std::endl;
            ret = -1;
          } else {
            std::cout << (t+1) << " of " << (N*N*D) << " tests passed." 
                      << std::endl;
          }
        }
      }
  
  return ret;
}
