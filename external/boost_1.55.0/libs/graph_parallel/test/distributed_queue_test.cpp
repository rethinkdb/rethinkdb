// Copyright (C) 2004-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/queue.hpp>
#include <boost/test/minimal.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/pending/queue.hpp>
#include <boost/property_map/property_map.hpp>
#include <utility>
#include <iostream>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif

using boost::graph::distributed::mpi_process_group;

struct global_value 
{
  global_value(int p = -1, std::size_t l = 0) : processor(p), value(l) {}

  int processor;
  std::size_t value;

  template<typename Archiver>
  void serialize(Archiver& ar, const unsigned int /*version*/)
  {
    ar & processor & value;
  }
};

namespace boost { namespace mpi {
    template<> struct is_mpi_datatype<global_value> : mpl::true_ { };
} } // end namespace boost::mpi

BOOST_IS_BITWISE_SERIALIZABLE(global_value)
BOOST_CLASS_IMPLEMENTATION(global_value,object_serializable)
BOOST_CLASS_TRACKING(global_value,track_never)

inline bool operator==(const global_value& x, const global_value& y)
{ return x.processor == y.processor && x.value == y.value; }

struct global_value_owner_map
{
  typedef int value_type;
  typedef value_type reference;
  typedef global_value key_type;
  typedef boost::readable_property_map_tag category;
};

int get(global_value_owner_map, global_value k)
{
  return k.processor;
}

void test_distributed_queue()
{
  mpi_process_group process_group;

  typedef boost::queue<global_value> local_queue_type;

  typedef boost::graph::distributed::distributed_queue<mpi_process_group, 
                                             global_value_owner_map,
                                             local_queue_type> dist_queue_type;

  dist_queue_type Q(process_group, global_value_owner_map());

  mpi_process_group::process_id_type id = process_id(process_group),
    n = num_processes(process_group);
  global_value v(0, 0);

  if (id == 0) {
    std::cerr << "Should print level of each processor in a binary tree:\n";
  }
  synchronize(process_group);

  if (id == n-1) Q.push(v);
  while (!Q.empty()) {
    v = Q.top(); Q.pop();
    
    std::cerr << "#" << id << ": level = " << v.value << std::endl;

    int level_begin = 1;
    for (std::size_t i = 0; i < v.value; ++i) level_begin *= 2;
    int level_end = level_begin * 2;
    BOOST_CHECK(level_begin <= (id + 1));
    BOOST_CHECK((id + 1) <= level_end);

    ++v.value;
    v.processor = v.processor * 2 + 1;
    if (v.processor < n) Q.push(v);
    ++v.processor;
    if (v.processor < n) Q.push(v);
  }
}

int test_main(int argc, char** argv)
{
  boost::mpi::environment env(argc, argv);
  test_distributed_queue();
  return 0;
}
