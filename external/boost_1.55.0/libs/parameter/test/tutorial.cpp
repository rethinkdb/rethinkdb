// Copyright David Abrahams 2005. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/parameter/keyword.hpp>

namespace graphs
{
   BOOST_PARAMETER_KEYWORD(tag, graph)    // Note: no semicolon
   BOOST_PARAMETER_KEYWORD(tag, visitor)
   BOOST_PARAMETER_KEYWORD(tag, root_vertex)
   BOOST_PARAMETER_KEYWORD(tag, index_map)
   BOOST_PARAMETER_KEYWORD(tag, color_map)
}

namespace graphs { namespace core
{
   template <class ArgumentPack>
   void depth_first_search(ArgumentPack const& args)
   {
       std::cout << "graph:\t" << args[graph] << std::endl;
       std::cout << "visitor:\t" << args[visitor] << std::endl;
       std::cout << "root_vertex:\t" << args[root_vertex] << std::endl;
       std::cout << "index_map:\t" << args[index_map] << std::endl;
       std::cout << "color_map:\t" << args[color_map] << std::endl;
   }
}} // graphs::core

int main()
{
     using namespace graphs;

     core::depth_first_search((
       graph = 'G', visitor = 2, root_vertex = 3.5,
       index_map = "hello, world", color_map = false));
     return 0;
}
