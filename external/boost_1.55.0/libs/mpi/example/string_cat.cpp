// Copyright (C) 2006 Douglas Gregor <doug.gregor@gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// An example using Boost.MPI's reduce() to concatenate strings.
#include <boost/mpi.hpp>
#include <iostream>
#include <string>
#include <boost/serialization/string.hpp> // Important for sending strings!
namespace mpi = boost::mpi;

/* Defining STRING_CONCAT_COMMUTATIVE lies to Boost.MPI by forcing it
 * to assume that string concatenation is commutative, which it is
 * not. However, doing so illustrates how the results of a reduction
 * can change when a non-commutative operator is assumed to be
 * commutative.
 */
#ifdef STRING_CONCAT_COMMUTATIVE
namespace boost { namespace mpi {

template<>
struct is_commutative<std::plus<std::string>, std::string> : mpl::true_ { };

} } // end namespace boost::mpi
#endif

int main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);
  mpi::communicator world;

  std::string names[10] = { "zero ", "one ", "two ", "three ", "four ",
                            "five ", "six ", "seven ", "eight ", "nine " };

  std::string result;
  reduce(world,
         world.rank() < 10? names[world.rank()] : std::string("many "),
         result, std::plus<std::string>(), 0);

  if (world.rank() == 0)
    std::cout << "The result is " << result << std::endl;

  return 0;
}
