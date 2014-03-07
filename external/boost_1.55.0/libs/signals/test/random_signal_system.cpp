// Boost.Signals library

// Copyright Douglas Gregor 2001-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/signal.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/random.hpp>
#include <map>
#include <set>
#include <stdlib.h>
#include <time.h>
#include <boost/test/minimal.hpp>

using namespace boost;
using namespace boost::signals;

struct signal_tag {
  typedef vertex_property_tag kind;
};

struct connection_tag {
  typedef edge_property_tag kind;
};

typedef signal4<void, int, int, double, int&> signal_type;
typedef adjacency_list<listS, listS, directedS,
                       // Vertex properties
                       property<signal_tag, signal_type*,
  //                       property<vertex_color_t, default_color_type,
                       property<vertex_index_t, int> >,
                       // Edge properties
                       property<connection_tag, connection,
                       property<edge_weight_t, int> > >
  signal_graph_type;
typedef signal_graph_type::vertex_descriptor vertex_descriptor;
typedef signal_graph_type::edge_descriptor edge_descriptor;

// The signal graph
static signal_graph_type signal_graph;

// Mapping from a signal to its associated vertex descriptor
static std::map<signal_type*, vertex_descriptor> signal_to_descriptor;

// Mapping from a connection to its associated edge descriptor
static std::map<connection, edge_descriptor> connection_to_descriptor;

std::map<signal_type*, int> min_signal_propagate_distance;

void remove_disconnected_connections()
{
  // remove disconnected connections
  std::map<connection, edge_descriptor>::iterator i =
    connection_to_descriptor.begin();
  while (i != connection_to_descriptor.end()) {
    if (!i->first.connected()) {
      connection_to_descriptor.erase(i++);
    }
    else {
      ++i;
    }
  }
}

void remove_signal(signal_type* sig)
{
  clear_vertex(signal_to_descriptor[sig], signal_graph);
  remove_vertex(signal_to_descriptor[sig], signal_graph);
  delete sig;
  signal_to_descriptor.erase(sig);
  remove_disconnected_connections();
}

void random_remove_signal(minstd_rand& rand_gen);

struct tracking_bridge {
  tracking_bridge(const tracking_bridge& other) 
    : sig(other.sig), rand_gen(other.rand_gen)
  { ++bridge_count; }

  tracking_bridge(signal_type* s, minstd_rand& rg) : sig(s), rand_gen(rg) 
  { ++bridge_count; }

  ~tracking_bridge()
  { --bridge_count; }

  void operator()(int cur_dist, int max_dist, double deletion_prob,
                  int& deletions_left) const
  {
    if (signal_to_descriptor.find(sig) == signal_to_descriptor.end())
      return;

    ++cur_dist;

    // Update the directed Bacon distance
    if (min_signal_propagate_distance.find(sig) ==
          min_signal_propagate_distance.end()) {
      min_signal_propagate_distance[sig] = cur_dist;
    }
    else if (cur_dist < min_signal_propagate_distance[sig]) {
      min_signal_propagate_distance[sig] = cur_dist;
    }
    else if (deletion_prob == 0.0) {
      // don't bother calling because we've already found a better route here
      return;
    }

    // Maybe delete the signal
    if (uniform_01<minstd_rand>(rand_gen)() < deletion_prob &&
        deletions_left-- && signal_to_descriptor.size() > 1) {
      random_remove_signal(rand_gen);
    }
    // propagate the signal
    else if (cur_dist < max_dist) {
      (*sig)(cur_dist, max_dist, deletion_prob, deletions_left);
    }
  }

  signal_type* sig;
  minstd_rand& rand_gen;
  static int bridge_count;
};

int tracking_bridge::bridge_count = 0;

namespace boost {
  template<typename V>
  void visit_each(V& v, const tracking_bridge& t, int)
  {
    v(t);
    v(t.sig);
  }
}

signal_type* add_signal()
{
  signal_type* sig = new signal_type();
  vertex_descriptor v = add_vertex(signal_graph);
  signal_to_descriptor[sig] = v;
  put(signal_tag(), signal_graph, v, sig);

  return sig;
}

connection add_connection(signal_type* sig1, signal_type* sig2,
                          minstd_rand& rand_gen)
{
  std::cout << "  Adding connection: " << sig1 << " -> " << sig2 << std::endl;

  connection c = sig1->connect(tracking_bridge(sig2, rand_gen));
  edge_descriptor e =
    add_edge(signal_to_descriptor[sig1], signal_to_descriptor[sig2],
             signal_graph).first;
  connection_to_descriptor[c] = e;
  put(connection_tag(), signal_graph, e, c);
  put(edge_weight, signal_graph, e, 1);
  return c;
}

