// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>

class counter
{
public:
    counter() : count(0) { }

    int add(int val) {
        boost::unique_lock<boost::recursive_mutex> scoped_lock(mutex);
        count += val;
        return count;
    }
    int increment() {
        boost::unique_lock<boost::recursive_mutex> scoped_lock(mutex);
        return add(1);
    }

private:
    boost::recursive_mutex mutex;
    int count;
};

counter c;

void change_count()
{
    std::cout << "count == " << c.increment() << std::endl;
}

int main(int, char*[])
{
    const int num_threads=4;

    boost::thread_group threads;
    for (int i=0; i < num_threads; ++i)
        threads.create_thread(&change_count);

    threads.join_all();

    return 0;
}
