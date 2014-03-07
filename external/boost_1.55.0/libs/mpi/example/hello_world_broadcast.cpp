// Copyright (C) 2006 Douglas Gregor <doug.gregor@gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// A simple Hello, world! example using Boost.MPI broadcast()

#include <boost/mpi.hpp>
#include <iostream>
#include <boost/serialization/string.hpp> // Needed to send/receive strings!
namespace mpi = boost::mpi;

int main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);
  mpi::communicator world;

  std::string value;
  if (world.rank() == 0) {
    value = "Hello, World!";
  }

  broadcast(world, value, 0);

  std::cout << "Process #" << world.rank() << " says " << value << std::endl;
  return 0;
}
