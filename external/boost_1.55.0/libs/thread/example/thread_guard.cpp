// (C) Copyright 2009-2012 Anthony Williams
// (C) Copyright 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <iostream>
#include <string>
#include <boost/thread/thread_only.hpp>
#include <boost/thread/thread_guard.hpp>

void do_something(int& i)
{
    ++i;
}

struct func
{
    int& i;

    func(int& i_):i(i_){}

    void operator()()
    {
        for(unsigned j=0;j<1000000;++j)
        {
            do_something(i);
        }
    }

private:
    func& operator=(func const&);

};

void do_something_in_current_thread()
{}


void f()
{
    int some_local_state;
    func my_func(some_local_state);
    boost::thread t(my_func);
    boost::thread_guard<> g(t);

    do_something_in_current_thread();
}

int main()
{
    f();
    return 0;
}


