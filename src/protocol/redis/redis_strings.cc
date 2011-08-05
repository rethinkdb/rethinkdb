#include "protocol/redis/redis_util.hpp"
#include <iostream>

//Strings Operations

//COMMAND_2(integer, append, string&, string&)
integer_result redis_actor_t::append(std::string &key, std::string &toappend) {
    set_oper_t oper(key, btree, 1234);
    if(oper.location.value.get() == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        oper.location.value->set_redis_type(REDIS_STRING);
    } else if(oper.location.value->get_redis_type() != REDIS_STRING) {
        return integer_result(redis_error("Operation against key holding wrong kind of value"));
    }
    
    redis_string_value_t *value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
    blob_t blob(value->get_content(), blob::btree_maxreflen);
    
    uint64_t old_size = blob.valuesize();
    blob.append_region(oper.location.txn.get(), toappend.size());
    blob.write_from_string(toappend, oper.location.txn.get(), old_size);

    return integer_result(blob.valuesize());
}

//COMMAND_1(integer, decr, string&)
integer_result redis_actor_t::decr(std::string &key) {
    return crement(key, -1);
}

//COMMAND_2(integer, decrby, string&, int)
integer_result redis_actor_t::decrby(std::string &key, int by) {
    return crement(key, -by);
}

//COMMAND_1(bulk, get, string&)
bulk_result redis_actor_t::get(string &key) {
    std::string *result = get_string(key);
    boost::shared_ptr<std::string> res(result);
    return bulk_result(res);
}

//COMMAND_2(integer, getbit, string&, unsigned)
integer_result redis_actor_t::getbit(string &key, unsigned bit) {
    std::string *full_string = get_string(key);
    char byte = full_string->at(bit / 8);
    // TODO check that this is the right bit ordering
    return !!(byte & (1 << (bit % 8)));
}

//COMMAND_3(bulk, getrange, string&, int, int)
bulk_result redis_actor_t::getrange(string &key, int start, int end) {
    std::string *full_string = get_string(key);
    std::string *result = new std::string(full_string->begin() + start, full_string->begin() + end);
    boost::shared_ptr<std::string> res(result);
    return bulk_result(res);
}

//COMMAND_2(bulk, getset, string&, string&)
bulk_result redis_actor_t::getset(std::string &key, std::string &new_val) {
    set_oper_t oper(key, btree, 1234);

    std::string *result = NULL;
    redis_string_value_t *value; 
    if(oper.location.value.get() == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        oper.location.value->set_redis_type(REDIS_STRING);
        value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
    } else if(oper.location.value->get_redis_type() != REDIS_STRING) {
        return redis_error("Operation against key holding wrong kind of value");
    } else {
        value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
        result = new std::string();
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        blob.read_to_string(*result, oper.location.txn.get(), 0, blob.valuesize());
        blob.clear(oper.location.txn.get());
    }

    blob_t blob(value->get_content(), blob::btree_maxreflen);
    blob.append_region(oper.location.txn.get(), new_val.size());
    blob.write_from_string(new_val, oper.location.txn.get(), 0);

    return bulk_result(boost::shared_ptr<std::string>(result));
}

//COMMAND_1(integer, incr, string&)
integer_result redis_actor_t::incr(std::string &key) {
    return crement(key, 1);
}

//COMMAND_2(integer, incrby, string&, int)
integer_result redis_actor_t::incrby(std::string &key, int by) {
    return crement(key, by);
}

//COMMAND_N(multi_bulk, mget)
multi_bulk_result redis_actor_t::mget(std::vector<std::string> &keys) {
    boost::shared_ptr<std::vector<std::string> > result =
        boost::shared_ptr<std::vector<std::string> >(new std::vector<std::string>());
    for(std::vector<std::string>::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
        result->push_back(*get_string(*iter));
    }
    return multi_bulk_result(result);
}

//COMMAND_N(status, mset)
status_result redis_actor_t::mset(std::vector<std::string> &keys_vals) {
    for(std::vector<std::string>::iterator iter = keys_vals.begin(); iter != keys_vals.end(); iter += 2) {
        set(*iter, *(iter + 1));
    }

    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = OK;
    result->msg = (const char *)("OK");
    return result;
}

COMMAND_N(integer, msetnx)

//COMMAND_2(status, set, string&, string&)
status_result redis_actor_t::set(string &key, string &val) {
    set_oper_t oper(key, btree, 1234);
    if(oper.location.value.get() == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        oper.location.value->set_redis_type(REDIS_STRING);
    } else if(oper.location.value->get_redis_type() != REDIS_STRING) {
        return redis_error("Operation against key holding wrong kind of value");
    }

    redis_string_value_t *value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
    
    blob_t blob(value->get_content(), blob::btree_maxreflen);
    blob.clear(oper.location.txn.get());

    blob.append_region(oper.location.txn.get(), val.size());
    blob.write_from_string(val, oper.location.txn.get(), 0);

    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = OK;
    result->msg = (const char *)("OK");
    return result;
}

//COMMAND_3(integer, setbit, string&, unsigned, unsigned)
integer_result redis_actor_t::setbit(std::string &key, unsigned bit_index, unsigned bit_val) {
    set_oper_t oper(key, btree, 1234);
    if(oper.location.value.get() == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        oper.location.value->set_redis_type(REDIS_STRING);
    } else if(oper.location.value->get_redis_type() != REDIS_STRING) {
        return integer_result(redis_error("Operation against key holding wrong kind of value"));
    }

    redis_string_value_t *value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
    blob_t blob(value->get_content(), blob::btree_maxreflen);
    int64_t difference = (bit_index / 8 + 1) - blob.valuesize();
    if(difference > 0) {
        blob.append_region(oper.location.txn.get(), difference);
    }

    buffer_group_t dest;
    boost::scoped_ptr<blob_acq_t> acq(new blob_acq_t);
    blob.expose_region(oper.location.txn.get(), rwi_write, bit_index / 8, 1, &dest, acq.get());

    buffer_group_t::buffer_t buff =  dest.get_buffer(0);
    int old_val = !!(reinterpret_cast<uint8_t *>(buff.data)[0] & (1 << (bit_index % 8)));
    reinterpret_cast<uint8_t *>(buff.data)[0] =
        (reinterpret_cast<uint8_t *>(buff.data)[0] | (1 << (bit_index % 8))) ^ (!!!bit_val << (bit_index % 8));

    return integer_result(old_val);
}

COMMAND_3(status, setex, string&, unsigned, string&)

//COMMAND_3(integer, setrange, string&, unsigned, string&)
integer_result redis_actor_t::setrange(std::string &key, unsigned offset, std::string &val) {
    set_oper_t oper(key, btree, 1234);
    if(oper.location.value.get() == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        oper.location.value->set_redis_type(REDIS_STRING);
    } else if(oper.location.value->get_redis_type() != REDIS_STRING) {
        return integer_result(redis_error("Operation against key holding wrong kind of value"));
    }
     
    redis_string_value_t *value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
    blob_t blob(value->get_content(), blob::btree_maxreflen);
    int64_t difference = (offset + val.size()) - blob.valuesize();
    if(difference > 0) {
        blob.append_region(oper.location.txn.get(), difference);
    }
    blob.write_from_string(val, oper.location.txn.get(), offset);

    return integer_result(blob.valuesize());
}

//COMMAND_1(integer, Strlen, string&)
integer_result redis_actor_t::Strlen(std::string &key) {
    std::string *string_result = get_string(key);
    return integer_result(string_result->size());
}
