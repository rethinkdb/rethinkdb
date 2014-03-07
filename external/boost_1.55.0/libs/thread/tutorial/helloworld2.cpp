// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/thread.hpp>
#include <iostream>

struct helloworld
{
    helloworld(const char* who) : m_who(who) { }
    void operator()()
    {
        std::cout << m_who << "says, \"Hello World.\"" << std::endl;
    }
    const char* m_who;
};

int main()
{
    boost::thread thrd(helloworld("Bob"));
    thrd.join();
}
