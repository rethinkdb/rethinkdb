
#ifndef __ARRAY_NODE_HPP__
#define __ARRAY_NODE_HPP__

#include <algorithm>
#include <stdio.h>

struct array_leaf_node_t;
struct array_internal_node_t;

// TODO: this shouldn't be a compile time option
#define NODE_ORDER    16

struct array_node_t {
    array_node_t(bool _leaf) : leaf(_leaf), nkeys(0) {}
    
    bool leaf;
    bool is_leaf() {
        return leaf;
    }
    bool is_internal() {
        return !leaf;
    }

    bool is_full() {
        return nkeys == NODE_ORDER;
    }

    typedef array_leaf_node_t leaf_node_t;
    typedef array_internal_node_t internal_node_t;

protected:
    int lookup(int key) {
        int *_value = std::lower_bound(keys, keys + nkeys, key);
        return _value - keys;
    }

protected:
    int nkeys;
    int keys[NODE_ORDER];
};

struct array_leaf_node_t : public array_node_t {
    array_leaf_node_t() : array_node_t(true) {}

#ifndef NDEBUG
    void print() {
        for(int i = 0; i < nkeys; i++) {
            printf("%d : %d, ", keys[i], values[i]);
        }
        printf("\n");
    }
#endif

    int insert(int key, int value) {
        if(nkeys == NODE_ORDER) {
            return 0;
        }
        int index = array_node_t::lookup(key);
        // TODO: handle duplicates
        // TODO: this is likely suboptimal, we might want two for
        // loops (one for keys, one for values)
        for(int i = nkeys - 1; i >= index; i--) {
            keys[i + 1] = keys[i];
            values[i + 1] = values[i];
        }
        keys[index] = key;
        values[index] = value;
        nkeys++;
        return 1;
    }

    int lookup(int key, int *value) {
        int index = array_node_t::lookup(key);
        if(index == nkeys || keys[index] != key) {
            return 0;
        } else {
            *value = values[index];
            return 1;
        }
    }

    void split(array_leaf_node_t **lnode, array_leaf_node_t **rnode, int *median) {
        *median = keys[nkeys / 2];
        
        // TODO: we need a good allocator here
        array_leaf_node_t *_rnode = new array_leaf_node_t();
        int rkeys = nkeys - nkeys / 2;
        memcpy(_rnode->keys, keys + nkeys / 2, rkeys * sizeof(*keys));
        memcpy(_rnode->values, values + nkeys / 2, rkeys * sizeof(*values));
        _rnode->nkeys = rkeys;
        
        nkeys /= 2;
        
        *lnode = this;
        *rnode = _rnode;
    }

private:
    int values[NODE_ORDER];
};

struct array_internal_node_t : public array_node_t  {
    array_internal_node_t() : array_node_t(false) {}

#ifndef NDEBUG
    void print() {
        printf("  ");
        for(int i = 0; i < nkeys; i++) {
            printf("%d, ", keys[i]);
        }
        printf("\n");
        for(int i = 0; i < nkeys + 1; i++) {
            printf("%d, ", (int)(long)values[i]);
        }
        printf("\n\n");
    }
#endif

    int insert(int key, array_node_t *lnode, array_node_t *rnode) {
        if(nkeys == NODE_ORDER) {
            return 0;
        }
        int index = array_node_t::lookup(key);
        // TODO: handle duplicates
        // TODO: this is likely suboptimal, we might want two for
        // loops (one for keys, one for values)
        values[nkeys + 1] = values[nkeys];
        for(int i = nkeys - 1; i >= index; i--) {
            keys[i + 1] = keys[i];
            values[i + 1] = values[i];
        }
        keys[index] = key;
        values[index] = lnode;
        values[index + 1] = rnode;
        nkeys++;
        return 1;
    }

    array_node_t* lookup(int key) {
        int index = array_node_t::lookup(key);
        if(index == nkeys) {
            return values[nkeys];
        } else {
            if(key < keys[index]) {
                return values[index];
            } else {
                return values[index + 1];
            }
        }
    }

    void split(array_internal_node_t **lnode, array_internal_node_t **rnode, int *median) {
        *median = keys[nkeys / 2];
        
        // TODO: we need a good allocator here
        array_internal_node_t *_rnode = new array_internal_node_t();
        int rkeys = nkeys - nkeys / 2 - 1;
        memcpy(_rnode->keys, keys + (nkeys / 2 + 1), rkeys * sizeof(*keys));
        memcpy(_rnode->values, values + (nkeys / 2 + 1), (rkeys + 1) * sizeof(*values));
        _rnode->nkeys = rkeys;

        nkeys /= 2;
        *lnode = this;
        *rnode = _rnode;
    }

private:
    array_node_t* values[NODE_ORDER + 1];
};

#endif // __ARRAY_NODE_HPP__

