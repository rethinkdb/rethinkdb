//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2010-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include "robust_mutex_test.hpp"
#include <boost/interprocess/detail/robust_emulation.hpp>
#include <boost/interprocess/sync/spin/recursive_mutex.hpp>

int main(int argc, char *argv[])
{
   using namespace boost::interprocess;

   return test::robust_mutex_test
      < ipcdetail::robust_spin_mutex<ipcdetail::spin_recursive_mutex> >(argc, argv);
}

#include <boost/interprocess/detail/config_end.hpp>
