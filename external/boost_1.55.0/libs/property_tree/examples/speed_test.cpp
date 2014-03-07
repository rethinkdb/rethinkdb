// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#define _HAS_ITERATOR_DEBUGGING 0

#include <boost/property_tree/ptree.hpp>
#include <boost/format.hpp>
#include <boost/shared_array.hpp>
#include <iostream>
#include <ctime>
#include <algorithm>

using namespace std;
using namespace boost;
using namespace boost::property_tree;

string dummy;
vector<string> keys;
vector<string> shuffled_keys;

void prepare_keys(int size)
{
    // Prepare keys
    keys.clear();
    for (int i = 0; i < size; ++i)
        keys.push_back((format("%d") % i).str());
    shuffled_keys = keys;
    srand(0);
    random_shuffle(shuffled_keys.begin(), shuffled_keys.end());
}

void clock_push_back(int size)
{
    prepare_keys(size);
    int max_repeats = 1000000 / size;
    shared_array<ptree> pt_array(new ptree[max_repeats]);

    int n = 0;
    clock_t t1 = clock(), t2;
    do
    {
        if (n >= max_repeats)
            break;
        ptree &pt = pt_array[n];
        for (int i = 0; i < size; ++i)
            pt.push_back(ptree::value_type(shuffled_keys[i], ptree()));
        t2 = clock();
        ++n;
    } while (t2 - t1 < CLOCKS_PER_SEC);

    cout << "  push_back (" << size << "): " << double(t2 - t1) / CLOCKS_PER_SEC / n * 1000 << " ms\n";

}

void clock_find(int size)
{
    prepare_keys(size);

    ptree pt;
    for (int i = 0; i < size; ++i)
        pt.push_back(ptree::value_type(keys[i], ptree("data")));

    int n = 0;
    clock_t t1 = clock(), t2;
    do
    {
        for (int i = 0; i < size; ++i)
            pt.find(shuffled_keys[i]);
        t2 = clock();
        ++n;
    } while (t2 - t1 < CLOCKS_PER_SEC);

    cout << "  find (" << size << "): " << double(t2 - t1) / CLOCKS_PER_SEC / n * 1000 << " ms\n";

}

void clock_erase(int size)
{
    prepare_keys(size);

    int max_repeats = 100000 / size;
    shared_array<ptree> pt_array(new ptree[max_repeats]);

    ptree pt;
    for (int n = 0; n < max_repeats; ++n)
        for (int i = 0; i < size; ++i)
            pt_array[n].push_back(ptree::value_type(keys[i], ptree("data")));

    int n = 0;
    clock_t t1 = clock(), t2;
    do
    {
        if (n >= max_repeats)
            break;
        ptree &pt = pt_array[n];
        for (int i = 0; i < size; ++i)
            pt.erase(shuffled_keys[i]);
        t2 = clock();
        ++n;
    } while (t2 - t1 < CLOCKS_PER_SEC);

    cout << "  erase (" << size << "): " << double(t2 - t1) / CLOCKS_PER_SEC / n * 1000 << " ms\n";
}

int main()
{

    // push_back
    clock_push_back(10);
    clock_push_back(100);
    clock_push_back(1000);

    // erase
    clock_erase(10);
    clock_erase(100);
    clock_erase(1000);

    // find
    clock_find(10);
    clock_find(100);
    clock_find(1000);

}
