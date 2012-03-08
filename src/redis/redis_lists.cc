#include "redis/redis_util.hpp"
#include "redis/counted/counted.hpp"

// Lists utilities
struct list_set_oper_t : set_oper_t {
    list_set_oper_t(std::string &key, btree_slice_t *btree, timestamp_t timestamp,
                    order_token_t otok, bool alloc_non_existant = true) :
        set_oper_t(key, btree, timestamp, otok),
        tree(btree->cache()->get_block_size())
    {
        redis_list_value_t *value = reinterpret_cast<redis_list_value_t *>(location.value.get());
        
        if (value == NULL && alloc_non_existant) {
            scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
            location.value.swap(smrsv);
            location.value->set_redis_type(REDIS_LIST);
            value = reinterpret_cast<redis_list_value_t *>(location.value.get());
            value->get_ref()->count = 0;
            value->get_ref()->node_id = NULL_BLOCK_ID;
        } else if (value->get_redis_type() != REDIS_LIST) {
            throw "ERR Operation against key holding the wrong kind of value";
        }

        if (value) {
            ref = value->get_ref();
            tree = counted_btree_t(ref, btree->cache()->get_block_size(), txn.get());
        }
    }

    bool exists() {
        return (location.value.get() != NULL);
    }

    void insert(int index, std::string &value) {
        insert(convert_index(index), value);
    }

    void insert(unsigned index, std::string &value) {
        tree.insert_index(index, value);
    }

    void remove(int index) {
        remove(convert_index(index));
    }

    void remove(unsigned index) {
        tree.remove(index);
    }

    unsigned get_size() {
        return ref->count;
    }

    bool get_element(int index, std::string &str_out) {
        return get_element(convert_index(index), str_out);
    }

    bool get_element(unsigned index, std::string &str_out) {
        blob_t b(const_cast<char *>(tree.at(index)->blb), blob::btree_maxreflen);
        b.read_to_string(str_out, txn.get(), 0, b.valuesize());

        return true;
    }

protected:
    unsigned convert_index(int index) {
        int size = get_size();

        int u_index = index;
        if (index < 0) {
            u_index = size + index;
        }

        if (u_index < 0 || u_index > size) throw "ERR Index out of range";
        
        return u_index;
    }

    sub_ref_t *ref;
    counted_btree_t tree;
};

struct list_read_oper_t : read_oper_t {
    list_read_oper_t(std::string &key, btree_slice_t *btree, order_token_t otok) :
        read_oper_t(key, btree, otok),
        tree(btree->cache()->get_block_size())
    {
        redis_list_value_t *value = reinterpret_cast<redis_list_value_t *>(location.value.get());

        if (value == NULL) {
            throw "ERR Key doesn't exist";
        } else if (value->get_redis_type() != REDIS_LIST) {
            throw "ERR Operation against key holding the wrong kind of value";
        }

        ref = value->get_ref();
        tree = counted_btree_t(ref, btree->cache()->get_block_size(), txn.get());
    }

    unsigned get_size() {
        return ref->count;
    }

    bool get_element(int index, std::string &str_out) {
        int size = get_size();

        if (index < 0) {
            index = size + index;
        }

        if (index < 0 || index >= size) return false;

        blob_t b(const_cast<char *>(tree.at(index)->blb), blob::btree_maxreflen);
        b.read_to_string(str_out, txn.get(), 0, b.valuesize());

        return true;
    }

    unsigned convert_index(int index) {
        int size = get_size();

        int u_index = index;
        if (index < 0) {
            u_index = size + index;
        }

        if (u_index < 0 || u_index > size) throw "ERR Index out of range";

        return u_index;
    }

protected:
    sub_ref_t *ref;
    counted_btree_t tree;
};


// Lists commands

WRITE(blpop)
WRITE(brpop)
WRITE(brpoplpush)

//READ(lindex)
KEYS(lindex)
SHARD_R(lindex)

EXECUTE_R(lindex) {
    list_read_oper_t oper(one, btree, otok);
    std::string value;
    if (oper.get_element(two, value)) {
        return bulk_response(value);
    } else {
        return read_response_t();
    }
}

//WRITE(linsert)
KEYS(linsert)
SHARD_W(linsert)

EXECUTE_W(linsert) {
    list_set_oper_t oper(one, btree, timestamp, otok);

    // In future we might do this with an iterator so that we don't have do 
    // full log(n) lookups of each element. It doesn't make that bit a
    // difference for now though
    unsigned index = 0;
    while (index < oper.get_size()) {
        std::string element;
        oper.get_element(index, element);

        if (element == three) {
            unsigned insert_index;
            if (two == std::string("BEFORE")) {
                insert_index = index;
            } else if (three == std::string("AFTER")) {
                insert_index = index + 1;
            } else {
                throw "ERR Syntax error";
            }

            oper.insert(insert_index, four);
            return int_response(oper.get_size());
        }

        index++;
    }

    // Pivot not found
    return int_response(-1);
}

//READ(llen)
KEYS(llen)
SHARD_R(llen)

EXECUTE_R(llen) {
    list_read_oper_t oper(one, btree, otok);
    return int_response(oper.get_size());
}

