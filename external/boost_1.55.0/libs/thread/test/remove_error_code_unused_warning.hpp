// Copyright (C) 2008 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/thread/thread.hpp>

void remove_unused_warning()
{
  //../../../boost/system/error_code.hpp:214:36: warning: 'boost::system::posix_category' defined but not used [-Wunused-variable]
  //../../../boost/system/error_code.hpp:215:36: warning: 'boost::system::errno_ecat' defined but not used [-Wunused-variable]
  //../../../boost/system/error_code.hpp:216:36: warning: 'boost::system::native_ecat' defined but not used [-Wunused-variable]

  (void)boost::system::posix_category;
  (void)boost::system::errno_ecat;
  (void)boost::system::native_ecat;

}
