//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

/*
  Sample data (edge_iterator_constructor.dat):
  5
  10
  0 1
  1 2
  1 3
  2 4
  3 4
  1 0
  2 1
  3 1
  4 2
  4 3

  Sample output:

  0 --> 1 
  1 --> 2 3 0 
  2 --> 4 1 
  3 --> 4 1 
  4 --> 2 3 

 */

#include <boost/config.hpp>
#include <utility>
#include <iostream>
#include <fstream>

#include <iterator>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/adjacency_list.hpp>

class edge_stream_iterator {
public:
  typedef std::input_iterator_tag  iterator_category;
  typedef std::pair<int,int>  value_type;
  typedef std::ptrdiff_t      difference_type;
  typedef const value_type*   pointer;
  typedef const value_type&   reference;

  edge_stream_iterator() : m_stream(0), m_end_marker(false) {}
  edge_stream_iterator(std::istream& s) : m_stream(&s) { m_read(); }
  reference operator*() const { return m_edge; }
  edge_stream_iterator& operator++() { 
    m_read(); 
    return *this;
  }
  edge_stream_iterator operator++(int)  {
    edge_stream_iterator tmp = *this;
    m_read();
    return tmp;
  }
protected:
  std::istream* m_stream;
  value_type m_edge;
  bool m_end_marker;
  void m_read() {
    m_end_marker = (*m_stream) ? true : false;
    if (m_end_marker) {
      *m_stream >> m_edge.first >> m_edge.second;
    }
    m_end_marker = (*m_stream) ? true : false;
  }
  friend bool operator==(const edge_stream_iterator& x,
                         const edge_stream_iterator& y);

};
bool operator==(const edge_stream_iterator& x,
                const edge_stream_iterator& y)
{
  return (x.m_stream == y.m_stream && x.m_end_marker == y.m_end_marker) 
    || x.m_end_marker == false && y.m_end_marker == false;
}
bool operator!=(const edge_stream_iterator& x,
                const edge_stream_iterator& y)
{
  return !(x == y);
}



int
main()
{
  typedef boost::adjacency_list<> IteratorConstructibleGraph;
  typedef boost::graph_traits<IteratorConstructibleGraph> Traits;
  Traits::vertices_size_type size_V;
  Traits::edges_size_type size_E;

  std::ifstream f("edge_iterator_constructor.dat");
  f >> size_V >> size_E;

  edge_stream_iterator edge_iter(f), end;
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ can't handle the iterator constructor
  IteratorConstructibleGraph G(size_V);
  while (edge_iter != end) {
    int i, j;
    boost::tie(i, j) = *edge_iter++;
    boost::add_edge(i, j, G);
  }
#else
  IteratorConstructibleGraph G(edge_iter, end, size_V);
#endif
  boost::print_graph(G);

  return 0;
}
