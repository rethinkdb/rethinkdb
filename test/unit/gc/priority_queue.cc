#include <retest.hpp>
#include <queue>
#include <stdlib.h>
#include <time.h>
#include "serializer/log/garbage_collector.hpp"
#include "utils.hpp"
#include <functional>
#include <vector>

#define NINTS 2000

void test_pq_basic() {
    /* test against std priority queue */
    priority_queue_t<int, std::less<int> > myPQ;
    std::priority_queue<int, std::vector<int, gnew_alloc<int> > > refPQ;
    srand(time(NULL));

    for (int i = 0; i < NINTS; i++) {
        int val = rand();
        myPQ.push(val);
        refPQ.push(val);
    }
    while (!refPQ.empty()) {
        assert_eq(myPQ.pop(), refPQ.top());
        refPQ.pop();
    }
    assert_eq(myPQ.empty(), true);
}

void test_pq_update() {
    priority_queue_t<int, std::less<int> > myPQ;
    std::priority_queue<int, std::vector<int, gnew_alloc<int> > > refPQ;
    std::vector<priority_queue_t<int, std::less<int> >::entry_t *, gnew_alloc<priority_queue_t<int, std::less<int> >::entry_t *> > entries;

    srand(time(NULL));

    for (int i = 0; i < NINTS; i++) {
        int val = rand();
        entries.push_back(myPQ.push(val));
    }
    for (int i = 0; i < NINTS; i++) {
        int val = rand();
        entries[i]->data = val;
        entries[i]->update();
        refPQ.push(val);
    }
    while (!refPQ.empty()) {
        assert_eq(myPQ.pop(), refPQ.top());
        refPQ.pop();
    }
    assert_eq(myPQ.empty(), true);
}