void remove_connection(connection c)
{
  signal_type* sig1 = get(signal_tag(), signal_graph,
                          source(connection_to_descriptor[c], signal_graph));
  signal_type* sig2 = get(signal_tag(), signal_graph,
                          target(connection_to_descriptor[c], signal_graph));
  std::cout << "  Removing connection: " << sig1 << " -> " << sig2
            << std::endl;
  c.disconnect();
  remove_edge(connection_to_descriptor[c], signal_graph);
  connection_to_descriptor.erase(c);
}

bool signal_connection_exists(signal_type* sig1, signal_type* sig2,
                              edge_descriptor& edge_desc)
{
  vertex_descriptor source_sig = signal_to_descriptor[sig1];
  vertex_descriptor target_sig = signal_to_descriptor[sig2];
  signal_graph_type::out_edge_iterator e;
  for (e = out_edges(source_sig, signal_graph).first;
       e != out_edges(source_sig, signal_graph).second; ++e) {
    if (target(*e, signal_graph) == target_sig) {
      edge_desc = *e;
      return true;
    }
  }
  return false;
}

bool signal_connection_exists(signal_type* sig1, signal_type* sig2)
{
  edge_descriptor e;
  return signal_connection_exists(sig1, sig2, e);
}

std::map<signal_type*, vertex_descriptor>::iterator
choose_random_signal(minstd_rand& rand_gen)
{
  int signal_idx
    = uniform_int<>(0, signal_to_descriptor.size() - 1)(rand_gen);
  std::map<signal_type*, vertex_descriptor>::iterator result =
    signal_to_descriptor.begin();
  for(; signal_idx; --signal_idx)
    ++result;

  return result;
}

void random_remove_signal(minstd_rand& rand_gen)
{
  std::map<signal_type*, vertex_descriptor>::iterator victim =
    choose_random_signal(rand_gen);
  std::cout << "  Removing signal " << victim->first << std::endl;
  remove_signal(victim->first);
}

void random_add_connection(minstd_rand& rand_gen)
{
  std::map<signal_type*, vertex_descriptor>::iterator source;
  std::map<signal_type*, vertex_descriptor>::iterator target;
  do {
    source = choose_random_signal(rand_gen);
    target = choose_random_signal(rand_gen);
  } while (signal_connection_exists(source->first, target->first));

  add_connection(source->first, target->first, rand_gen);
}

void random_remove_connection(minstd_rand& rand_gen)
{
  int victim_idx =
    uniform_int<>(0, num_edges(signal_graph)-1)(rand_gen);
  signal_graph_type::edge_iterator e = edges(signal_graph).first;
  while (victim_idx--) {
    ++e;
  }

  remove_connection(get(connection_tag(), signal_graph, *e));
}

void random_bacon_test(minstd_rand& rand_gen)
{
  signal_type* kevin = choose_random_signal(rand_gen)->first;
  min_signal_propagate_distance.clear();
  min_signal_propagate_distance[kevin] = 0;

  const int horizon = 10; // only go to depth 10 at most

  std::cout << "  Bacon test: kevin is " << kevin
            << "\n    Propagating signal...";

  // Propagate the signal out to the horizon
  int deletions_left = 0;
  (*kevin)(0, horizon, 0.0, deletions_left);

  std::cout << "OK\n    Finding shortest paths...";

  // Initialize all colors to white
  {
    unsigned int num = 0;
    for (signal_graph_type::vertex_iterator v = vertices(signal_graph).first;
         v != vertices(signal_graph).second;
         ++v) {
      //      put(vertex_color, signal_graph, *v, white_color);
      put(vertex_index, signal_graph, *v, num++);
    }

    BOOST_CHECK(num == num_vertices(signal_graph));
  }

  // Perform a breadth-first search starting at kevin, and record the
  // distances from kevin to each reachable node.
  std::map<vertex_descriptor, int> bacon_distance_map;

#if 0
  bacon_distance_map[signal_to_descriptor[kevin]] = 0;
  breadth_first_visit(signal_graph, signal_to_descriptor[kevin],
                      visitor(
                        make_bfs_visitor(
                         record_distances(
                           make_assoc_property_map(bacon_distance_map),
                           on_examine_edge()))).
                      color_map(get(vertex_color, signal_graph)));
#endif

  dijkstra_shortest_paths(signal_graph, signal_to_descriptor[kevin],
                          distance_map(make_assoc_property_map(bacon_distance_map)));
  std::cout << "OK\n";
  // Make sure the bacon distances agree (prior to the horizon)
  {
    std::map<signal_type*, int>::iterator i;
    for (i = min_signal_propagate_distance.begin();
         i != min_signal_propagate_distance.end();
         ++i) {
      if (i->second != bacon_distance_map[signal_to_descriptor[i->first]]) {
        std::cout << "Signal distance to " << i->first << " was "
                  << i->second << std::endl;
        std::cout << "Graph distance was "
                  << bacon_distance_map[signal_to_descriptor[i->first]]
                  << std::endl;
      }
      BOOST_CHECK(i->second == bacon_distance_map[signal_to_descriptor[i->first]]);
    }
  }
}

