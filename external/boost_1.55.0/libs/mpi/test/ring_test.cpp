// Copyright (C) 2005, 2006 Douglas Gregor.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// A test of the communicator that passes data around a ring and
// verifies that the same data makes it all the way. Should test all
// of the various kinds of data that can be sent (primitive types, POD
// types, serializable objects, etc.)
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/test/minimal.hpp>
#include <algorithm>
#include "gps_position.hpp"
#include <boost/serialization/string.hpp>
#include <boost/serialization/list.hpp>

using boost::mpi::communicator;
using boost::mpi::status;

template<typename T>
void
ring_test(const communicator& comm, const T& pass_value, const char* kind,
          int root = 0)
{
  T transferred_value;

  int rank = comm.rank();
  int size = comm.size();

  if (rank == root) {
    std::cout << "Passing " << kind << " around a ring from root " << root
              << "...";
    comm.send((rank + 1) % size, 0, pass_value);
    comm.recv((rank + size - 1) % size, 0, transferred_value);
    BOOST_CHECK(transferred_value == pass_value);
    if (transferred_value == pass_value) std::cout << " OK." << std::endl;
  } else {
    comm.recv((rank + size - 1) % size, 0, transferred_value);
    BOOST_CHECK(transferred_value == pass_value);
    comm.send((rank + 1) % size, 0, transferred_value);
  }

  (comm.barrier)();
}


template<typename T>
void
ring_array_test(const communicator& comm, const T* pass_values,
                int n, const char* kind, int root = 0)
{
  T* transferred_values = new T[n];
  int rank = comm.rank();
  int size = comm.size();

  if (rank == root) {

    std::cout << "Passing " << kind << " array around a ring from root "
              << root  << "...";
    comm.send((rank + 1) % size, 0, pass_values, n);
    comm.recv((rank + size - 1) % size, 0, transferred_values, n);
    bool okay = std::equal(pass_values, pass_values + n,
                           transferred_values);
    BOOST_CHECK(okay);
    if (okay) std::cout << " OK." << std::endl;
  } else {
    status stat = comm.probe(boost::mpi::any_source, 0);
    boost::optional<int> num_values = stat.template count<T>();
    if (boost::mpi::is_mpi_datatype<T>())
      BOOST_CHECK(num_values && *num_values == n);
    else
      BOOST_CHECK(!num_values || *num_values == n);     
    comm.recv(stat.source(), 0, transferred_values, n);
    BOOST_CHECK(std::equal(pass_values, pass_values + n,
                           transferred_values));
    comm.send((rank + 1) % size, 0, transferred_values, n);
  }
  (comm.barrier)();
  delete [] transferred_values;
}

enum color_t {red, green, blue};
BOOST_IS_MPI_DATATYPE(color_t)

int test_main(int argc, char* argv[])
{
  boost::mpi::environment env(argc, argv);

  communicator comm;
  if (comm.size() == 1) {
    std::cerr << "ERROR: Must run the ring test with more than one process."
              << std::endl;
    MPI_Abort(comm, -1);
  }

  // Check transfer of individual objects
  ring_test(comm, 17, "integers", 0);
  ring_test(comm, 17, "integers", 1);
  ring_test(comm, red, "enums", 1);
  ring_test(comm, red, "enums", 1);
  ring_test(comm, gps_position(39,16,20.2799), "GPS positions", 0);
  ring_test(comm, gps_position(26,25,30.0), "GPS positions", 1);
  ring_test(comm, std::string("Rosie"), "string", 0);

  std::list<std::string> strings;
  strings.push_back("Hello");
  strings.push_back("MPI");
  strings.push_back("World");
  ring_test(comm, strings, "list of strings", 1);

  // Check transfer of arrays
  int int_array[2] = { 17, 42 };
  ring_array_test(comm, int_array, 2, "integer", 1);
  gps_position gps_position_array[2] = {
    gps_position(39,16,20.2799),
    gps_position(26,25,30.0)
  };
  ring_array_test(comm, gps_position_array, 2, "GPS position", 1);

  std::string string_array[3] = { "Hello", "MPI", "World" };
  ring_array_test(comm, string_array, 3, "string", 0);
  ring_array_test(comm, string_array, 3, "string", 1);

  return 0;
}
