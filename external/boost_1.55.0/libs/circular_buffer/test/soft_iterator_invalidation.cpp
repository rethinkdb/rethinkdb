// Demonstration of rules when an iterator is considered to be valid if the soft
// iterator invalidation definition is applied.
// Note: The soft iterator invalidation definition CAN NOT be applied
//       to the space optimized circular buffer.

// Copyright (c) 2003-2008 Jan Gaspar

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_CB_DISABLE_DEBUG

#include "test.hpp"

// test of the example (introduced in the documentation)
void validity_example_test() {

    circular_buffer<int> cb(3);

    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    circular_buffer<int>::iterator it = cb.begin();

    BOOST_CHECK(*it == 1);

    cb.push_back(4);

    BOOST_CHECK(*it == 4);
}

void validity_insert_test() {

    int array[] = { 1, 2, 3 };

    // memory placement: { 1, 2, 3 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb(4, array, array + 3);

    // it1 -> 1, it2 -> 2, it3 -> 3
    circular_buffer<int>::iterator it1 = cb.begin();
    circular_buffer<int>::iterator it2 = cb.begin() + 1;
    circular_buffer<int>::iterator it3 = cb.begin() + 2;

    cb.insert(cb.begin() + 1, 4);

    // memory placement: { 1, 4, 2, 3 }
    // circular buffer:  { 1, 4, 2, 3 }
    BOOST_CHECK(*it1 == 1);
    BOOST_CHECK(*it2 == 4);
    BOOST_CHECK(*it3 == 2);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 4);
    BOOST_CHECK(cb[2] == 2);
    BOOST_CHECK(cb[3] == 3);

    // it4 -> 3
    circular_buffer<int>::iterator it4 = it1 + 3;

    cb.insert(cb.begin() + 1, 5);

    // memory placement: { 3, 5, 4, 2 }
    // circular buffer:  { 5, 4, 2, 3 }
    BOOST_CHECK(*it1 == 3);
    BOOST_CHECK(*it2 == 5);
    BOOST_CHECK(*it3 == 4);
    BOOST_CHECK(*it4 == 2);
    BOOST_CHECK(cb[0] == 5);
    BOOST_CHECK(cb[1] == 4);
    BOOST_CHECK(cb[2] == 2);
    BOOST_CHECK(cb[3] == 3);
}

void validity_insert_n_test() {

    // memory placement: { 1, 2, 3 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb(5);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    // it1 -> 1, it2 -> 2, it3 -> 3
    circular_buffer<int>::iterator it1 = cb.begin();
    circular_buffer<int>::iterator it2 = cb.begin() + 1;
    circular_buffer<int>::iterator it3 = cb.begin() + 2;

    cb.insert(cb.begin() + 1, 2, 4);

    // memory placement: { 1, 4, 4, 2, 3 }
    // circular buffer:  { 1, 4, 4, 2, 3 }
    BOOST_CHECK(*it1 == 1);
    BOOST_CHECK(*it2 == 4);
    BOOST_CHECK(*it3 == 4);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 4);
    BOOST_CHECK(cb[2] == 4);
    BOOST_CHECK(cb[3] == 2);
    BOOST_CHECK(cb[4] == 3);

    // it4 -> 2, it5 -> 3
    circular_buffer<int>::iterator it4 = it1 + 3;
    circular_buffer<int>::iterator it5 = it1 + 4;

    cb.insert(cb.begin() + 1, 2, 5);

    // memory placement: { 3, 5, 4, 4, 2 } - 5 inserted only once
    // circular buffer:  { 5, 4, 4, 2, 3 }
    BOOST_CHECK(*it1 == 3);
    BOOST_CHECK(*it2 == 5);
    BOOST_CHECK(*it3 == 4);
    BOOST_CHECK(*it4 == 4);
    BOOST_CHECK(*it5 == 2);
    BOOST_CHECK(cb[0] == 5);
    BOOST_CHECK(cb[1] == 4);
    BOOST_CHECK(cb[2] == 4);
    BOOST_CHECK(cb[3] == 2);
    BOOST_CHECK(cb[4] == 3);
}

