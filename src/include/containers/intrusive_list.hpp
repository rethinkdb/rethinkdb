
#ifndef __INTRUSIVE_LIST_HPP__
#define __INTRUSIVE_LIST_HPP__

template <class derived_t>
class intrusive_list_node_t {
public:
    intrusive_list_node_t() : prev(NULL), next(NULL) {}
    derived_t *prev, *next;
};

template <class node_t>
class intrusive_list_t {
public:
    intrusive_list_t() : _head(NULL), _tail(NULL) {}

    bool empty() {
        return !head();
    }

    void clear() {
        _head = NULL;
        _tail = NULL;
    }
    
    node_t* head() {
        return _head;
    }
    node_t* tail() {
        return _tail;
    }
    
    void push_front(node_t *_value) {
        intrusive_list_node_t<node_t> *value = _value;
        value->next = _head;
        value->prev = NULL;
        if(_head) {
            ((intrusive_list_node_t<node_t>*)_head)->prev = _value;
        }
        _head = _value;
        if(_tail == NULL) {
            _tail = _value;
        }
    }
    
    void push_back(node_t *_value) {
        intrusive_list_node_t<node_t> *value = _value;
        value->prev = _tail;
        value->next = NULL;
        if(_tail) {
            ((intrusive_list_node_t<node_t>*)_tail)->next = _value;
        }
        _tail = _value;
        if(_head == NULL) {
            _head = _value;
        }
    }
    
    void remove(node_t *_value) {
        intrusive_list_node_t<node_t> *value = _value;
        if(value->next) {
            ((intrusive_list_node_t<node_t>*)(value->next))->prev = value->prev;
        } else {
            _tail = value->prev;
        }
        if(value->prev) {
            ((intrusive_list_node_t<node_t>*)(value->prev))->next = value->next;
        } else {
            _head = value->next;
        }
    }

    void append_and_clear(intrusive_list_t<node_t> *list) {
        if(list->empty())
            return;
        
        if(!_head) {
            // We're empty, just set head and tail to the new list
            _head = list->head();
            _tail = list->tail();
        } else {
            // Just continue to new list
            ((intrusive_list_node_t<node_t>*)_tail)->next = list->head();
            ((intrusive_list_node_t<node_t>*)list->head())->prev = _tail;
            _tail = list->tail();
        }
        // Note, we can't do appends without clear because we'd break
        // the previous pointer in the head of the appended list.
        list->clear();
    }

protected:
    node_t *_head, *_tail;
};

// TODO: It would be really nice to support iterators on this list
// some day.

#endif // __INTRUSIVE_LIST_HPP__

