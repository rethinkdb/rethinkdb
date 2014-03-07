// Copyright (C) 2013 Alain Miniussi <alain.miniussi@oca.eu>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// test threading::level operations

#include <boost/mpi/environment.hpp>
#include <boost/test/minimal.hpp>
#include <iostream>
#include <sstream>

namespace mpi = boost::mpi;

int
test_main(int argc, char* argv[]) {
  mpi::threading::level required = mpi::threading::level(-1);
  BOOST_CHECK(argc == 2);
  std::istringstream cmdline(argv[1]);
  cmdline >> required;
  BOOST_CHECK(!cmdline.bad());
  mpi::environment env(argc,argv,required);
  BOOST_CHECK(env.thread_level() >= mpi::threading::single);
  BOOST_CHECK(env.thread_level() <= mpi::threading::multiple);
  return 0;
}