void validity_insert_range_test() {

    vector<int> v1;
    v1.push_back(4);
    v1.push_back(5);

    vector<int> v2;
    v2.push_back(6);
    v2.push_back(7);


    // memory placement: { 1, 2, 3 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb1(5);
    cb1.push_back(1);
    cb1.push_back(2);
    cb1.push_back(3);

    // it11 -> 1, it12 -> 2, it13 -> 3
    circular_buffer<int>::iterator it11 = cb1.begin();
    circular_buffer<int>::iterator it12 = cb1.begin() + 1;
    circular_buffer<int>::iterator it13 = cb1.begin() + 2;

    cb1.insert(cb1.begin() + 1, v1.begin(), v1.end());

    // memory placement: { 1, 4, 5, 2, 3 }
    // circular buffer:  { 1, 4, 5, 2, 3 }
    BOOST_CHECK(*it11 == 1);
    BOOST_CHECK(*it12 == 4);
    BOOST_CHECK(*it13 == 5);
    BOOST_CHECK(cb1[0] == 1);
    BOOST_CHECK(cb1[1] == 4);
    BOOST_CHECK(cb1[2] == 5);
    BOOST_CHECK(cb1[3] == 2);
    BOOST_CHECK(cb1[4] == 3);

    // it14 -> 2, it15 -> 3
    circular_buffer<int>::iterator it14 = it11 + 3;
    circular_buffer<int>::iterator it15 = it11 + 4;

    cb1.insert(cb1.begin() + 1, v2.begin(), v2.end());

    // memory placement: { 3, 7, 4, 5, 2 } - 7 inserted only
    // circular buffer:  { 7, 4, 5, 2, 3 }
    BOOST_CHECK(*it11 == 3);
    BOOST_CHECK(*it12 == 7);
    BOOST_CHECK(*it13 == 4);
    BOOST_CHECK(*it14 == 5);
    BOOST_CHECK(*it15 == 2);
    BOOST_CHECK(cb1[0] == 7);
    BOOST_CHECK(cb1[1] == 4);
    BOOST_CHECK(cb1[2] == 5);
    BOOST_CHECK(cb1[3] == 2);
    BOOST_CHECK(cb1[4] == 3);

    // memory placement: { 1, 2, 3 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb2(5);
    cb2.push_back(1);
    cb2.push_back(2);
    cb2.push_back(3);

    // it21 -> 1, it22 -> 2, it23 -> 3
    circular_buffer<int>::iterator it21 = cb2.begin();
    circular_buffer<int>::iterator it22 = cb2.begin() + 1;
    circular_buffer<int>::iterator it23 = cb2.begin() + 2;

    cb2.insert(cb2.begin() + 1, MyInputIterator(v1.begin()), MyInputIterator(v1.end()));

    // memory placement: { 1, 4, 5, 2, 3 }
    // circular buffer:  { 1, 4, 5, 2, 3 }
    BOOST_CHECK(*it21 == 1);
    BOOST_CHECK(*it22 == 4);
    BOOST_CHECK(*it23 == 5);
    BOOST_CHECK(cb2[0] == 1);
    BOOST_CHECK(cb2[1] == 4);
    BOOST_CHECK(cb2[2] == 5);
    BOOST_CHECK(cb2[3] == 2);
    BOOST_CHECK(cb2[4] == 3);

    // it24 -> 2, it25 -> 3
    circular_buffer<int>::iterator it24 = it21 + 3;
    circular_buffer<int>::iterator it25 = it21 + 4;

    cb2.insert(cb2.begin() + 1, MyInputIterator(v2.begin()), MyInputIterator(v2.end()));

    // memory placement: { 2, 3, 7, 4, 5 } - using input iterator inserts all items even if they are later replaced
    // circular buffer:  { 7, 4, 5, 2, 3 }
    BOOST_CHECK(*it21 == 2);
    BOOST_CHECK(*it22 == 3);
    BOOST_CHECK(*it23 == 7);
    BOOST_CHECK(*it24 == 4);
    BOOST_CHECK(*it25 == 5);
    BOOST_CHECK(cb2[0] == 7);
    BOOST_CHECK(cb2[1] == 4);
    BOOST_CHECK(cb2[2] == 5);
    BOOST_CHECK(cb2[3] == 2);
    BOOST_CHECK(cb2[4] == 3);
}

