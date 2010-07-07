
#ifndef __INTRUSIVE_LIST_HPP__
#define __INTRUSIVE_LIST_HPP__

template <class derived_t>
class intrusive_list_node_t {
public:
    intrusive_list_node_t() : prev(NULL), next(NULL) {}
protected:
    friend class intrusive_list_t<derived_t>;
    derived_t *prev, *next;
};

template <class node_t>
class intrusive_list_t {
public:
    intrusive_list_t() : _head(NULL), _tail(NULL), _size(0) {}

    bool empty() {
        return !head();
    }

    void clear() {
        _head = NULL;
        _tail = NULL;
        _size = 0;
    }
    
    node_t* head() {
        return _head;
    }
    node_t* tail() {
        return _tail;
    }
    
    void push_front(node_t *_value) {
        validate();
    
        intrusive_list_node_t<node_t> *value = _value;
        assert(value->next == NULL && value->prev == NULL);
        value->next = _head;
        value->prev = NULL;
        if(_head) {
            ((intrusive_list_node_t<node_t>*)_head)->prev = _value;
        }
        _head = _value;
        if(_tail == NULL) {
            _tail = _value;
        }
        _size++;
        
        validate();
    }
    
    void push_back(node_t *_value) {
        validate();
    
        intrusive_list_node_t<node_t> *value = _value;
        assert(value->next == NULL && value->prev == NULL);
        value->prev = _tail;
        value->next = NULL;
        if(_tail) {
            ((intrusive_list_node_t<node_t>*)_tail)->next = _value;
        }
        _tail = _value;
        if(_head == NULL) {
            _head = _value;
        }
        _size++;
        
        validate();
    }
    
    void remove(node_t *_value) {
        validate();
    
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
        value->next = value->prev = NULL;
        _size--;
        
        validate();
    }

    void append_and_clear(intrusive_list_t<node_t> *list) {
        validate();
        list->validate();
    
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
        _size += list->size();
        list->clear();
        
        validate();
    }

    unsigned int size() { return _size; }
    
    void validate() {
        node_t *last_node = NULL;
        unsigned int count = 0;
        for (node_t *node = _head; node; node=((intrusive_list_node_t<node_t>*)node)->next) {
            count ++;
            assert(((intrusive_list_node_t<node_t>*)node)->prev == last_node);
            last_node = node;
        }
        assert(_tail == last_node);
        assert(_size == count);
    }
    
    class iterator {
    
    protected:
        iterator(node_t *node) : _node(node) { }
        node_t *_node;
        
    public:
        iterator() : _node(NULL) { }
        
        node_t &operator*() const {
            assert(_node);
            return *_node;
        }
        
        iterator operator++() {   // Prefix version
            assert(_node);
            _node = ((intrusive_list_node_t<node_t>*)_node)->next;
            return this;
        }
        iterator operator++(int) {   // Postfix version
            assert(_node);
            iterator last(_node);
            _node = ((intrusive_list_node_t<node_t>*)_node)->next;
            return last;
        }
        
        // Currently it's impossible to start at list.end() and then decrement to get the last
        // element in the list. Is it worth adding this?
        
        iterator operator--() {   // Prefix version
            assert(_node);
            _node = ((intrusive_list_node_t<node_t>*)_node)->prev;
            return this;
        }
        iterator operator--(int) {   // Postfix version
            assert(_node);
            iterator last(_node);
            _node = ((intrusive_list_node_t<node_t>*)_node)->prev;
            return last;
        }
        
        bool operator==(const iterator &other) const {
            return other._node == _node;
        }
        
        bool operator!=(const iterator &other) const {
            return other._node != _node;
        }
        
        friend class intrusive_list_t<node_t>;
    };
    
    iterator begin() {
        return iterator(_head);
    }
    
    iterator end() {
        return iterator(NULL);
    }
    
protected:
    node_t *_head, *_tail;
    unsigned int _size;
};

#endif // __INTRUSIVE_LIST_HPP__

