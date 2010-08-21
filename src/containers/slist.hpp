
#ifndef __SLIST_HPP__
#define __SLIST_HPP__

#include "alloc/memalign.hpp"

template<typename T>
class slist_node_t {
public:
    slist_node_t *next;
    T *ptr;
};

template<typename T>
slist_node_t<T>* cons(T *ptr, slist_node_t<T> *head) {
    memalign_alloc_t<> alloc;
    return cons(ptr, head, &alloc);
}

template<typename T, class alloc_t>
slist_node_t<T>* cons(T *ptr, slist_node_t<T> *head, alloc_t *alloc) {
    slist_node_t<T> *new_head = (slist_node_t<T>*)alloc->malloc(sizeof(slist_node_t<T>));
    new_head->next = head;
    new_head->ptr = ptr;
    return new_head;
}

template<typename T>
slist_node_t<T>* free_node(slist_node_t<T> *head) {
    memalign_alloc_t<> alloc;
    return free_node(head, &alloc);
}

template<typename T, class alloc_t>
slist_node_t<T>* free_node(slist_node_t<T> *head, alloc_t *alloc) {
    slist_node_t<T> *new_head = head->next;
    alloc->free(head);
    return new_head;
}

template<typename T>
T* head(slist_node_t<T> *head) {
    return head->ptr;
}

template<typename T>
slist_node_t<T>* tail(slist_node_t<T> *head) {
    return head->next;
}

#endif // __SLIST_HPP__