void validity_rinsert_test() {

    int array[] = { 1, 2, 3 };

    // memory placement: { 1, 2, 3 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb(4, array, array + 3);

    // it1 -> 1, it2 -> 2, it3 -> 3
    circular_buffer<int>::iterator it1 = cb.begin();
    circular_buffer<int>::iterator it2 = cb.begin() + 1;
    circular_buffer<int>::iterator it3 = cb.begin() + 2;

    cb.rinsert(cb.begin() + 2, 4);

    // memory placement: { 2, 4, 3, 1 }
    // circular buffer:  { 1, 2, 4, 3 }
    BOOST_CHECK(*it1 == 2);
    BOOST_CHECK(*it2 == 4);
    BOOST_CHECK(*it3 == 3);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 2);
    BOOST_CHECK(cb[2] == 4);
    BOOST_CHECK(cb[3] == 3);

    // it4 -> 1
    circular_buffer<int>::iterator it4 = it1 - 1;

    cb.rinsert(cb.begin() + 2, 5);

    // memory placement: { 5, 4, 1, 2 }
    // circular buffer:  { 1, 2, 5, 4 }
    BOOST_CHECK(*it1 == 5);
    BOOST_CHECK(*it2 == 4);
    BOOST_CHECK(*it3 == 1);
    BOOST_CHECK(*it4 == 2);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 2);
    BOOST_CHECK(cb[2] == 5);
    BOOST_CHECK(cb[3] == 4);
}

void validity_rinsert_n_test() {

    // memory placement: { 1, 2, 3 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb(5);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    // it1 -> 1, it2 -> 2, it3 -> 3
    circular_buffer<int>::iterator it1 = cb.begin();
    circular_buffer<int>::iterator it2 = cb.begin() + 1;
    circular_buffer<int>::iterator it3 = cb.begin() + 2;

    cb.rinsert(cb.begin() + 2, 2, 4);

    // memory placement: { 4, 4, 3, 1, 2 }
    // circular buffer:  { 1, 2, 4, 4, 3 }
    BOOST_CHECK(*it1 == 4);
    BOOST_CHECK(*it2 == 4);
    BOOST_CHECK(*it3 == 3);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 2);
    BOOST_CHECK(cb[2] == 4);
    BOOST_CHECK(cb[3] == 4);
    BOOST_CHECK(cb[4] == 3);

    // it4 -> 1, it5 -> 2
    circular_buffer<int>::iterator it4 = it1 - 2;
    circular_buffer<int>::iterator it5 = it1 - 1;

    cb.rinsert(cb.begin() + 4, 2, 5);

    // memory placement: { 4, 5, 1, 2, 4 } - 5 inserted only once
    // circular buffer:  { 1, 2, 4, 4, 5 }
    BOOST_CHECK(*it1 == 4);
    BOOST_CHECK(*it2 == 5);
    BOOST_CHECK(*it3 == 1);
    BOOST_CHECK(*it4 == 2);
    BOOST_CHECK(*it5 == 4);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 2);
    BOOST_CHECK(cb[2] == 4);
    BOOST_CHECK(cb[3] == 4);
    BOOST_CHECK(cb[4] == 5);
}

