// Copyright (C) 2006 Douglas Gregor <doug.gregor@gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// An example using Boost.MPI's split() operation on communicators to
// create separate data-generating processes and data-collecting
// processes using boost::optional for broadcasting.
#include <boost/mpi.hpp>
#include <iostream>
#include <cstdlib>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/optional.hpp>
namespace mpi = boost::mpi;

enum message_tags { msg_data_packet, msg_finished };

void generate_data(mpi::communicator local, mpi::communicator world)
{
  using std::srand;
  using std::rand;

  // The rank of the collector within the world communicator
  int master_collector = local.size();

  srand(time(0) + world.rank());

  // Send out several blocks of random data to the collectors.
  int num_data_blocks = rand() % 3 + 1;
  for (int block = 0; block < num_data_blocks; ++block) {
    // Generate some random dataa
    int num_samples = rand() % 1000;
    std::vector<int> data;
    for (int i = 0; i < num_samples; ++i) {
      data.push_back(rand());
    }

    // Send our data to the master collector process.
    std::cout << "Generator #" << local.rank() << " sends some data..."
              << std::endl;
    world.send(master_collector, msg_data_packet, data);
  }

  // Wait for all of the generators to complete
  (local.barrier)();

  // The first generator will send the message to the master collector
  // indicating that we're done.
  if (local.rank() == 0)
    world.send(master_collector, msg_finished);
}

void collect_data(mpi::communicator local, mpi::communicator world)
{
  // The rank of the collector within the world communicator
  int master_collector = world.size() - local.size();

  if (world.rank() == master_collector) {
    while (true) {
      // Wait for a message
      mpi::status msg = world.probe();
      if (msg.tag() == msg_data_packet) {
        // Receive the packet of data into a boost::optional
        boost::optional<std::vector<int> > data;
        data = std::vector<int>();
        world.recv(msg.source(), msg.source(), *data);

        // Broadcast the actual data.
        broadcast(local, data, 0);
      } else if (msg.tag() == msg_finished) {
        // Receive the message
        world.recv(msg.source(), msg.tag());

        // Broadcast to each collector to tell them we've finished.
        boost::optional<std::vector<int> > data;
        broadcast(local, data, 0);
        break;
      }
    }
  } else {
    boost::optional<std::vector<int> > data;
    do {
      // Wait for a broadcast from the master collector
      broadcast(local, data, 0);
      if (data) {
        std::cout << "Collector #" << local.rank()
                  << " is processing data." << std::endl;
      }
    } while (data);
  }
}

int main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);
  mpi::communicator world;

  if (world.size() < 4) {
    if (world.rank() == 0) {
      std::cerr << "Error: this example requires at least 4 processes."
                << std::endl;
    }
    env.abort(-1);
  }

  bool is_generator = world.rank() < 2 * world.size() / 3;
  mpi::communicator local = world.split(is_generator? 0 : 1);
  if (is_generator) generate_data(local, world);
  else collect_data(local, world);

  return 0;
}
