
#ifndef __INTRUSIVE_LIST_HPP__
#define __INTRUSIVE_LIST_HPP__

template <class derived_t>
class intrusive_list_node_t {
public:
    derived_t *prev, *next;
};

template <class node_t>
class intrusive_list_t {
public:
    intrusive_list_t() : _head(NULL), _tail(NULL) {}
    
    node_t* head() {
        return _head;
    }
    node_t* tail() {
        return _tail;
    }
    
    void push_front(node_t *value) {
        value->next = _head;
        value->prev = NULL;
        if(_head) {
            _head->prev = value;
        }
        _head = value;
        if(_tail == NULL) {
            _tail = value;
        }
    }
    
    void push_back(node_t *value) {
        value->prev = _tail;
        value->next = NULL;
        if(_tail) {
            _tail->next = value;
        }
        _tail = value;
        if(_head == NULL) {
            _head = value;
        }
    }
    
    void remove(node_t *value) {
        if(value->next) {
            value->next->prev = value->prev;
        } else {
            _tail = value->prev;
        }
        if(value->prev) {
            value->prev->next = value->next;
        } else {
            _head = value->next;
        }
    }

protected:
    node_t *_head, *_tail;
};

// TODO: It would be really nice to support iterators on this list
// some day.

#endif // __INTRUSIVE_LIST_HPP__