void randomly_create_connections(minstd_rand& rand_gen, double edge_probability)
{
  // Randomly create connections
  uniform_01<minstd_rand> random(rand_gen);
  for (signal_graph_type::vertex_iterator v1 = vertices(signal_graph).first;
       v1 != vertices(signal_graph).second; ++v1) {
    for (signal_graph_type::vertex_iterator v2 = vertices(signal_graph).first;
         v2 != vertices(signal_graph).second; ++v2) {
      if (random() < edge_probability) {
        add_connection(get(signal_tag(), signal_graph, *v1),
                       get(signal_tag(), signal_graph, *v2),
                       rand_gen);
      }
    }
  }
}

void random_recursive_deletion(minstd_rand& rand_gen)
{
  signal_type* kevin = choose_random_signal(rand_gen)->first;
  min_signal_propagate_distance.clear();
  min_signal_propagate_distance[kevin] = 0;

  const int horizon = 4; // only go to depth "horizon" at most

  std::cout << "  Recursive deletion test: start is " << kevin << std::endl;

  // Propagate the signal out to the horizon
  int deletions_left = (int)(0.05*num_vertices(signal_graph));
  (*kevin)(0, horizon, 0.05, deletions_left);
}

int test_main(int argc, char* argv[])
{
  if (argc < 4) {
    std::cerr << "Usage: random_signal_system <# of initial signals> "
              << "<edge probability> <iterations>" << std::endl;
    return 1;
  }

  int number_of_initial_signals = atoi(argv[1]);
  double edge_probability = atof(argv[2]);
  int iterations = atoi(argv[3]);

  int seed;
  if (argc == 5)
    seed = atoi(argv[4]);
  else
    seed = time(0);

  std::cout << "Number of initial signals: " << number_of_initial_signals
            << std::endl;
  std::cout << "Edge probability: " << edge_probability << std::endl;
  std::cout << "Iterations: " << iterations << std::endl;
  std::cout << "Seed: " << seed << std::endl;

  // Initialize random number generator
  minstd_rand rand_gen;
  rand_gen.seed(seed);

  for (int iter = 0; iter < iterations; ++iter) {
    if (num_vertices(signal_graph) < 2) {
      for (int i = 0; i < number_of_initial_signals; ++i)
        add_signal();
    }

    while (num_edges(signal_graph) < 2) {
      randomly_create_connections(rand_gen, edge_probability);
    }

    std::cerr << "Iteration #" << (iter+1) << std::endl;

    uniform_int<> random_action(0, 7);
    switch (random_action(rand_gen)) {
    case 0:
      std::cout << "  Adding new signal: " << add_signal() << std::endl;
      break;

    case 1:
      random_remove_signal(rand_gen);
      break;

    case 2:
      if (num_edges(signal_graph) <
            num_vertices(signal_graph)*num_vertices(signal_graph)) {
        random_add_connection(rand_gen);
      }
      break;

    case 3:
      random_remove_connection(rand_gen);
      break;

    case 4:
    case 5:
    case 6:
      random_bacon_test(rand_gen);
      break;

    case 7:
      random_recursive_deletion(rand_gen);
      break;
    }
  }

  for (signal_graph_type::vertex_iterator v = vertices(signal_graph).first;
       v != vertices(signal_graph).second;
       ++v) {
    delete get(signal_tag(), signal_graph, *v);
  }

  BOOST_CHECK(tracking_bridge::bridge_count == 0);
  if (tracking_bridge::bridge_count != 0) {
    std::cerr << tracking_bridge::bridge_count << " connections remain.\n";
  }
  return 0;
}
