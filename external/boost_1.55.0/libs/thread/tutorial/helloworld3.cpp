// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>

void helloworld(const char* who)
{
    std::cout << who << "says, \"Hello World.\"" << std::endl;
}

int main()
{
    boost::thread thrd(boost::bind(&helloworld, "Bob"));
    thrd.join();
}