void validity_rinsert_range_test() {

    vector<int> v1;
    v1.push_back(4);
    v1.push_back(5);

    vector<int> v2;
    v2.push_back(6);
    v2.push_back(7);


    // memory placement: { 1, 2, 3 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb1(5);
    cb1.push_back(1);
    cb1.push_back(2);
    cb1.push_back(3);

    // it1 -> 1, it2 -> 2, it3 -> 3
    circular_buffer<int>::iterator it11 = cb1.begin();
    circular_buffer<int>::iterator it12 = cb1.begin() + 1;
    circular_buffer<int>::iterator it13 = cb1.begin() + 2;

    cb1.rinsert(cb1.begin() + 2, v1.begin(), v1.end());

    // memory placement: { 4, 5, 3, 1, 2 }
    // circular buffer:  { 1, 2, 4, 5, 3 }
    BOOST_CHECK(*it11 == 4);
    BOOST_CHECK(*it12 == 5);
    BOOST_CHECK(*it13 == 3);
    BOOST_CHECK(cb1[0] == 1);
    BOOST_CHECK(cb1[1] == 2);
    BOOST_CHECK(cb1[2] == 4);
    BOOST_CHECK(cb1[3] == 5);
    BOOST_CHECK(cb1[4] == 3);

    // it14 -> 1, it15 -> 2
    circular_buffer<int>::iterator it14 = it11 - 2;
    circular_buffer<int>::iterator it15 = it11 - 1;

    cb1.rinsert(cb1.begin() + 4, v2.begin(), v2.end());

    // memory placement: { 5, 6, 1, 2, 4 } - 6 inserted only
    // circular buffer:  { 1, 2, 4, 5, 6 }
    BOOST_CHECK(*it11 == 5);
    BOOST_CHECK(*it12 == 6);
    BOOST_CHECK(*it13 == 1);
    BOOST_CHECK(*it14 == 2);
    BOOST_CHECK(*it15 == 4);
    BOOST_CHECK(cb1[0] == 1);
    BOOST_CHECK(cb1[1] == 2);
    BOOST_CHECK(cb1[2] == 4);
    BOOST_CHECK(cb1[3] == 5);
    BOOST_CHECK(cb1[4] == 6);

    // memory placement: { 1, 2, 3 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb2(5);
    cb2.push_back(1);
    cb2.push_back(2);
    cb2.push_back(3);

    // it1 -> 1, it2 -> 2, it3 -> 3
    circular_buffer<int>::iterator it21 = cb2.begin();
    circular_buffer<int>::iterator it22 = cb2.begin() + 1;
    circular_buffer<int>::iterator it23 = cb2.begin() + 2;

    cb2.rinsert(cb2.begin() + 2, MyInputIterator(v1.begin()), MyInputIterator(v1.end()));

    // memory placement: { 4, 5, 3, 1, 2 }
    // circular buffer:  { 1, 2, 4, 5, 3 }
    BOOST_CHECK(*it21 == 4);
    BOOST_CHECK(*it22 == 5);
    BOOST_CHECK(*it23 == 3);
    BOOST_CHECK(cb2[0] == 1);
    BOOST_CHECK(cb2[1] == 2);
    BOOST_CHECK(cb2[2] == 4);
    BOOST_CHECK(cb2[3] == 5);
    BOOST_CHECK(cb2[4] == 3);

    // it24 -> 1, it25 -> 2
    circular_buffer<int>::iterator it24 = it21 - 2;
    circular_buffer<int>::iterator it25 = it21 - 1;

    cb2.rinsert(cb2.begin() + 4, MyInputIterator(v2.begin()), MyInputIterator(v2.end()));

    // memory placement: { 5, 6, 1, 2, 4 }
    // circular buffer:  { 1, 2, 4, 5, 6 }
    BOOST_CHECK(*it21 == 5);
    BOOST_CHECK(*it22 == 6);
    BOOST_CHECK(*it23 == 1);
    BOOST_CHECK(*it24 == 2);
    BOOST_CHECK(*it25 == 4);
    BOOST_CHECK(cb2[0] == 1);
    BOOST_CHECK(cb2[1] == 2);
    BOOST_CHECK(cb2[2] == 4);
    BOOST_CHECK(cb2[3] == 5);
    BOOST_CHECK(cb2[4] == 6);
}

