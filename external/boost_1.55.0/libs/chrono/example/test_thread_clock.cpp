//  test_thread_clock.cpp  ----------------------------------------------------------//

//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#include <boost/chrono/thread_clock.hpp>
#include <boost/type_traits.hpp>

#include <iostream>


void test_thread_clock()
{
#if defined(BOOST_CHRONO_HAS_THREAD_CLOCK)
  using namespace boost::chrono;

    std::cout << "thread_clock test" << std::endl;
    thread_clock::duration delay = milliseconds(5);
    thread_clock::time_point start = thread_clock::now();
    while (thread_clock::now() - start <= delay)
        ;
    thread_clock::time_point stop = thread_clock::now();
    thread_clock::duration elapsed = stop - start;
    std::cout << "paused " << nanoseconds(elapsed).count() << " nanoseconds\n";
    start = thread_clock::now();
    stop = thread_clock::now();
    std::cout << "thread_clock resolution estimate: " << nanoseconds(stop-start).count() << " nanoseconds\n";
#else
    std::cout << "thread_clock not available\n";
#endif
}


int main()
{
    test_thread_clock();
    return 0;
}

