
#ifndef __ARRAY_NODE_HPP__
#define __ARRAY_NODE_HPP__

#include <algorithm>
#include <stdio.h>

template <typename block_id_t>
struct array_leaf_node_t;

template <typename block_id_t>
struct array_internal_node_t;

// TODO: this shouldn't be a compile time option (or perhaps it should
// in this case)
#define NODE_ORDER    16

// TODO: optimize for cache alignment/cache misses/etc.
template <typename block_id_t>
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

    typedef array_leaf_node_t<block_id_t> leaf_node_t;
    typedef array_internal_node_t<block_id_t> internal_node_t;

    void print() {
#ifndef NDEBUG
        if(is_leaf()) {
            ((leaf_node_t*)this)->print();
        } else {
            ((internal_node_t*)this)->print();
        }
#endif
    }
    
protected:
    int lookup(int key) {
        int *_value = std::lower_bound(keys, keys + nkeys, key);
        return _value - keys;
    }

protected:
    int nkeys;
    int keys[NODE_ORDER];
};

template <typename block_id_t>
struct array_leaf_node_t : public array_node_t<block_id_t> {
public:
    typedef array_node_t<block_id_t> parent_t;

public:
    array_leaf_node_t() : parent_t(true) {}

    void print() {
#ifndef NDEBUG
        for(int i = 0; i < this->nkeys; i++) {
            printf("%d : %d, ", this->keys[i], values[i]);
        }
        printf("\n");
#endif
    }

    int insert(int key, int value) {
        if(this->nkeys == NODE_ORDER) {
            return 0;
        }
        int index = parent_t::lookup(key);
        // TODO: handle duplicates
        // TODO: this is likely suboptimal, we might want two for
        // loops (one for keys, one for values)
        for(int i = this->nkeys - 1; i >= index; i--) {
            this->keys[i + 1] = this->keys[i];
            values[i + 1] = values[i];
        }
        this->keys[index] = key;
        values[index] = value;
        this->nkeys++;
        return 1;
    }

    int lookup(int key, int *value) {
        int index = parent_t::lookup(key);
        if(index == this->nkeys || this->keys[index] != key) {
            return 0;
        } else {
            *value = values[index];
            return 1;
        }
    }

    void split(array_leaf_node_t<block_id_t> *rnode, int *median) {
        *median = this->keys[this->nkeys / 2];
        
        int rkeys = this->nkeys - this->nkeys / 2;
        memcpy(rnode->keys, this->keys + this->nkeys / 2,
               rkeys * sizeof(*this->keys));
        memcpy(rnode->values, values + this->nkeys / 2, rkeys * sizeof(*values));
        rnode->nkeys = rkeys;
        
        this->nkeys /= 2;
    }

private:
    int values[NODE_ORDER];
};

template <typename block_id_t>
struct array_internal_node_t : public array_node_t<block_id_t>  {
public:
    typedef array_node_t<block_id_t> parent_t;

public:
    array_internal_node_t() : parent_t(false) {}

    void print() {
#ifndef NDEBUG
        printf("  ");
        for(int i = 0; i < this->nkeys; i++) {
            printf("%d, ", this->keys[i]);
        }
        printf("\n");
        for(int i = 0; i < this->nkeys + 1; i++) {
            printf("%d, ", (int)(long)values[i]);
        }
        printf("\n\n");
#endif
    }

    int insert(int key, block_id_t lnode, block_id_t rnode) {
        if(this->nkeys == NODE_ORDER) {
            return 0;
        }
        int index = parent_t::lookup(key);
        // TODO: handle duplicates
        // TODO: this is likely suboptimal, we might want two for
        // loops (one for keys, one for values)
        values[this->nkeys + 1] = values[this->nkeys];
        for(int i = this->nkeys - 1; i >= index; i--) {
            this->keys[i + 1] = this->keys[i];
            values[i + 1] = values[i];
        }
        this->keys[index] = key;
        values[index] = lnode;
        values[index + 1] = rnode;
        this->nkeys++;
        return 1;
    }

    block_id_t lookup(int key) {
        int index = parent_t::lookup(key);
        if(index == this->nkeys) {
            return values[this->nkeys];
        } else {
            if(key < this->keys[index]) {
                return values[index];
            } else {
                return values[index + 1];
            }
        }
    }

    void split(array_internal_node_t<block_id_t> *rnode, int *median) {
        *median = this->keys[this->nkeys / 2];
        
        int rkeys = this->nkeys - this->nkeys / 2 - 1;
        memcpy(rnode->keys, this->keys + (this->nkeys / 2 + 1),
               rkeys * sizeof(*this->keys));
        memcpy(rnode->values, values + (this->nkeys / 2 + 1),
               (rkeys + 1) * sizeof(*values));
        rnode->nkeys = rkeys;

        this->nkeys /= 2;
    }

private:
    block_id_t values[NODE_ORDER + 1];
};

#endif // __ARRAY_NODE_HPP__

