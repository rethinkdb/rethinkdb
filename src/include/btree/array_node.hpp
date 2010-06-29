
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
    explicit array_node_t(bool _leaf) : leaf(_leaf), nkeys(0) {}
    
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

    bool is_empty() {
        return nkeys == 0;
    }

    bool is_underfull() {
        return nkeys < NODE_ORDER / 2;
    }

    int lookup_key(int key) {
        int index = lookup(key);
        return keys[index];
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

    int remove(int key) {
        int index = parent_t::lookup(key);
        if(index == this->nkeys || this->keys[index] != key) {
            return 0;
        } else {
            for(int i = index; i < this->nkeys; i++) {
                this->keys[i] = this->keys[i + 1];
                this->values[i] = this->values[i + 1];
            }

            this->nkeys--;
            return 1;
        }
    }

    void shift(int amount) {
        if (amount > 0) {
            for (int i = NODE_ORDER - 1; i >0; i--) {
                if (i - amount >= 0) {
                    this->keys[i] = this->keys[i - amount];
                    this->values[i] = this->values[i - amount];
                } else {
                    break;
                }
            }
        } else if (amount < 0) {
            amount = -amount;
            for (int i = 0; i < NODE_ORDER; i++) {
                if (i + amount < NODE_ORDER) {
                    this->keys[i] = this->keys[i + amount];
                    this->values[i] = this->values[i + amount];
                } else {
                    break;
                }
            }
        }
    }

    int merge(array_internal_node_t<block_id_t> *parent, array_leaf_node_t<block_id_t> *sibling) {
        if(!(this->is_underfull() && sibling->is_underfull()))
            return 0;

        if(this->keys[0] < sibling->keys[0]) {
            // [ this ] [ sibling ] -> [ this sibling ]
            memcpy(this->keys + this->nkeys, sibling->keys, sibling->nkeys * sizeof(*this->keys));
            memcpy(this->values + this->nkeys, sibling->values, sibling->nkeys * sizeof(*this->keys));
            if(!parent->remove(this->keys[0], sibling->keys[0]))
                return 0;
        } else {
            // [ sibling ] [ this ] -> [ sibling this ]
            //make some space
            this->shift(sibling->nkeys);

            //copy in the sibling
            memcpy(this->keys, sibling->keys, sibling->nkeys * sizeof(*this->keys));
            memcpy(this->values, sibling->values, sibling->nkeys * sizeof(*this->keys));
            if(!parent->remove(this->keys[sibling->nkeys], sibling->keys[0]))
                return 0;
        }

        this->nkeys += sibling->nkeys;

        //update the parent
        return 1;
    }

    //make us have the same number of nodes as the sibling
    void level(array_internal_node_t<block_id_t> *parent, array_leaf_node_t<block_id_t> *sibling) {
        int to_copy = (sibling->nkeys - this->nkeys) / 2;
        if (this->keys[0] < sibling->keys[0]) {
            //    [ this ] <-- [ sibling ]
            // copy from sibling to this
            memcpy(this->keys + this->nkeys, sibling->keys, to_copy * sizeof(*this->keys));
            memcpy(this->values + this->nkeys, sibling->values, to_copy * sizeof(*this->values));

            // shift the sibling
            sibling->shift(-to_copy);

            this->nkeys += to_copy;
            sibling->nkeys -= to_copy;

            parent->change_key(this->keys[0], sibling->keys[0]);
        } else {
            //    [ sibling ] --> [ this ]
            //shift to make room for new stuff
            this->shift(to_copy);

            //copy from sibling to this
            memcpy(this->keys, sibling->keys + (sibling->nkeys - to_copy), to_copy * sizeof(*this->keys));
            memcpy(this->values, sibling->values + (sibling->nkeys - to_copy), to_copy * sizeof(*this->values));

            this->nkeys += to_copy;
            sibling->nkeys -= to_copy;

            parent->change_key(sibling->keys[0], this->keys[0]);
        }
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

    int remove(int key, int sib_key) {
        int index = parent_t::lookup(key), sib_index = parent_t::lookup(sib_key);
        if (sib_key == this->keys[sib_index])
            sib_index++;
        if (key == this->keys[index])
            index++;

        if (index == sib_index)
            return 0;

        block_id_t value_to_keep = this->values[index];

        int index_to_remove = index < sib_index ? index : sib_index;

        //shift over to remove the index
        for (int i = index_to_remove; i < this->nkeys - 1; i++) {
            this->keys[i] = this->keys[i + 1];
            this->values[i] = this->values[i + 1];
        }

        //copy over the last element
        this->values[this->nkeys - 1] = this->values[this->nkeys];

        //make sure the remaining key points to the new node
        this->values[index_to_remove] = value_to_keep;

        this->nkeys--;
        
        return 1;
    }

    void shift(int amount) {
        if (amount > 0) {
            for (int i = NODE_ORDER; i >= 0; i--) {
                if (0 <= (i - amount) && i < NODE_ORDER)
                    this->keys[i] = this->keys[i - amount];
                if (0 <= (i - amount))
                    this->values[i] = this->values[i - amount];
                else
                    break;
            }
        } else {
            amount = -amount;
            for (int i = 0; i <= NODE_ORDER; i++) {
                if (i + amount < NODE_ORDER)
                    this->keys[i] = this->keys[i + amount];
                if (i + amount <= NODE_ORDER)
                    this->values[i] = this->values[i + amount];
                else
                    break;
            }
        }
    }

    int merge(array_internal_node_t<block_id_t> *parent, array_internal_node_t<block_id_t> *sibling) {
        if(!(this->is_underfull() && sibling->is_underfull())) {
            return 0;
        }

        if(this->keys[0] < sibling->keys[0]) {
            int parent_key = parent->lookup_key(this->keys[this->nkeys - 1]);

            // [ this ] [ sibling ] -> [ this sibling ]
            this->keys[this->nkeys] = parent_key;
            memcpy(this->keys + this->nkeys + 1, sibling->keys, sibling->nkeys * sizeof(*this->keys));
            memcpy(this->values + this->nkeys + 1, sibling->values, sibling->nkeys * sizeof(*this->values));

            //fix the parent
            parent->remove(this->keys[0], sibling->keys[0]);
        } else {
            int parent_key = parent->lookup_key(sibling->keys[sibling->nkeys - 1]);
            // [ sibling ] [ this ] -> [ sibling this ]
            //make some space
            shift(sibling->nkeys + 1);
            this->keys[sibling->nkeys] = parent_key;

            //copy in the sibling
            memcpy(this->keys, sibling->keys, sibling->nkeys * sizeof(*this->keys));
            memcpy(this->values, sibling->values, (sibling->nkeys + 1) * sizeof(*this->values));

            //fix the parent
            parent->remove(this->keys[sibling->nkeys + 1], sibling->keys[0]);
        }

        this->nkeys += sibling->nkeys;

        return 1;
    }

    // find a sibling of the key, returns -1 if the sibling is before (lower keys)
    // and 1 if it's after (higher keys)
    int sibling(int key, int *sib_key) {
        assert(this->nkeys > 1);
        int index = parent_t::lookup(key);

        if (index + 1 < this->nkeys) {
            *sib_key = this->keys[index + 1];
            return 1;
        } else {
            *sib_key = this->keys[index - 1];
            return -1;
        }
    }

    void change_key(int key, int new_key) {
        int index = parent_t::lookup(key);

        this->keys[index] = new_key;
    }
    //make us have the same number of nodes as the sibling
    void level(array_internal_node_t<block_id_t> *parent, array_internal_node_t<block_id_t> *sibling) {
        int to_copy = (sibling->nkeys - this->nkeys) / 2;
        if (this->keys[0] < sibling->keys[0]) {
            //first grab a key from the parent
            this->keys[this->nkeys] = parent->lookup_key(this->keys[0]);

            //    [ this ] <-- [ sibling ]
            // copy from sibling to this
            memcpy(this->keys + this->nkeys + 1, sibling->keys, to_copy * sizeof(*this->keys));
            memcpy(this->values + this->nkeys + 1, sibling->values, to_copy * sizeof(*this->values));

            // shift the sibling
            sibling->shift(-to_copy);

            this->nkeys += to_copy;
            sibling->nkeys -= to_copy;

            parent->change_key(this->keys[0], this->keys[this->nkeys]);
        } else {
            //    [ sibling ] --> [ this ]
            //shift to make room for new stuff
            this->shift(to_copy);

            //copy from sibling to this
            memcpy(this->keys, sibling->keys + (sibling->nkeys - to_copy) + 1, (to_copy - 1) * sizeof(*this->keys));
            memcpy(this->values, sibling->values + (sibling->nkeys - to_copy) + 1, to_copy * sizeof(*this->values));

            //fill in the missing key
            this->keys[to_copy - 1] = parent->lookup_key(sibling->keys[0]);

            //clean up the sibling
            memset(sibling->keys + (sibling->nkeys - to_copy), 0, to_copy * sizeof(*this->keys));
            memset(sibling->values + (sibling->nkeys + 1 - to_copy), 0, to_copy * sizeof(*this->keys));

            this->nkeys += to_copy;
            sibling->nkeys -= to_copy;

            parent->change_key(sibling->keys[0], this->keys[0]);
        }
    }

private:
    block_id_t values[NODE_ORDER + 1];
};

#endif // __ARRAY_NODE_HPP__

