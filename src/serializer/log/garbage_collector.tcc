#include "garbage_collector.hpp"


template<class Priority_Queue>
base_value_t<Priority_Queue>::base_value_t() {
    priority_queue = NULL;
    index = -1;
}

template<class Priority_Queue, int N>
void gcarray_t<Priority_Queue, N>::set(size_t pos, bool value) {
#ifndef NDEBUG
    check("Index out of bounds", pos > N);
#endif
    std::bitset<N>::set(pos, value);
    if (base_value_t<Priority_Queue>::priority_queue)
        base_value_t<Priority_Queue>::priority_queue->update(index);
}

template<class Priority_Queue, int N>
bool gcarray_t<Priority_Queue, N>::get(size_t pos) {
#ifndef NDEBUG
    check("Index out of bounds", pos > N);
#endif
    return std::bitset<N>::get(pos);
}

template<class Priority_Queue, int N>
bool gcarray_t<Priority_Queue, N>::operator< (const gcarray_t &b) {
    return data.count() < b.data.count();
}


template<class Key, class Value, class Less>
void priority_queue_t<Key, Value, Less>::swap(int i, int j) {
    entry_t tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
    heap[i].value.index = i;
    heap[j].value.index = j;
}

template<class Key, class Value, class Less>
int priority_queue_t<Key, Value, Less>::parent(int i) {
    return ((i + 1) / 2) - 1;
}

template<class Key, class Value, class Less>
int priority_queue_t<Key, Value, Less>::left(int i) {
    return (2 * (i + 1)) - 1;
}

template<class Key, class Value, class Less>
int priority_queue_t<Key, Value, Less>::right(int i) {
    return 2 * (i + 1);
}

template<class Key, class Value, class Less>
void priority_queue_t<Key, Value, Less>::bubble_up(int &i) {
    while (Less(heap[parent(i)], heap[i])) {
        swap(i, parent(i));
        i = parent(i);
    }
}

template<class Key, class Value, class Less>
void priority_queue_t<Key, Value, Less>::bubble_down(int &i) {
    while (Less(heap[i], heap[left(i)]) || Less(heap[i], heap[right(i)])) {
        if (Less(heap[left(i)], heap[right(i)])) {
            swap(i, right(i));
            i = right(i);
        } else {
            swap(i, left(i));
            i = left(i);
        }
    }
}


template<class Key, class Value, class Less>
bool priority_queue_t<Key, Value, Less>::empty() {
    return heap.empty();
}


template<class Key, class Value, class Less>
size_t priority_queue_t<Key, Value, Less>::size() {
    return heap.size();
}

template<class Key, class Value, class Less>
Key priority_queue_t<Key, Value, Less>::peak() {
    return heap[0].key;
}

template<class Key, class Value, class Less>
void priority_queue_t<Key, Value, Less>::push(Key key, Value &value) {
    entry_t entry(key, value);
    heap.push_back(entry);
    value.priority_queue = this;
    value.index = heap.size();
    bubble_up(heap.size());
}

template<class Key, class Value, class Less>
Key priority_queue_t<Key, Value, Less>::pop() {
    Key key = heap.pop_front().value;
    heap.push_front(heap.pop_back());
    heap.front().index = 0;
    bubble_down(0);
}

template<class Key, class Value, class Less>
void priority_queue_t<Key, Value, Less>::update(int index) {
    bubble_up(index);
    bubble_down(index);
    heap[index].value.index = index;
}
