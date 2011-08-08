#include "protocol/redis/redis_util.hpp"
#include <iostream>

struct string_set_oper_t : set_oper_t {
    string_set_oper_t(std::string &key, btree_slice_t *btree, int64_t timestamp) :
        set_oper_t(key, btree, timestamp)
    {
        if(location.value.get() == NULL) {
            scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
            location.value.swap(smrsv);
            location.value->set_redis_type(REDIS_STRING);
        } else if(location.value->get_redis_type() != REDIS_STRING) {
            // Throw error
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
    string_read_oper_t(std::string &key, btree_slice_t *btree) :
        read_oper_t(key, btree)
    {
        if(location.value.get() == NULL) {
            // Throw error 
        } else if(location.value->get_redis_type() != REDIS_STRING) {
            // Throw error
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

//COMMAND_2(integer, append, string&, string&)
integer_result redis_actor_t::append(std::string &key, std::string &toappend) {
    string_set_oper_t oper(key, btree, 1234);
    int new_size = oper.append(toappend);
    
    return integer_result(new_size);
}

//COMMAND_1(integer, decr, string&)
integer_result redis_actor_t::decr(std::string &key) {
    string_set_oper_t oper(key, btree, 1234);
    return integer_result(oper.crement(-1));
}

//COMMAND_2(integer, decrby, string&, int)
integer_result redis_actor_t::decrby(std::string &key, int by) {
    string_set_oper_t oper(key, btree, 1234);
    return integer_result(oper.crement(-by));
}

//COMMAND_1(bulk, get, string&)
bulk_result redis_actor_t::get(string &key) {
    string_read_oper_t oper(key, btree);
    std::string *result = new std::string();
    oper.get_string(*result);
    boost::shared_ptr<std::string> res(result);
    return bulk_result(res);
}

//COMMAND_2(integer, getbit, string&, unsigned)
integer_result redis_actor_t::getbit(string &key, unsigned bit) {
    string_read_oper_t oper(key, btree);
    std::string str;
    oper.get_string(str);
    char byte = str.at(bit / 8);
    // TODO check that this is the right bit ordering
    return !!(byte & (1 << (bit % 8)));
}

//COMMAND_3(bulk, getrange, string&, int, int)
bulk_result redis_actor_t::getrange(string &key, int start, int end) {
    string_read_oper_t oper(key, btree);
    std::string *str = new std::string();
    oper.get_range(*str, start, end);
    boost::shared_ptr<std::string> res(str);
    return bulk_result(res);
}

//COMMAND_2(bulk, getset, string&, string&)
bulk_result redis_actor_t::getset(std::string &key, std::string &new_val) {
    string_set_oper_t oper(key, btree, 1234);

    std::string *old_val = new std::string();
    oper.get_string(*old_val);
    oper.set(new_val);

    return bulk_result(boost::shared_ptr<std::string>(old_val));
}

//COMMAND_1(integer, incr, string&)
integer_result redis_actor_t::incr(std::string &key) {
    string_set_oper_t oper(key, btree, 1234);
    return integer_result(oper.crement(1));
}

//COMMAND_2(integer, incrby, string&, int)
integer_result redis_actor_t::incrby(std::string &key, int by) {
    string_set_oper_t oper(key, btree, 1234);
    return integer_result(oper.crement(by));
}

COMMAND_N(multi_bulk, mget)
COMMAND_N(status, mset)
COMMAND_N(integer, msetnx)

//COMMAND_2(status, set, string&, string&)
status_result redis_actor_t::set(string &key, string &val) {
    string_set_oper_t oper(key, btree, 1234);
    oper.set(val);

    return redis_ok();
}

//COMMAND_3(integer, setbit, string&, unsigned, unsigned)
integer_result redis_actor_t::setbit(std::string &key, unsigned bit_index, unsigned bit_val) {
    string_set_oper_t oper(key, btree, 1234);
    return integer_result(oper.setbit(bit_index, bit_val));
}

COMMAND_3(status, setex, string&, unsigned, string&)

//COMMAND_3(integer, setrange, string&, unsigned, string&)
integer_result redis_actor_t::setrange(std::string &key, unsigned offset, std::string &val) {
    string_set_oper_t oper(key, btree, 1234);
    return integer_result(oper.set_range(val, offset));
}

//COMMAND_1(integer, Strlen, string&)
integer_result redis_actor_t::Strlen(std::string &key) {
    string_read_oper_t oper(key, btree);
    return integer_result(oper.get_length());
}
