// Copyright (C) 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

// FIXME: Including because of a missing header in the serialization library.
// Patch sent to list...
#include <cassert>

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/serialization/list.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/test/minimal.hpp>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif

using boost::graph::distributed::mpi_process_group;

int test_main(int argc, char** argv)
{
  boost::mpi::environment env(argc, argv);

  mpi_process_group pg;

  int seventeen = 17;
  std::list<int> seventeens(17, 17);

  if (process_id(pg) == 0) {
    send(pg, 1, 0, seventeen);
    send(pg, 1, 1, seventeens);
  }
  synchronize(pg);

  if (process_id(pg) == 1) {
    int value;
    receive(pg, 0, 0, value);
    BOOST_CHECK(seventeen == value);

    std::list<int> values;
    receive(pg, 0, 1, values);
    BOOST_CHECK(seventeens == values);
  }

  return 0;
}
