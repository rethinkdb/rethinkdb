//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include "TuTest.hpp"

#include <boost/test/test_tools.hpp>

#include <stdexcept>



int test_main( int, char* [] )
{
  TuTest machine;
  machine.initiate();
  // unconsumed_event sanity check
  BOOST_REQUIRE_THROW( machine.process_event( EvY() ), std::runtime_error );
  machine.process_event( EvX() );
  return 0;
}
