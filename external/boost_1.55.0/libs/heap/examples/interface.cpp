/*=============================================================================
    Copyright (c) 2010 Tim Blechmann

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <iostream>
#include <iomanip>

#include "../../../boost/heap/fibonacci_heap.hpp"

using std::cout;
using std::endl;
using namespace boost::heap;

//[ basic_interface
// PriorityQueue is expected to be a max-heap of integer values
template <typename PriorityQueue>
void basic_interface(void)
{
    PriorityQueue pq;

    pq.push(2);
    pq.push(3);
    pq.push(1);

    cout << "Priority Queue: popped elements" << endl;
    cout << pq.top() << " "; // 3
    pq.pop();
    cout << pq.top() << " "; // 2
    pq.pop();
    cout << pq.top() << " "; // 1
    pq.pop();
    cout << endl;
}
//]

//[ iterator_interface
// PriorityQueue is expected to be a max-heap of integer values
template <typename PriorityQueue>
void iterator_interface(void)
{
    PriorityQueue pq;

    pq.push(2);
    pq.push(3);
    pq.push(1);

    typename PriorityQueue::iterator begin = pq.begin();
    typename PriorityQueue::iterator end = pq.end();

    cout << "Priority Queue: iteration" << endl;
    for (typename PriorityQueue::iterator it = begin; it != end; ++it)
        cout << *it << " "; // 1, 2, 3 in unspecified order
    cout << endl;
}
//]

//[ ordered_iterator_interface
// PriorityQueue is expected to be a max-heap of integer values
template <typename PriorityQueue>
void ordered_iterator_interface(void)
{
    PriorityQueue pq;

    pq.push(2);
    pq.push(3);
    pq.push(1);

    typename PriorityQueue::ordered_iterator begin = pq.ordered_begin();
    typename PriorityQueue::ordered_iterator end = pq.ordered_end();

    cout << "Priority Queue: ordered iteration" << endl;
    for (typename PriorityQueue::ordered_iterator it = begin; it != end; ++it)
        cout << *it << " "; // 3, 2, 1 (i.e. 1, 2, 3 in heap order)
    cout << endl;
}
//]


//[ merge_interface
// PriorityQueue is expected to be a max-heap of integer values
template <typename PriorityQueue>
void merge_interface(void)
{
    PriorityQueue pq;

    pq.push(3);
    pq.push(5);
    pq.push(1);

    PriorityQueue pq2;

    pq2.push(2);
    pq2.push(4);
    pq2.push(0);

    pq.merge(pq2);

    cout << "Priority Queue: merge" << endl;
    cout << "first queue: ";
    while (!pq.empty()) {
        cout << pq.top() << " "; // 5 4 3 2 1 0
        pq.pop();
    }
    cout << endl;

    cout << "second queue: ";
    while (!pq2.empty()) {
        cout << pq2.top() << " "; // 4 2 0
        pq2.pop();
    }
    cout << endl;
}
//]

//[ heap_merge_algorithm
// PriorityQueue is expected to be a max-heap of integer values
template <typename PriorityQueue>
void heap_merge_algorithm(void)
{
    PriorityQueue pq;

    pq.push(3);
    pq.push(5);
    pq.push(1);

    PriorityQueue pq2;

    pq2.push(2);
    pq2.push(4);
    pq2.push(0);

    boost::heap::heap_merge(pq, pq2);

    cout << "Priority Queue: merge" << endl;
    cout << "first queue: ";
    while (!pq.empty()) {
        cout << pq.top() << " "; // 5 4 3 2 1 0
        pq.pop();
    }
    cout << endl;

    cout << "second queue: ";
    while (!pq2.empty()) {
        cout << pq2.top() << " "; // 4 2 0
        pq2.pop();
    }
    cout << endl;
}
//]

//[ mutable_interface
// PriorityQueue is expected to be a max-heap of integer values
template <typename PriorityQueue>
void mutable_interface(void)
{
    PriorityQueue pq;
    typedef typename PriorityQueue::handle_type handle_t;

    handle_t t3 = pq.push(3);
    handle_t t5 = pq.push(5);
    handle_t t1 = pq.push(1);

    pq.update(t3, 4);
    pq.increase(t5, 7);
    pq.decrease(t1, 0);

    cout << "Priority Queue: update" << endl;
    while (!pq.empty()) {
        cout << pq.top() << " "; // 7, 4, 0
        pq.pop();
    }
    cout << endl;
}
//]

//[ mutable_fixup_interface
// PriorityQueue is expected to be a max-heap of integer values
template <typename PriorityQueue>
void mutable_fixup_interface(void)
{
    PriorityQueue pq;
    typedef typename PriorityQueue::handle_type handle_t;

    handle_t t3 = pq.push(3);
    handle_t t5 = pq.push(5);
    handle_t t1 = pq.push(1);

    *t3 = 4;
    pq.update(t3);

    *t5 = 7;
    pq.increase(t5);

    *t1 = 0;
    pq.decrease(t1);

    cout << "Priority Queue: update with fixup" << endl;
    while (!pq.empty()) {
        cout << pq.top() << " "; // 7, 4, 0
        pq.pop();
    }
    cout << endl;
}
//]

//[ mutable_interface_handle_in_value
struct heap_data
{
    fibonacci_heap<heap_data>::handle_type handle;
    int payload;

    heap_data(int i):
        payload(i)
    {}

    bool operator<(heap_data const & rhs) const
    {
        return payload < rhs.payload;
    }
};

void mutable_interface_handle_in_value(void)
{
    fibonacci_heap<heap_data> heap;
    heap_data f(2);

    fibonacci_heap<heap_data>::handle_type handle = heap.push(f);
    (*handle).handle = handle; // store handle in node
}
//]


int main(void)
{
    using boost::heap::fibonacci_heap;

    cout << std::setw(2);

    basic_interface<fibonacci_heap<int> >();
    cout << endl;

    iterator_interface<fibonacci_heap<int> >();
    cout << endl;

    ordered_iterator_interface<fibonacci_heap<int> >();
    cout << endl;

    merge_interface<fibonacci_heap<int> >();
    cout << endl;

    mutable_interface<fibonacci_heap<int> >();
    cout << endl;

    mutable_fixup_interface<fibonacci_heap<int> >();

    mutable_interface_handle_in_value();
}
