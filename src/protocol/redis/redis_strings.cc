#include "protocol/redis/redis.hpp"
#include "protocol/redis/redis_util.hpp"

struct string_set_oper_t : set_oper_t {
    string_set_oper_t(std::string &key, btree_slice_t *btree, timestamp_t timestamp, order_token_t otok) :
        set_oper_t(key, btree, timestamp, otok)
    {
        if(location.value.get() == NULL) {
            scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
            location.value.swap(smrsv);
            location.value->set_redis_type(REDIS_STRING);
        } else if(location.value->get_redis_type() != REDIS_STRING) {
            throw "Operation against key holding wrong kind of value";
        }

        value = reinterpret_cast<redis_string_value_t *>(location.value.get());
    }

    void clear() {
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        blob.clear(location.txn.get());
    }

    int set_range(std::string &val, int offset) {
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        uint64_t old_size = blob.valuesize();

        // Append region we're writing (if necessary)
        uint64_t new_size = offset + val.size();
        if(new_size > old_size) {
            blob.append_region(location.txn.get(), new_size - old_size);
        }

        // Write string
        blob.write_from_string(val, location.txn.get(), offset);

        return blob.valuesize();
    }

    int append(std::string &val) {
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        return set_range(val, blob.valuesize());
    }

    int set(std::string &val) {
        clear();
        return set_range(val, 0);
    }

    int crement(int by) {
        return incr_loc<redis_value_t>(location, by);
    }

    int setbit(unsigned bit_index, unsigned bit_val) {
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        int64_t difference = (bit_index / 8 + 1) - blob.valuesize();
        if(difference > 0) {
            blob.append_region(location.txn.get(), difference);
        }

        buffer_group_t dest;
        boost::scoped_ptr<blob_acq_t> acq(new blob_acq_t);
        blob.expose_region(location.txn.get(), rwi_write, bit_index / 8, 1, &dest, acq.get());

        buffer_group_t::buffer_t buff =  dest.get_buffer(0);
        int old_val = !!(reinterpret_cast<uint8_t *>(buff.data)[0] & (1 << (bit_index % 8)));
        reinterpret_cast<uint8_t *>(buff.data)[0] =
           (reinterpret_cast<uint8_t *>(buff.data)[0] | (1 << (bit_index % 8))) ^ (!!!bit_val << (bit_index % 8));

        return old_val;
    }

    void get_string(std::string &out) {
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        blob.read_to_string(out, location.txn.get(), 0, blob.valuesize());
    }

    ~string_set_oper_t() {
        
    }

protected:
    redis_string_value_t *value;
};

struct string_read_oper_t : read_oper_t {
    string_read_oper_t(std::string &key, btree_slice_t *btree, order_token_t otok) :
        read_oper_t(key, btree, otok)
    {
        if(location.value.get() == NULL) {
            // Throw error 
        } else if(location.value->get_redis_type() != REDIS_STRING) {
            throw "Operation against key holding wrong kind of value";
        }

        value = reinterpret_cast<redis_string_value_t *>(location.value.get());
    }

    int get_length() {
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        return blob.valuesize();
    }

    void get_range(std::string &out, unsigned offset, unsigned len) {
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        blob.read_to_string(out, location.txn.get(), offset, len);
    }

    void get_string(std::string &out) {
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        blob.read_to_string(out, location.txn.get(), 0, blob.valuesize());
    }
    
protected:
    redis_string_value_t *value;
};

//Strings Operations

//WRITE(append)
KEYS(append)
SHARD_W(append)

EXECUTE_W(append) {
    string_set_oper_t oper(one, btree, timestamp, otok);
    int new_size = oper.append(two);
    
    return int_response(new_size);
}

//WRITE(decr)
KEYS(decr)
SHARD_W(decr)

EXECUTE_W(decr) {
    string_set_oper_t oper(one, btree, timestamp, otok);
    return int_response(oper.crement(-1));
}

//WRITE(decrby)
KEYS(decrby)
SHARD_W(decrby)

EXECUTE_W(decrby) {
    string_set_oper_t oper(one, btree, timestamp, otok);
    return int_response(oper.crement(-two));
}

//READ(get)
KEYS(get)
SHARD_R(get)
PARALLEL(get)

EXECUTE_R(get) {
    string_read_oper_t oper(one, btree, otok);
    std::string result;
    oper.get_string(result);

    return read_response_t(new bulk_result_t(result));
}

//READ(getbit)
KEYS(getbit)
SHARD_R(getbit)
PARALLEL(getbit)

EXECUTE_R(getbit) {
    string_read_oper_t oper(one, btree, otok);
    std::string str;
    oper.get_string(str);
    char byte = str.at(two / 8);
    // TODO check that this is the right bit ordering
    return int_response(!!(byte & (1 << (two % 8))));
}

//READ(getrange)
KEYS(getrange)
SHARD_R(getrange)
PARALLEL(getrange)

EXECUTE_R(getrange) {
    string_read_oper_t oper(one, btree, otok);
    std::string str;
    oper.get_range(str, two, three);
    return read_response_t(new bulk_result_t(str));
}

//WRITE(getset)
KEYS(getset)
SHARD_W(getset)

EXECUTE_W(getset) {
    string_set_oper_t oper(one, btree, timestamp, otok);

    std::string old_val;
    oper.get_string(old_val);
    oper.set(two);

    return write_response_t(new bulk_result_t(two));
}

//WRITE(incr)
KEYS(incr)
SHARD_W(incr)

EXECUTE_W(incr) {
    string_set_oper_t oper(one, btree, timestamp, otok);
    return int_response(oper.crement(1));
}

//WRITE(incrby)
KEYS(incrby)
SHARD_W(incrby)

EXECUTE_W(incrby) {
    string_set_oper_t oper(one, btree, timestamp, otok);
    return int_response(oper.crement(two));
}

READ(mget)
WRITE(mset)
WRITE(msetnx)

//WRITE(set)
KEYS(set)
SHARD_W(set)

EXECUTE_W(set) {
    string_set_oper_t oper(one, btree, timestamp, otok);
    oper.set(two);

    return write_response_t(new ok_result_t());
}

//WRITE(setbit)
KEYS(setbit)
SHARD_W(setbit)

EXECUTE_W(setbit) {
    string_set_oper_t oper(one, btree, timestamp, otok);
    return int_response(oper.setbit(two, three));
}

WRITE(setex)

//WRITE(setrange)
KEYS(setrange)
SHARD_W(setrange)

EXECUTE_W(setrange) {
    string_set_oper_t oper(one, btree, timestamp, otok);
    return int_response(oper.set_range(three, two));
}

//READ(Strlen)
KEYS(Strlen)
SHARD_R(Strlen)
PARALLEL(Strlen)

EXECUTE_R(Strlen) {
    string_read_oper_t oper(one, btree, otok);
    return int_response(oper.get_length());
}
