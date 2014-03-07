// Copyright (C) 2006 Douglas Gregor <doug.gregor@gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// An example using Boost.MPI's skeletons and content to optimize
// communication.
#include <boost/mpi.hpp>
#include <boost/serialization/list.hpp>
#include <algorithm>
#include <functional>
#include <numeric>
#include <iostream>
#include <stdlib.h>
namespace mpi = boost::mpi;

int main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);
  mpi::communicator world;

  if (world.size() < 2 || world.size() > 4) {
    if (world.rank() == 0)
      std::cerr << "error: please execute this program with 2-4 processes.\n";
    world.abort(-1);
  }

  if (world.rank() == 0) {
    int list_len = 50;
    int iterations = 10;

    if (argc > 1) list_len = atoi(argv[1]);
    if (argc > 2) iterations = atoi(argv[2]);

    if (list_len <= 0) {
      std::cerr << "error: please specific a list length greater than zero.\n";
      world.abort(-1);
    }

    // Generate the list and broadcast its structure
    std::list<int> l(list_len);
    broadcast(world, mpi::skeleton(l), 0);

    // Generate content several times and broadcast out that content
    mpi::content c = mpi::get_content(l);
    for (int i = 0; i < iterations; ++i) {
      do {
        std::generate(l.begin(), l.end(), &random);
      } while (std::find_if(l.begin(), l.end(),
                            std::bind1st(std::not_equal_to<int>(), 0))
               == l.end());


      std::cout << "Iteration #" << i << ": sending content"
                << " (min = " << *std::min_element(l.begin(), l.end())
                << ", max = " << *std::max_element(l.begin(), l.end())
                << ", avg = "
                << std::accumulate(l.begin(), l.end(), 0)/l.size()
                << ").\n";

      broadcast(world, c, 0);
    }

    // Notify the slaves that we're done by sending all zeroes
    std::fill(l.begin(), l.end(), 0);
    broadcast(world, c, 0);

  } else {
    // Receive the content and build up our own list
    std::list<int> l;
    broadcast(world, mpi::skeleton(l), 0);

    mpi::content c = mpi::get_content(l);
    int i = 0;
    do {
      broadcast(world, c, 0);

      if (std::find_if(l.begin(), l.end(),
                       std::bind1st(std::not_equal_to<int>(), 0)) == l.end())
        break;

      if (world.rank() == 1)
        std::cout << "Iteration #" << i << ": max value = "
                  << *std::max_element(l.begin(), l.end()) << ".\n";
      else if (world.rank() == 2)
        std::cout << "Iteration #" << i << ": min value = "
                  << *std::min_element(l.begin(), l.end()) << ".\n";
      else if (world.rank() == 3)
        std::cout << "Iteration #" << i << ": avg value = "
                  << std::accumulate(l.begin(), l.end(), 0)/l.size()
                  << ".\n";
      ++i;
    } while (true);
  }

  return 0;
}
