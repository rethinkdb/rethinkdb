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

void
test_threading_level_io(mpi::threading::level orig) {
  std::ostringstream out;
  namespace mt = boost::mpi::threading;
  mt::level printed = mt::level(-1);

  out << orig;
  BOOST_CHECK(out.good());
  std::string orig_str(out.str());
  std::cout << "orig string:" << orig_str << '\n';
  std::istringstream in(orig_str);
  in >> printed;
  BOOST_CHECK(!in.bad());
  std::cout << "orig: " << orig << ", printed: " << printed << std::endl;
  BOOST_CHECK(orig == printed);
}

void
test_threading_levels_io() {
  namespace mt = boost::mpi::threading;
  test_threading_level_io(mt::single);
  test_threading_level_io(mt::funneled);
  test_threading_level_io(mt::serialized);
  test_threading_level_io(mt::multiple);
}

void
test_threading_level_cmp() {
  namespace mt = boost::mpi::threading;
  BOOST_CHECK(mt::single == mt::single);
  BOOST_CHECK(mt::funneled == mt::funneled);
  BOOST_CHECK(mt::serialized == mt::serialized);
  BOOST_CHECK(mt::multiple == mt::multiple);
  
  BOOST_CHECK(mt::single != mt::funneled);
  BOOST_CHECK(mt::single != mt::serialized);
  BOOST_CHECK(mt::single != mt::multiple);

  BOOST_CHECK(mt::funneled != mt::single);
  BOOST_CHECK(mt::funneled != mt::serialized);
  BOOST_CHECK(mt::funneled != mt::multiple);

  BOOST_CHECK(mt::serialized != mt::single);
  BOOST_CHECK(mt::serialized != mt::funneled);
  BOOST_CHECK(mt::serialized != mt::multiple);

  BOOST_CHECK(mt::multiple != mt::single);
  BOOST_CHECK(mt::multiple != mt::funneled);
  BOOST_CHECK(mt::multiple != mt::serialized);

  BOOST_CHECK(mt::single < mt::funneled);
  BOOST_CHECK(mt::funneled > mt::single);
  BOOST_CHECK(mt::single < mt::serialized);
  BOOST_CHECK(mt::serialized > mt::single);
  BOOST_CHECK(mt::single < mt::multiple);
  BOOST_CHECK(mt::multiple > mt::single);

  BOOST_CHECK(mt::funneled < mt::serialized);
  BOOST_CHECK(mt::serialized > mt::funneled);
  BOOST_CHECK(mt::funneled < mt::multiple);
  BOOST_CHECK(mt::multiple > mt::funneled);

  BOOST_CHECK(mt::serialized < mt::multiple);
  BOOST_CHECK(mt::multiple > mt::serialized);

  BOOST_CHECK(mt::single <= mt::single);
  BOOST_CHECK(mt::single <= mt::funneled);
  BOOST_CHECK(mt::funneled >= mt::single);
  BOOST_CHECK(mt::single <= mt::serialized);
  BOOST_CHECK(mt::serialized >= mt::single);
  BOOST_CHECK(mt::single <= mt::multiple);
  BOOST_CHECK(mt::multiple >= mt::single);

  BOOST_CHECK(mt::funneled <= mt::funneled);
  BOOST_CHECK(mt::funneled <= mt::serialized);
  BOOST_CHECK(mt::serialized >= mt::funneled);
  BOOST_CHECK(mt::funneled <= mt::multiple);
  BOOST_CHECK(mt::multiple >= mt::funneled);

  BOOST_CHECK(mt::serialized <= mt::serialized);
  BOOST_CHECK(mt::serialized <= mt::multiple);
  BOOST_CHECK(mt::multiple >= mt::serialized);

  BOOST_CHECK(mt::multiple <= mt::multiple);
}
    
int
test_main(int argc, char* argv[]) {
  test_threading_levels_io();
  test_threading_level_cmp();
  return 0;
}
