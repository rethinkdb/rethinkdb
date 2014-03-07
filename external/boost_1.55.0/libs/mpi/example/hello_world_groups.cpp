// Copyright (C) 2013 Andreas Hehn <hehn@phys.ethz.ch>, ETH Zurich
// based on
// hellp-world_broadcast.cpp (C) 2006 Douglas Gregor <doug.gregor@gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// A simple Hello world! example
// using boost::mpi::group and boost::mpi::broadcast()

#include <stdexcept>
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/group.hpp>
#include <boost/mpi/collectives.hpp>

#include <boost/serialization/string.hpp>

namespace mpi = boost::mpi;

int main(int argc, char* argv[])
{
  mpi::environment  env(argc, argv);
  mpi::communicator world;
  if(world.size() < 2)
    throw std::runtime_error("Please run with at least 2 MPI processes!");

  int group_a_ranks[2] = {0,1};

  mpi::group world_group = world.group();
  mpi::group group_a     = world_group.include(group_a_ranks,group_a_ranks+2);

  mpi::communicator comm_a(world,group_a);

  std::string value("Hello world!");
  if(comm_a)
  {
    if(comm_a.rank() == 0) {
         value = "Hello group a!";
    }
    mpi::broadcast(comm_a, value, 0);
  }
  std::cout << "Process #" << world.rank() << " says " << value << std::endl;
  return 0;
}
