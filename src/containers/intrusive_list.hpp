// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_INTRUSIVE_LIST_HPP_
#define CONTAINERS_INTRUSIVE_LIST_HPP_

#include "errors.hpp"

template <class node_t> class intrusive_list_t;

template <class derived_t>
class intrusive_list_node_t {
    friend class intrusive_list_t<derived_t>;

public:
    intrusive_list_node_t() :
#ifndef NDEBUG
        parent_list(NULL),
#endif
        prev(NULL), next(NULL)
        {}
    virtual ~intrusive_list_node_t() {
        rassert(prev == NULL);
        rassert(next == NULL);
        rassert(parent_list == NULL);
    }

#ifndef NDEBUG
    bool in_a_list() {
        return (parent_list != NULL);
    }
#endif

private:
#ifndef NDEBUG
    intrusive_list_t<derived_t> *parent_list;
#endif

    derived_t *prev, *next;

    DISABLE_COPYING(intrusive_list_node_t);
};

template <class node_t>
class intrusive_list_t {
public:
    intrusive_list_t() : _head(NULL), _tail(NULL), _size(0) {}
    ~intrusive_list_t() {
        //rassert(empty());
    }

    bool empty() {
        return !head();
    }

    node_t *head() const {
        return _head;
    }
    node_t *tail() const {
        return _tail;
    }

    node_t *next(node_t *elem) const {
        return static_cast<intrusive_list_node_t<node_t> *>(elem)->next;
    }
    node_t *prev(node_t *elem) const {
        return static_cast<intrusive_list_node_t<node_t> *>(elem)->prev;
    }

    void push_front(node_t *_value) {

        intrusive_list_node_t<node_t> *value = _value;
        rassert(value->next == NULL && value->prev == NULL && _head != _value && !value->parent_list); // Make sure that the object is not already in a list.
#ifndef NDEBUG
        value->parent_list = this;
#endif
        value->next = _head;
        value->prev = NULL;
        if(_head) {
            static_cast<intrusive_list_node_t<node_t> *>(_head)->prev = _value;
        }
        _head = _value;
        if(_tail == NULL) {
            _tail = _value;
        }
        _size++;
    }

    void push_back(node_t *_value) {

        intrusive_list_node_t<node_t> *value = _value;
        rassert(value->next == NULL && value->prev == NULL && _head != _value && !value->parent_list); // Make sure that the object is not already in a list.
#ifndef NDEBUG
        value->parent_list = this;
#endif
        value->prev = _tail;
        value->next = NULL;
        if(_tail) {
            static_cast<intrusive_list_node_t<node_t> *>(_tail)->next = _value;
        }
        _tail = _value;
        if(_head == NULL) {
            _head = _value;
        }
        _size++;
    }

    void remove(node_t *_value) {

        intrusive_list_node_t<node_t> *value = _value;

#ifndef NDEBUG
        rassert(value->parent_list == this);
        value->parent_list = NULL;
#endif

        if(value->next) {
            static_cast<intrusive_list_node_t<node_t> *>(value->next)->prev = value->prev;
        } else {
            _tail = value->prev;
        }
        if(value->prev) {
            static_cast<intrusive_list_node_t<node_t> *>(value->prev)->next = value->next;
        } else {
            _head = value->next;
        }
        value->next = value->prev = NULL;
        _size--;
    }

    void pop_front() {
        rassert(!empty());
        remove(head());
    }
    void pop_back() {
        rassert(!empty());
        remove(tail());
    }

    void append_and_clear(intrusive_list_t<node_t> *list) {

        if(list->empty())
            return;

#ifndef NDEBUG
        for (node_t *x = list->head(); x; x = list->next(x)) {
            rassert(static_cast<intrusive_list_node_t<node_t> *>(x)->parent_list == list);
            static_cast<intrusive_list_node_t<node_t> *>(x)->parent_list = this;
        }
#endif

        if(!_head) {
            // We're empty, just set head and tail to the new list
            _head = list->head();
            _tail = list->tail();
        } else {
            // Just continue to new list
            static_cast<intrusive_list_node_t<node_t> *>(_tail)->next = list->head();
            static_cast<intrusive_list_node_t<node_t> *>(list->head())->prev = _tail;
            _tail = list->tail();
        }
        // Note, we can't do appends without clear because we'd break
        // the previous pointer in the head of the appended list.
        _size += list->size();
        list->_head = NULL;
        list->_tail = NULL;
        list->_size = 0;
    }

    unsigned int size() const { return _size; }

#ifndef NDEBUG
    void validate() {
        node_t *last_node = NULL;
        unsigned int count = 0;
        for (node_t *node = _head; node; node = static_cast<intrusive_list_node_t<node_t> *>(node)->next) {
            count++;
            rassert(static_cast<intrusive_list_node_t<node_t> *>(node)->parent_list == this);
            rassert(static_cast<intrusive_list_node_t<node_t> *>(node)->prev == last_node);
            last_node = node;
        }
        rassert(_tail == last_node);
        rassert(_size == count);
    }
#endif


private:
    node_t *_head, *_tail;
    unsigned int _size;

    DISABLE_COPYING(intrusive_list_t);
};

#endif // CONTAINERS_INTRUSIVE_LIST_HPP_

