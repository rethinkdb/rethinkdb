// Copyright (C) 2005-2006 Douglas Gregor <doug.gregor@gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// A test of the all_gather() collective.
#include <boost/mpi/collectives/all_gather.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/test/minimal.hpp>
#include <algorithm>
#include "gps_position.hpp"
#include <boost/serialization/string.hpp>
#include <boost/serialization/list.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/lexical_cast.hpp>

using boost::mpi::communicator;

using boost::mpi::packed_skeleton_iarchive;
using boost::mpi::packed_skeleton_oarchive;

template<typename Generator>
void
all_gather_test(const communicator& comm, Generator generator,
                const char* kind)
{
  typedef typename Generator::result_type value_type;
  value_type value = generator(comm.rank());

  using boost::mpi::all_gather;

  std::vector<value_type> values;
  if (comm.rank() == 0) {
    std::cout << "Gathering " << kind << "...";
    std::cout.flush();
  }

  all_gather(comm, value, values);

  std::vector<value_type> expected_values;
  for (int p = 0; p < comm.size(); ++p)
    expected_values.push_back(generator(p));
  BOOST_CHECK(values == expected_values);
  if (comm.rank() == 0 && values == expected_values)
    std::cout << "OK." << std::endl;

  (comm.barrier)();
}

// Generates integers to test with gather()
struct int_generator
{
  typedef int result_type;

  int operator()(int p) const { return 17 + p; }
};

// Generates GPS positions to test with gather()
struct gps_generator
{
  typedef gps_position result_type;

  gps_position operator()(int p) const
  {
    return gps_position(39 + p, 16, 20.2799);
  }
};

struct string_generator
{
  typedef std::string result_type;

  std::string operator()(int p) const
  {
    std::string result = boost::lexical_cast<std::string>(p);
    result += " rosebud";
    if (p != 1) result += 's';
    return result;
  }
};

struct string_list_generator
{
  typedef std::list<std::string> result_type;

  std::list<std::string> operator()(int p) const
  {
    std::list<std::string> result;
    for (int i = 0; i <= p; ++i) {
      std::string value = boost::lexical_cast<std::string>(i);
      result.push_back(value);
    }
    return result;
  }
};

int test_main(int argc, char* argv[])
{
  boost::mpi::environment env(argc, argv);
  communicator comm;
  all_gather_test(comm, int_generator(), "integers");
  all_gather_test(comm, gps_generator(), "GPS positions");
  all_gather_test(comm, string_generator(), "string");
  all_gather_test(comm, string_list_generator(), "list of strings");
  return 0;
}
