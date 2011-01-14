
#ifndef __CONCURRENCY_PMAP_HPP__
#define __CONCURRENCY_PMAP_HPP__

#include "concurrency/task.hpp"

template<typename callable_t>
void pmap(int count, callable_t c) {
    task_t<void> *returns[count];
    for (int i = 0; i < count; i++) {
        returns[i] = task<void>(c, i);
    }
    for (int i = 0; i < count; i++) {
        returns[i]->join();
    }
}

template<typename callable_t, typename arg1_t>
void pmap(int count, callable_t c, arg1_t a1) {
    pmap(count, boost::bind(c, a1, _1));
}

template<typename callable_t, typename arg1_t, typename arg2_t>
void pmap(int count, callable_t c, arg1_t a1, arg2_t a2) {
    pmap(count, boost::bind(c, a1, a2, _1));
}

template<typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t>
void pmap(int count, callable_t c, arg1_t a1, arg2_t a2, arg3_t a3) {
    pmap(count, boost::bind(c, a1, a2, a3, _1));
}

template<typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t, typename arg4_t>
void pmap(int count, callable_t c, arg1_t a1, arg2_t a2, arg3_t a3, arg4_t a4) {
    pmap(count, boost::bind(c, a1, a2, a3, a4, _1));
}

template<typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t, typename arg4_t, typename arg5_t>
void pmap(int count, callable_t c, arg1_t a1, arg2_t a2, arg3_t a3, arg4_t a4, arg5_t a5) {
    pmap(count, boost::bind(c, a1, a2, a3, a4, a5, _1));
}

#endif /* __CONCURRENCY_PMAP_HPP__ */