//WRITE(lpop)
KEYS(lpop)
SHARD_W(lpop)

EXECUTE_W(lpop) {
    list_set_oper_t oper(one, btree, timestamp, otok);
    if (oper.get_size() <= 0) {
        // list is empty, nothing to pop
        return write_response_t();
    }

    std::string first_elem;
    oper.get_element(0, first_elem);
    oper.remove(0);

    return bulk_response(first_elem);
}

//WRITE(lpush)
redis_protocol_t::indicated_key_t redis_protocol_t::lpush::get_keys() {
    return one[0];
}

SHARD_W(lpush)
EXECUTE_W(lpush) {
    list_set_oper_t oper(one[0], btree, timestamp, otok);
    for (std::vector<std::string>::iterator iter = one.begin() + 1; iter != one.end(); ++iter) {
        // TODO insert in reverse order
        oper.insert(0, *iter);
    }

    return int_response(oper.get_size());
}

//WRITE(lpushx)
KEYS(lpushx)
SHARD_W(lpushx)

EXECUTE_W(lpushx) {
    list_set_oper_t oper(one, btree, timestamp, otok, false);
    if (!oper.exists()) {
        // List doesn't already exist abort
        return int_response(0);
    }
    
    oper.insert(0, two);
    return int_response(oper.get_size());
}

//READ(lrange)
KEYS(lrange)
SHARD_R(lrange)

EXECUTE_R(lrange) {
    list_read_oper_t oper(one, btree, otok);
    std::vector<std::string> result;

    unsigned start = oper.convert_index(two);
    unsigned stop = oper.convert_index(three);
    for (unsigned i = start; i <= stop; i++) {
        std::string str;
        if (oper.get_element(i, str)) {
            result.push_back(str);
        } else {
            // We've hit an out of range index
            break;
        }
    }

    return read_response_t(new multi_bulk_result_t(result));
}

//WRITE(lrem)
KEYS(lrem)
SHARD_W(lrem)

EXECUTE_W(lrem) {
    list_set_oper_t oper(one, btree, timestamp, otok);
    unsigned index = (two < 0) ? oper.get_size() - 1 : 0;
    int diff = (two < 0) ? -1 : 1;
    unsigned to_remove = (two == 0) ? (-1) : abs(two);
    unsigned removed = 0;

    // Since index is unsigned this condition also checks for index > 0
    while ((index < oper.get_size()) && (removed < to_remove)) {
        std::string element;
        oper.get_element(index, element);

        if (element == three) {
            oper.remove(index);
            removed++;
        } else {
            index += diff;
        }
    }

    return int_response(oper.get_size());
}

//WRITE(lset)
KEYS(lset)
SHARD_W(lset)

EXECUTE_W(lset) {
    list_set_oper_t oper(one, btree, timestamp, otok);
    oper.remove(two);
    oper.insert(two, three);

    return write_response_t(new ok_result_t());
}

//WRITE(ltrim)
KEYS(ltrim)
SHARD_W(ltrim)

EXECUTE_W(ltrim) {
    list_set_oper_t oper(one, btree, timestamp, otok);

    int size = oper.get_size();

    int start = two;
    if (start > size) {
        start = size;
    } else if (start < 0) {
        start += size;
        if (start < 0) {
            start = 0;
        }
    }

    int end = three;
    if (end > size) {
        end = size;
    } else if (end < 0) {
        end += size;
        if (end < 0) {
            end = 0;
        }
    }

    if (start > end) {
        start = end;
    }

    // Clear the end of the list first (so we don't have to rethink our bounds)
    for (int i = size - 1; i > end; i--) {
        oper.remove(i);
    }

    for (int i = start - 1; i >= 0; i--) {
        oper.remove((unsigned)i);
    }
    
    return write_response_t(new ok_result_t());
}

//WRITE(rpop)
KEYS(rpop)
SHARD_W(rpop)

EXECUTE_W(rpop) {
    list_set_oper_t oper(one, btree, timestamp, otok);
    int last_index = oper.get_size() - 1;
    std::string last_element;
    oper.get_element(last_index, last_element);
    oper.remove(last_index);

    return bulk_response(last_element);
}

//WRITE(rpush)
redis_protocol_t::indicated_key_t redis_protocol_t::rpush::get_keys() {
    return one[0];
}

SHARD_W(rpush)
EXECUTE_W(rpush) {
    list_set_oper_t oper(one[0], btree, timestamp, otok);
    for (std::vector<std::string>::iterator iter = one.begin() + 1; iter != one.end(); ++iter) {
        // TODO insert in reverse order
        oper.insert(oper.get_size(), *iter);
    }

    return int_response(oper.get_size());
}

//WRITE(rpushx)
KEYS(rpushx)
SHARD_W(rpushx)

EXECUTE_W(rpushx) {
    list_set_oper_t oper(one, btree, timestamp, otok, false);
    if (!oper.exists()) {
        // List doesn't already exist abort
        return int_response(0);
    }
    
    oper.insert(oper.get_size(), two);
    return int_response(oper.get_size());
}
