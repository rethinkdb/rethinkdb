// Copyright (C) 2006 Trustees of Indiana University
//
// Authors: Douglas Gregor
//          Andrew Lumsdaine

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Performance test of the reduce() collective
#include <boost/mpi.hpp>
#include <boost/lexical_cast.hpp>

namespace mpi = boost::mpi;

struct add_int {
  int operator()(int x, int y) const { return x + y; }
};

struct wrapped_int
{
  wrapped_int() : value(0) { }
  wrapped_int(int value) : value(value) { }

  template<typename Archiver>
  void serialize(Archiver& ar, const unsigned int /*version*/) {
    ar & value;
  }

  int value;
};

inline wrapped_int operator+(wrapped_int x, wrapped_int y)
{
  return wrapped_int(x.value + y.value);
}

namespace boost { namespace mpi {
  template<> struct is_mpi_datatype<wrapped_int> : mpl::true_ { };
} }

struct serialized_int
{
  serialized_int() : value(0) { }
  serialized_int(int value) : value(value) { }

  template<typename Archiver>
  void serialize(Archiver& ar, const unsigned int /*version*/) {
    ar & value;
  }

  int value;
};

inline serialized_int operator+(serialized_int x, serialized_int y)
{
  return serialized_int(x.value + y.value);
}

int main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);
  mpi::communicator world;

  int repeat_count = 100;
  int outer_repeat_count = 2;

  if (argc > 1) repeat_count = boost::lexical_cast<int>(argv[1]);
  if (argc > 2) outer_repeat_count = boost::lexical_cast<int>(argv[2]);

  if (world.rank() == 0)
    std::cout << "# of processors: " << world.size() << std::endl
              << "# of iterations: " << repeat_count << std::endl;

  int value = world.rank();
  int result;
  wrapped_int wi_value = world.rank();
  wrapped_int wi_result;
  serialized_int si_value = world.rank();
  serialized_int si_result;

  // Spin for a while...
  for (int i = 0; i < repeat_count/10; ++i) {
    reduce(world, value, result, std::plus<int>(), 0);
    reduce(world, value, result, add_int(), 0);
    reduce(world, wi_value, wi_result, std::plus<wrapped_int>(), 0);
    reduce(world, si_value, si_result, std::plus<serialized_int>(), 0);
  }

  for (int outer = 0; outer < outer_repeat_count; ++outer) {
    // Raw MPI
    mpi::timer time;
    for (int i = 0; i < repeat_count; ++i) {
      MPI_Reduce(&value, &result, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    double reduce_raw_mpi_total_time = time.elapsed();

    // MPI_INT/MPI_SUM case
    time.restart();
    for (int i = 0; i < repeat_count; ++i) {
      reduce(world, value, result, std::plus<int>(), 0);
    }
    double reduce_int_sum_total_time = time.elapsed();

    // MPI_INT/MPI_Op case
    time.restart();
    for (int i = 0; i < repeat_count; ++i) {
      reduce(world, value, result, add_int(), 0);
    }
    double reduce_int_op_total_time = time.elapsed();

    // MPI_Datatype/MPI_Op case
    time.restart();
    for (int i = 0; i < repeat_count; ++i) {
      reduce(world, wi_value, wi_result, std::plus<wrapped_int>(), 0);
    }
    double reduce_type_op_total_time = time.elapsed();
  
    // Serialized/MPI_Op case
    time.restart();
    for (int i = 0; i < repeat_count; ++i) {
      reduce(world, si_value, si_result, std::plus<serialized_int>(), 0);
    }
    double reduce_ser_op_total_time = time.elapsed();


    if (world.rank() == 0)
      std::cout << "\nInvocation\tElapsed Time (seconds)"
                << "\nRaw MPI\t\t\t" << reduce_raw_mpi_total_time
                << "\nMPI_INT/MPI_SUM\t\t" << reduce_int_sum_total_time
                << "\nMPI_INT/MPI_Op\t\t" << reduce_int_op_total_time
                << "\nMPI_Datatype/MPI_Op\t" << reduce_type_op_total_time
                << "\nSerialized/MPI_Op\t" << reduce_ser_op_total_time 
                << std::endl;
  }

  return 0;
}
