#include "containers/priority_queue.hpp"

template<class T, class Less>
void priority_queue_t<T, Less>::entry_t::update() {
    if (pq)
        pq->update(index);
}

template<class T, class Less>
void priority_queue_t<T, Less>::swap_entries(unsigned int i, unsigned int j) {
    entry_t *tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
    heap[i]->index = i;
    heap[j]->index = j;
}

template<class T, class Less>
unsigned int priority_queue_t<T, Less>::parent(unsigned int i) {
    return ((i + 1) / 2) - 1;
}

template<class T, class Less>
unsigned int priority_queue_t<T, Less>::left(unsigned int i) {
    return (2 * (i + 1)) - 1;
}

template<class T, class Less>
unsigned int priority_queue_t<T, Less>::right(unsigned int i) {
    return 2 * (i + 1);
}

template<class T, class Less>
void priority_queue_t<T, Less>::bubble_up(int *i) {
    while (*i > 0 && Less()(heap[parent(*i)]->data, heap[*i]->data)) {
        swap_entries(*i, parent(*i));
        *i = parent(*i);
    }
}

template<class T, class Less>
void priority_queue_t<T, Less>::bubble_up(int j) {
    int i = j;
    bubble_up(&i);
}

template<class T, class Less>
void priority_queue_t<T, Less>::bubble_down(int *i) {
    while ((left(*i) < heap.size() && Less()(heap[*i]->data, heap[left(*i)]->data)) ||
           (right(*i) < heap.size() &&  Less()(heap[*i]->data, heap[right(*i)]->data))) {
        if ((right(*i) < heap.size()) && Less()(heap[left(*i)]->data, heap[right(*i)]->data)) {
            swap_entries(*i, right(*i));
            *i = right(*i);
        } else {
            swap_entries(*i, left(*i));
            *i = left(*i);
        }
    }
}

template<class T, class Less>
void priority_queue_t<T, Less>::bubble_down(int j) {
    int i = j;
    bubble_down(&i);
}

template<class T, class Less>
priority_queue_t<T, Less>::priority_queue_t() {}


template<class T, class Less>
priority_queue_t<T, Less>::~priority_queue_t() {
    for (unsigned int i = 0; i < heap.size(); i++)
        delete heap[i];
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
    return heap[0]->data;
}

template<class T, class Less>
typename priority_queue_t<T, Less>::entry_t *priority_queue_t<T, Less>::push(T data) {
    typename priority_queue_t<T, Less>::entry_t *result = new entry_t();
    result->data = data;
    result->pq = this;
    result->index = heap.size();

    heap.push_back(result);
    bubble_up(heap.size() - 1);
    return result;
}

template<class T, class Less>
void priority_queue_t<T, Less>::remove(entry_t *e) {
    unsigned index = e->index;

    if (index < heap.size() - 1) {
        heap[index] = heap.back();
        heap[index]->index = index;
        update(index);
    }

    heap.pop_back();
    delete e;
}


template<class T, class Less>
T priority_queue_t<T, Less>::pop() {
    T result = heap.front()->data;

    delete heap.front();
    heap.pop_front();

    if (!heap.empty()) {
        heap.push_front(heap.back());
        heap.front()->index = 0;
        heap.pop_back();
    }

    bubble_down(0);

    return result;
}

template<class T, class Less>
void priority_queue_t<T, Less>::update(int index) {
    rassert(index >= 0);
    rassert((unsigned) index < size());
    int i = index;
    bubble_up(&i);
    bubble_down(&i);
}

template<class T, class Less>
void priority_queue_t<T, Less>::validate() {
    for (unsigned int i = 0; i < heap.size(); i++) {
        if(i != 0)
            rassert(!Less()(heap[parent(i)]->data, heap[i]->data));

        rassert(heap[i]->index == (int) i);
        rassert(heap[i]->pq == this);
    }
}
