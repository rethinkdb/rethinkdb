// Copyright (C) 2005-2006 Matthias Troyer

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// An example of a parallel Monte Carlo simulation using some nodes to produce
// data and others to aggregate the data
#include <iostream>

#include <boost/mpi.hpp>
#include <boost/random/parallel.hpp>
#include <boost/random.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <cstdlib>

namespace mpi = boost::mpi;

enum {sample_tag, sample_skeleton_tag, sample_broadcast_tag, quit_tag};


void calculate_samples(int sample_length)
{
  int num_samples = 100;
  std::vector<double> sample(sample_length);

  // setup communicator by splitting

  mpi::communicator world;
  mpi::communicator calculate_communicator = world.split(0);

  unsigned int num_calculate_ranks = calculate_communicator.size();
  
  // the master of the accumulaion ranks is the first of them, hence
  // with a rank just one after the last calculation rank
  int master_accumulate_rank = num_calculate_ranks;
  
  // the master of the calculation ranks sends the skeleton of the sample 
  // to the master of the accumulation ranks

  if (world.rank()==0)
    world.send(master_accumulate_rank,sample_skeleton_tag,mpi::skeleton(sample));
  
  // next we extract the content of the sample vector, to be used in sending
  // the content later on
  
  mpi::content sample_content = mpi::get_content(sample);
  
  // now intialize the parallel random number generator
  
  boost::lcg64 engine(
        boost::random::stream_number = calculate_communicator.rank(),
        boost::random::total_streams = calculate_communicator.size()
      );
      
  boost::variate_generator<boost::lcg64&,boost::uniform_real<> > 
    rng(engine,boost::uniform_real<>());
    
  for (unsigned int i=0; i<num_samples/num_calculate_ranks+1;++i) {
    
    // calculate sample by filling the vector with random numbers
    // note that std::generate will not work since it takes the generator
    // by value, and boost::ref cannot be used as a generator.
    // boost::ref should be fixed so that it can be used as generator
    
    BOOST_FOREACH(double& x, sample)
      x = rng();
    
    // send sample to accumulation ranks
    // Ideally we want to do this as a broadcast with an inter-communicator 
    // between the calculation and accumulation ranks. MPI2 should support 
    // this, but here we present an MPI1 compatible solution.
    
    // send content of sample to first (master) accumulation process
    
    world.send(master_accumulate_rank,sample_tag,sample_content);
    
    // gather some results from all calculation ranks
    
    double local_result = sample[0];
    std::vector<double> gathered_results(calculate_communicator.size());
    mpi::all_gather(calculate_communicator,local_result,gathered_results);
  }
  
  // we are done: the master tells the accumulation ranks to quit
  if (world.rank()==0)
    world.send(master_accumulate_rank,quit_tag);
}



void accumulate_samples()
{
  std::vector<double> sample;

  // setup the communicator for all accumulation ranks by splitting

  mpi::communicator world;
  mpi::communicator accumulate_communicator = world.split(1);

  bool is_master_accumulate_rank = accumulate_communicator.rank()==0;

  // the master receives the sample skeleton
  
  if (is_master_accumulate_rank) 
    world.recv(0,sample_skeleton_tag,mpi::skeleton(sample));
    
  // and broadcasts it to all accumulation ranks
  mpi::broadcast(accumulate_communicator,mpi::skeleton(sample),0);
  
  // next we extract the content of the sample vector, to be used in receiving
  // the content later on
  
  mpi::content sample_content = mpi::get_content(sample);
  
  // accumulate until quit is called
  double sum=0.;
  while (true) {
  
    
    // the accumulation master checks whether we should quit
    if (world.iprobe(0,quit_tag)) {
      world.recv(0,quit_tag);
      for (int i=1; i<accumulate_communicator.size();++i)
        accumulate_communicator.send(i,quit_tag);
      std::cout << sum << "\n";
      break; // We're done
    }

    // the otehr accumulation ranks check whether we should quit
    if (accumulate_communicator.iprobe(0,quit_tag)) {
      accumulate_communicator.recv(0,quit_tag);
      std::cout << sum << "\n";
      break; // We're done
    }
    
    // check whether the master accumulation rank has received a sample
    if (world.iprobe(mpi::any_source,sample_tag)) {
      BOOST_ASSERT(is_master_accumulate_rank);
      
      // receive the content
      world.recv(mpi::any_source,sample_tag,sample_content);
      
      // now we need to braodcast
      // the problam is we do not have a non-blocking broadcast that we could 
      // abort if we receive a quit message from the master. We thus need to
      // first tell all accumulation ranks to start a broadcast. If the sample
      // is small, we could just send the sample in this message, but here we
      // optimize the code for large samples, so that the overhead of these
      // sends can be ignored, and we count on an optimized broadcast
      // implementation with O(log N) complexity

      for (int i=1; i<accumulate_communicator.size();++i)
        accumulate_communicator.send(i,sample_broadcast_tag);
      
      // now broadcast the contents of the sample to all accumulate ranks
      mpi::broadcast(accumulate_communicator,sample_content,0);
      
      // and handle the sample by summing the appropriate value
      sum += sample[0];
    }
  
    // the other accumulation ranks wait for a mesage to start the broadcast
    if (accumulate_communicator.iprobe(0,sample_broadcast_tag)) {
      BOOST_ASSERT(!is_master_accumulate_rank);
      
      accumulate_communicator.recv(0,sample_broadcast_tag);
      
      // receive broadcast of the sample contents
      mpi::broadcast(accumulate_communicator,sample_content,0);
      
      // and handle the sample
      
      // and handle the sample by summing the appropriate value
      sum += sample[accumulate_communicator.rank()];
    }
  }
}

int main(int argc, char** argv)
{
  mpi::environment env(argc, argv);
  mpi::communicator world;

  // half of the processes generate, the others accumulate
  // the sample size is just the number of accumulation ranks
  if (world.rank() < world.size()/2)
    calculate_samples(world.size()-world.size()/2);
  else
    accumulate_samples();

  return 0;
}