void validity_erase_test() {

    // memory placement: { 4, 5, 1, 2, 3 }
    // circular buffer:  { 1, 2, 3, 4, 5 }
    circular_buffer<int> cb(5);
    cb.push_back(-1);
    cb.push_back(0);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    cb.push_back(4);
    cb.push_back(5);

    // it1 -> 1, it2 -> 2, it3 -> 3, it4 -> 4
    circular_buffer<int>::iterator it1 = cb.begin();
    circular_buffer<int>::iterator it2 = cb.begin() + 1;
    circular_buffer<int>::iterator it3 = cb.begin() + 2;
    circular_buffer<int>::iterator it4 = cb.begin() + 3;

    cb.erase(cb.begin() + 1);

    // memory placement: { 5, X, 1, 3, 4 }
    // circular buffer:  { 1, 3, 4, 5 }
    BOOST_CHECK(*it1 == 1);
    BOOST_CHECK(*it2 == 3);
    BOOST_CHECK(*it3 == 4);
    BOOST_CHECK(*it4 == 5);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 3);
    BOOST_CHECK(cb[2] == 4);
    BOOST_CHECK(cb[3] == 5);
}

void validity_erase_range_test() {

    // memory placement: { 4, 5, 6, 1, 2, 3 }
    // circular buffer:  { 1, 2, 3, 4, 5, 6 }
    circular_buffer<int> cb(6);
    cb.push_back(-2);
    cb.push_back(-1);
    cb.push_back(0);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    cb.push_back(4);
    cb.push_back(5);
    cb.push_back(6);

    // it1 -> 1, it2 -> 2, it3 -> 3, it4 -> 4
    circular_buffer<int>::iterator it1 = cb.begin();
    circular_buffer<int>::iterator it2 = cb.begin() + 1;
    circular_buffer<int>::iterator it3 = cb.begin() + 2;
    circular_buffer<int>::iterator it4 = cb.begin() + 3;

    cb.erase(cb.begin() + 2, cb.begin() + 4);

    // memory placement: { 6, X, X, 1, 2, 5 }
    // circular buffer:  { 1, 2, 5, 6 }
    BOOST_CHECK(*it1 == 1);
    BOOST_CHECK(*it2 == 2);
    BOOST_CHECK(*it3 == 5);
    BOOST_CHECK(*it4 == 6);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 2);
    BOOST_CHECK(cb[2] == 5);
    BOOST_CHECK(cb[3] == 6);
}

void validity_rerase_test() {

    // memory placement: { 4, 5, 1, 2, 3 }
    // circular buffer:  { 1, 2, 3, 4, 5 }
    circular_buffer<int> cb(5);
    cb.push_back(-1);
    cb.push_back(0);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    cb.push_back(4);
    cb.push_back(5);

    // it1 -> 2, it2 -> 3, it3 -> 4, it4 -> 5
    circular_buffer<int>::iterator it1 = cb.begin() + 1;
    circular_buffer<int>::iterator it2 = cb.begin() + 2;
    circular_buffer<int>::iterator it3 = cb.begin() + 3;
    circular_buffer<int>::iterator it4 = cb.begin() + 4;

    cb.rerase(cb.begin() + 1);

    // memory placement: { 4, 5, X, 1, 3 }
    // circular buffer:  { 1, 3, 4, 5 }
    BOOST_CHECK(*it1 == 1);
    BOOST_CHECK(*it2 == 3);
    BOOST_CHECK(*it3 == 4);
    BOOST_CHECK(*it4 == 5);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 3);
    BOOST_CHECK(cb[2] == 4);
    BOOST_CHECK(cb[3] == 5);
}

