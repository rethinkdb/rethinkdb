// Copyright (C) 2008 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/thread/thread.hpp>

void do_nothing()
{}

void test()
{
    boost::thread t1(do_nothing);
    boost::thread t2;
    t2=t1;
}

#include "./remove_error_code_unused_warning.hpp"
