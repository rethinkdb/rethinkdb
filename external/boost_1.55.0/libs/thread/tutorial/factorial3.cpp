// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/thread.hpp>
#include <iostream>

const int NUM_CALCS=5;

class factorial
{
public:
    factorial(int x, int& res) : x(x), res(res) { }
    void operator()() { res = calculate(x); }
    int result() const { return res; }

private:
    int calculate(int x) { return x <= 1 ? 1 : x * calculate(x-1); }

private:
    int x;
    int& res;
};

int main()
{
    int results[NUM_CALCS];
    boost::thread_group thrds;
    for (int i=0; i < NUM_CALCS; ++i)
        thrds.create_thread(factorial(i*10, results[i]));
    thrds.join_all();
    for (int j=0; j < NUM_CALCS; ++j)
        std::cout << j*10 << "! = " << results[j] << std::endl;
}