void validity_rerase_range_test() {

    // memory placement: { 4, 5, 6, 1, 2, 3 }
    // circular buffer:  { 1, 2, 3, 4, 5, 6 }
    circular_buffer<int> cb(6);
    cb.push_back(-2);
    cb.push_back(-1);
    cb.push_back(0);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    cb.push_back(4);
    cb.push_back(5);
    cb.push_back(6);

    // it1 -> 3, it2 -> 4, it3 -> 5, it4 -> 6
    circular_buffer<int>::iterator it1 = cb.begin() + 2;
    circular_buffer<int>::iterator it2 = cb.begin() + 3;
    circular_buffer<int>::iterator it3 = cb.begin() + 4;
    circular_buffer<int>::iterator it4 = cb.begin() + 5;

    cb.rerase(cb.begin() + 2, cb.begin() + 4);

    // memory placement: { 2, 5, 6, X, X, 1 }
    // circular buffer:  { 1, 2, 5, 6 }
    BOOST_CHECK(*it1 == 1);
    BOOST_CHECK(*it2 == 2);
    BOOST_CHECK(*it3 == 5);
    BOOST_CHECK(*it4 == 6);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 2);
    BOOST_CHECK(cb[2] == 5);
    BOOST_CHECK(cb[3] == 6);
}

void validity_linearize_test() {

    // memory placement: { 3, 1, 2 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb(3);
    cb.push_back(0);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    // it1 -> 1, it2 -> 2, it3 -> 3
    circular_buffer<int>::iterator it1 = cb.begin();
    circular_buffer<int>::iterator it2 = cb.begin() + 1;
    circular_buffer<int>::iterator it3 = cb.begin() + 2;

    cb.linearize();

    // memory placement: { 1, 2, 3 }
    // circular buffer:  { 1, 2, 3 }
    BOOST_CHECK(*it1 == 2);
    BOOST_CHECK(*it2 == 3);
    BOOST_CHECK(*it3 == 1);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 2);
    BOOST_CHECK(cb[2] == 3);
}

void validity_swap_test() {

    // memory placement: { 3, 1, 2 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb1(3);
    cb1.push_back(0);
    cb1.push_back(1);
    cb1.push_back(2);
    cb1.push_back(3);

    // it11 -> 1, it12 -> 2, it13 -> 3
    circular_buffer<int>::iterator it11 = cb1.begin();
    circular_buffer<int>::iterator it12 = cb1.begin() + 1;
    circular_buffer<int>::iterator it13 = cb1.begin() + 2;

    // memory placement: { 4, 5, 6 }
    // circular buffer:  { 4, 5, 6 }
    circular_buffer<int> cb2(5);
    cb2.push_back(4);
    cb2.push_back(5);
    cb2.push_back(6);

    // it21 -> 4, it22 -> 5, it23 -> 6
    circular_buffer<int>::iterator it21 = cb2.begin();
    circular_buffer<int>::iterator it22 = cb2.begin() + 1;
    circular_buffer<int>::iterator it23 = cb2.begin() + 2;

    cb1.swap(cb2);

    // Although iterators refer to the original elements,
    // their interal state is inconsistent and no other operation
    // (except dereferencing) can be invoked on them any more.
    BOOST_CHECK(*it11 == 1);
    BOOST_CHECK(*it12 == 2);
    BOOST_CHECK(*it13 == 3);
    BOOST_CHECK(*it21 == 4);
    BOOST_CHECK(*it22 == 5);
    BOOST_CHECK(*it23 == 6);
    BOOST_CHECK(cb1[0] == 4);
    BOOST_CHECK(cb1[1] == 5);
    BOOST_CHECK(cb1[2] == 6);
    BOOST_CHECK(cb2[0] == 1);
    BOOST_CHECK(cb2[1] == 2);
    BOOST_CHECK(cb2[2] == 3);
}

void validity_push_back_test() {

    // memory placement: { 3, 1, 2 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb(3);
    cb.push_back(0);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    // it1 -> 1, it2 -> 2, it3 -> 3
    circular_buffer<int>::iterator it1 = cb.begin();
    circular_buffer<int>::iterator it2 = cb.begin() + 1;
    circular_buffer<int>::iterator it3 = cb.begin() + 2;

    cb.push_back(4);

    // memory placement: { 3, 4, 2 }
    // circular buffer:  { 2, 3, 4 }
    BOOST_CHECK(*it1 == 4);
    BOOST_CHECK(*it2 == 2);
    BOOST_CHECK(*it3 == 3);
    BOOST_CHECK(cb[0] == 2);
    BOOST_CHECK(cb[1] == 3);
    BOOST_CHECK(cb[2] == 4);
}

