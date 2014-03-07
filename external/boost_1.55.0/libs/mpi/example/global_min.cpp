// Copyright (C) 2013 Alain Miniussi <alain.miniussi@oca.eu>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// An example using Boost.MPI's all_reduce() that compute the minimum
// of each process's value and broadcast the result to all the processes.

#include <boost/mpi.hpp>
#include <iostream>
#include <cstdlib>
namespace mpi = boost::mpi;

int main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);
  mpi::communicator world;

  std::srand(world.rank());
  int my_number = std::rand();
  int minimum;

  all_reduce(world, my_number, minimum, mpi::minimum<int>());
  
  if (world.rank() == 0) {
      std::cout << "The minimum value is " << minimum << std::endl;
  }

  return 0;
}
