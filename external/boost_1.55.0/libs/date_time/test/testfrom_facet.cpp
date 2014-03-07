/* Copyright (c) 2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland
 * $Date: 2008-02-27 12:00:24 -0800 (Wed, 27 Feb 2008) $
 *
 * This file isn't part of the official regression test suite at
 * the moment, but it is a basic test of the strings_from_facet.hpp
 * infrastructure that can be compiled trivially.
 */


#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>

#include "boost/date_time/strings_from_facet.hpp"
#include "algorithm_ext/container_print.hpp"



int
main() 
{
  using boost::date_time::gather_month_strings;
  using boost::date_time::gather_weekday_strings;

  std::vector<std::string> data;
  std::vector<std::wstring> wdata;

  data = gather_month_strings<char>(std::locale::classic());
  print(data, std::cout);
  data = gather_month_strings<char>(std::locale::classic(), false);
  print(data, std::cout);
  data = gather_weekday_strings<char>(std::locale::classic());
  print(data, std::cout);
  data = gather_weekday_strings<char>(std::locale::classic(), false);
  print(data, std::cout);

  wdata = gather_month_strings<wchar_t>(std::locale::classic());
  std::wofstream wof("from_facet_test.out");
  int i=0;
  while (i < wdata.size()) {
    wof << wdata[i] << std::endl;
    i++;
  }
}