void validity_push_front_test() {

    // memory placement: { 3, 1, 2 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb(3);
    cb.push_back(0);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    // it1 -> 1, it2 -> 2, it3 -> 3
    circular_buffer<int>::iterator it1 = cb.begin();
    circular_buffer<int>::iterator it2 = cb.begin() + 1;
    circular_buffer<int>::iterator it3 = cb.begin() + 2;

    cb.push_front(4);

    // memory placement: { 4, 1, 2 }
    // circular buffer:  { 4, 1, 2 }
    BOOST_CHECK(*it1 == 1);
    BOOST_CHECK(*it2 == 2);
    BOOST_CHECK(*it3 == 4);
    BOOST_CHECK(cb[0] == 4);
    BOOST_CHECK(cb[1] == 1);
    BOOST_CHECK(cb[2] == 2);
}

void validity_pop_back_test() {

    // memory placement: { 3, 1, 2 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb(3);
    cb.push_back(0);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    // it1 -> 1, it2 -> 2
    circular_buffer<int>::iterator it1 = cb.begin();
    circular_buffer<int>::iterator it2 = cb.begin() + 1;

    cb.pop_back();

    // memory placement: { X, 1, 2 }
    // circular buffer:  { 1, 2 }
    BOOST_CHECK(*it1 == 1);
    BOOST_CHECK(*it2 == 2);
    BOOST_CHECK(cb[0] == 1);
    BOOST_CHECK(cb[1] == 2);
}

void validity_pop_front_test() {

    // memory placement: { 3, 1, 2 }
    // circular buffer:  { 1, 2, 3 }
    circular_buffer<int> cb(3);
    cb.push_back(0);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    // it1 -> 2, it2 -> 3
    circular_buffer<int>::iterator it1 = cb.begin() + 1;
    circular_buffer<int>::iterator it2 = cb.begin() + 2;

    cb.pop_front();

    // memory placement: { 3, X, 2 }
    // circular buffer:  { 2, 3 }
    BOOST_CHECK(*it1 == 2);
    BOOST_CHECK(*it2 == 3);
    BOOST_CHECK(cb[0] == 2);
    BOOST_CHECK(cb[1] == 3);
}

// test main
test_suite* init_unit_test_suite(int /*argc*/, char* /*argv*/[]) {

    test_suite* tests = BOOST_TEST_SUITE("Unit tests for the iterator of the circular_buffer.");

    tests->add(BOOST_TEST_CASE(&validity_example_test));
    tests->add(BOOST_TEST_CASE(&validity_insert_test));
    tests->add(BOOST_TEST_CASE(&validity_insert_n_test));
    tests->add(BOOST_TEST_CASE(&validity_insert_range_test));
    tests->add(BOOST_TEST_CASE(&validity_rinsert_test));
    tests->add(BOOST_TEST_CASE(&validity_rinsert_n_test));
    tests->add(BOOST_TEST_CASE(&validity_rinsert_range_test));
    tests->add(BOOST_TEST_CASE(&validity_erase_test));
    tests->add(BOOST_TEST_CASE(&validity_erase_range_test));
    tests->add(BOOST_TEST_CASE(&validity_rerase_test));
    tests->add(BOOST_TEST_CASE(&validity_rerase_range_test));
    tests->add(BOOST_TEST_CASE(&validity_linearize_test));
    tests->add(BOOST_TEST_CASE(&validity_swap_test));
    tests->add(BOOST_TEST_CASE(&validity_push_back_test));
    tests->add(BOOST_TEST_CASE(&validity_push_front_test));
    tests->add(BOOST_TEST_CASE(&validity_pop_back_test));
    tests->add(BOOST_TEST_CASE(&validity_pop_front_test));

    return tests;
}
