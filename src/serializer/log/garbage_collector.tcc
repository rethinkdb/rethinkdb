#include "garbage_collector.hpp"

template<class T, class Less>
void priority_queue_t<T, Less>::entry_t::update() {
    if (pq)
        pq->update(index);
}

template<class T, class Less>
void priority_queue_t<T, Less>::swap(int i, int j) {
    entry_t tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
    heap[i].value.index = i;
    heap[j].value.index = j;
}

template<class T, class Less>
int priority_queue_t<T, Less>::parent(int i) {
    return ((i + 1) / 2) - 1;
}

template<class T, class Less>
int priority_queue_t<T, Less>::left(int i) {
    return (2 * (i + 1)) - 1;
}

template<class T, class Less>
int priority_queue_t<T, Less>::right(int i) {
    return 2 * (i + 1);
}

template<class T, class Less>
void priority_queue_t<T, Less>::bubble_up(int &i) {
    while (heap[parent(i)] < heap[i]) {
        swap(i, parent(i));
        i = parent(i);
    }
}

template<class T, class Less>
void priority_queue_t<T, Less>::bubble_down(int &i) {
    while (heap[i] < heap[left(i)] || heap[i] < heap[right(i)]) {
        if (Less(heap[left(i)], heap[right(i)])) {
            swap(i, right(i));
            i = right(i);
        } else {
            swap(i, left(i));
            i = left(i);
        }
    }
}

template<class T, class Less>
bool priority_queue_t<T, Less>::empty() {
    return heap.empty();
}


template<class T, class Less>
size_t priority_queue_t<T, Less>::size() {
    return heap.size();
}

template<class T, class Less>
T priority_queue_t<T, Less>::peak() {
    return heap[0].data;
}

template<class T, class Less>
typename priority_queue_t<T, Less>::entry_t *priority_queue_t<T, Less>::push(T data) {
    priority_queue_t<T, Less>::entry_t *result = new entry_t();
    result->data = data;
    result->pq = this;
    result->index = heap.size() + 1;

    heap.push_back(result);
    bubble_up(heap.size());

    return result;
}

template<class T, class Less>
T priority_queue_t<T, Less>::pop() {
    T t = heap.pop_front().data;
    heap.push_front(heap.pop_back());
    heap.front().index = 0;
    bubble_down(0);
    return t;
}

template<class T, class Less>
void priority_queue_t<T, Less>::update(int index) {
    bubble_up(index);
    bubble_down(index);
    heap[index].index = index;
}
