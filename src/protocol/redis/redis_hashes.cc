#include "protocol/redis/redis_util.hpp"
#include "btree/iteration.hpp"
#include <boost/lexical_cast.hpp>

// Utility functions for hashes

struct hash_set_oper_t : set_oper_t {
    hash_set_oper_t(std::string &key, std::string &field, btree_slice_t *btree, int64_t timestamp) :
        set_oper_t(key, btree, timestamp),
        nested_key(field),
        nested_sizer(btree->cache()->get_block_size())
    {
        // set_oper_t constructor finds this hash, here we find the field in the nested btree

        redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
        if(value == NULL) {
            // Hash doesn't exist, we create it on creation of the first field
            scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
            location.value.swap(smrsv);
            location.value->set_redis_type(REDIS_HASH);
            value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
            value->get_root() = NULL_BLOCK_ID;
        } else if(value->get_redis_type() != REDIS_HASH) {
            // TODO in this case?
        }

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(value->get_root()));
        got_superblock_t nested_superblock;
        nested_superblock.sb.swap(nested_btree_sb);
        nested_superblock.txn = location.txn;

        find_keyvalue_location_for_write(&nested_sizer, &nested_superblock, nested_key.key(), tstamp, &nested_loc);
    }

    ~hash_set_oper_t() {
        // Apply change to sub tree
        apply_keyvalue_change(&nested_sizer, &nested_loc, nested_key.key(), tstamp);
        
        redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
        virtual_superblock_t *sb = reinterpret_cast<virtual_superblock_t *>(nested_loc.sb.get());
        value->get_root() = sb->get_root_block_id();
        // Parent change will be applied by parent destructor
    }

    keyvalue_location_t<redis_nested_string_value_t> nested_loc;

protected:
    btree_key_buffer_t nested_key;
    value_sizer_t<redis_nested_string_value_t> nested_sizer;
};

std::string *redis_actor_t::hget_string(std::string &key, std::string &field) {
    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    std::string *result = NULL;
    if(value == NULL) {
        //Error key doesn't exist
    } else if(value->get_redis_type() != REDIS_HASH) {
        //Error key isn't a string
    } else {
        value_sizer_t<redis_nested_string_value_t> nested_sizer(btree->cache()->get_block_size());

        got_superblock_t nested_superblock;
        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(value->get_root()));
        nested_superblock.sb.swap(nested_btree_sb);
        nested_superblock.txn = location.txn;

        btree_key_buffer_t nested_key(field);
        keyvalue_location_t<redis_nested_string_value_t> nested_loc;

        find_keyvalue_location_for_read(&nested_sizer, &nested_superblock, nested_key.key(), &nested_loc);

        redis_nested_string_value_t *value = nested_loc.value.get();
        if(value != NULL) {
            result = new std::string();
            blob_t blob(value->content, blob::btree_maxreflen);
            blob.read_to_string(*result, nested_loc.txn.get(), 0, blob.valuesize());
        } // else result remains NULL
    }

    return result;
}

//Hash Operations

//COMMAND_N(integer, hdel)
integer_result redis_actor_t::hdel(std::vector<std::string> &key_fields) {
    int deleted = 0;

    std::string key = key_fields[0];
    set_oper_t oper(key, btree, 1234);

    value_sizer_t<redis_nested_string_value_t> nested_sizer(btree->cache()->get_block_size());

    if(oper.location.value.get() != NULL ||
       oper.location.value->get_redis_type() == REDIS_HASH) {
        
        redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(oper.location.value.get());
        block_id_t root = value->get_root();

        for(std::vector<std::string>::iterator iter = key_fields.begin() + 1;
                iter != key_fields.end(); ++iter) {
            btree_key_buffer_t field(*iter);

            // The nested superblock will be consumed for each field lookup
            boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
            got_superblock_t nested_superblock;
            nested_superblock.sb.swap(nested_btree_sb);
            nested_superblock.txn = oper.location.txn;

            keyvalue_location_t<redis_nested_string_value_t> loc;
            find_keyvalue_location_for_write(&nested_sizer, &nested_superblock, field.key(), oper.tstamp, &loc);

            redis_nested_string_value_t *nst_str = loc.value.get();
            if(nst_str != NULL) {
                // Delete the value
                blob_t blob(nst_str->content, blob::btree_maxreflen);
                blob.clear(oper.location.txn.get());

                scoped_malloc<redis_nested_string_value_t> null;
                loc.value.swap(null);

                apply_keyvalue_change(&nested_sizer, &loc, field.key(), oper.tstamp);

                // This delete may have changed the root which we need for the next operation
                root = reinterpret_cast<virtual_superblock_t *>(loc.sb.get())->get_root_block_id();

                deleted++;
            }
        }

        // If any of the deletes change the root we now have to make that change in the hash value
        value->get_root() = root;
    }

    return integer_result(deleted);
}

//COMMAND_2(integer, hexists, string&, string&)
integer_result redis_actor_t::hexists(std::string &key, std::string &field) {
    std::string *s = hget_string(key, field);
    if(s != NULL) delete s;
    return integer_result(!!s);
}

//COMMAND_2(bulk, hget, string&, string&)
bulk_result redis_actor_t::hget(std::string &key, std::string &field) {
    return bulk_result(boost::shared_ptr<std::string>(hget_string(key, field)));
}

//COMMAND_1(multi_bulk, hgetall, string&)
multi_bulk_result redis_actor_t::hgetall(std::string &key) {
    boost::shared_ptr<std::vector<std::string> > vals(new std::vector<std::string>());

    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    if(value != NULL) {
        block_id_t root = value->get_root();

        boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> >
            nested_sizer(new value_sizer_t<redis_nested_string_value_t>(btree->cache()->get_block_size()));

        store_key_t none_key;
        none_key.size = 0;

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        slice_keys_iterator_t<redis_nested_string_value_t>
            tree_iter(nested_sizer, location.txn, nested_btree_sb, btree->home_thread(),
            rget_bound_none, none_key, rget_bound_none, none_key);

        while (true) {
            boost::optional<key_value_pair_t<redis_nested_string_value_t> > next = tree_iter.next();
            if(next) {
                std::string k = next.get().key;
                vals->push_back(k);

                boost::shared_array<char> val = next.get().value;
                blob_t blob(val.get(), blob::btree_maxreflen);
                std::string this_val(blob.valuesize(), '\0');
                blob.read_to_string(this_val, location.txn.get(), 0, blob.valuesize());
                vals->push_back(this_val);
            } else {
                break;
            }
        }
    }

    return multi_bulk_result(vals);

}

//COMMAND_3(integer, hincrby, string&, string&, int)
integer_result redis_actor_t::hincrby(std::string &key, std::string &field, int by) {
    hash_set_oper_t oper(key, field, btree, 1234);

    int int_value = 0;
    std::string int_string;
    redis_nested_string_value_t *value = oper.nested_loc.value.get();
    if(value == NULL) {
        scoped_malloc<redis_nested_string_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
        oper.nested_loc.value.swap(smrsv);
        value = reinterpret_cast<redis_nested_string_value_t *>(oper.nested_loc.value.get());
    } else {
        blob_t blob(value->content, blob::btree_maxreflen);
        blob.read_to_string(int_string, oper.location.txn.get(), 0, blob.valuesize());
        blob.clear(oper.location.txn.get());
        try {
            int_value = boost::lexical_cast<int64_t>(int_string);
        } catch(boost::bad_lexical_cast &) {
            return integer_result(redis_error("value is not an integer or out of range"));
        }
    }

    int_value += by;
    int_string = boost::lexical_cast<std::string>(int_value);

    blob_t blob(value->content, blob::btree_maxreflen);
    blob.append_region(oper.location.txn.get(), int_string.size());
    blob.write_from_string(int_string, oper.location.txn.get(), 0);
    
    return integer_result(int_value);
}

//COMMAND_1(multi_bulk, hkeys, string&)
multi_bulk_result redis_actor_t::hkeys(std::string &key) {
    boost::shared_ptr<std::vector<std::string> > vals(new std::vector<std::string>());

    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    if(value != NULL) {
        block_id_t root = value->get_root();

        boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> >
            nested_sizer(new value_sizer_t<redis_nested_string_value_t>(btree->cache()->get_block_size()));

        store_key_t none_key;
        none_key.size = 0;

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        slice_keys_iterator_t<redis_nested_string_value_t>
            tree_iter(nested_sizer, location.txn, nested_btree_sb, btree->home_thread(),
            rget_bound_none, none_key, rget_bound_none, none_key);

        while (true) {
            boost::optional<key_value_pair_t<redis_nested_string_value_t> > next = tree_iter.next();
            if(next) {
                std::string k = next.get().key;
                vals->push_back(k);
            } else {
                break;
            }
        }
    }

    return multi_bulk_result(vals);
}

//COMMAND_1(integer, hlen, string&)
integer_result redis_actor_t::hlen(std::string &key) {
    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    block_id_t root = value->get_root();

    boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> > nested_sizer(new value_sizer_t<redis_nested_string_value_t>(btree->cache()->get_block_size()));

    store_key_t none_key;
    none_key.size = 0;

    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
    slice_keys_iterator_t<redis_nested_string_value_t> tree_iter(nested_sizer, location.txn, nested_btree_sb, btree->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);

    int count = 0;
    while (true) {
        boost::optional<key_value_pair_t<redis_nested_string_value_t> > next = tree_iter.next();
        if(next) {
            count++;
        } else {
            break;
        }
    }

    return integer_result(count);
}

//COMMAND_N(multi_bulk, hmget)
multi_bulk_result redis_actor_t::hmget(std::vector<std::string> &key_fields) {
    std::string key = key_fields[0];
    value_sizer_t<redis_nested_string_value_t> nested_sizer(btree->cache()->get_block_size());
    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>());

    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    if((value != NULL) || (value->get_redis_type() == REDIS_HASH)) {
        
        block_id_t root = value->get_root();

        for(std::vector<std::string>::iterator iter = key_fields.begin() + 1;
                iter != key_fields.end(); ++iter) {
            btree_key_buffer_t field(*iter);

            // The nested superblock will be consumed for each field lookup
            boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
            got_superblock_t nested_superblock;
            nested_superblock.sb.swap(nested_btree_sb);
            nested_superblock.txn = location.txn;

            keyvalue_location_t<redis_nested_string_value_t> loc;
            find_keyvalue_location_for_read(&nested_sizer, &nested_superblock, field.key(), &loc);

            redis_nested_string_value_t *nst_str = loc.value.get();
            if(nst_str != NULL) {
                blob_t blob(nst_str->content, blob::btree_maxreflen);
                std::string res(blob.valuesize(), '\0');
                blob.read_to_string(res, location.txn.get(), 0, blob.valuesize());
                result->push_back(res);
            }
        }
    }

    return multi_bulk_result(result);
}

//COMMAND_N(status, hmset)
status_result redis_actor_t::hmset(std::vector<std::string> &key_fields_values) {
    std::string key = key_fields_values[0];
    set_oper_t oper(key, btree, 1234);

    value_sizer_t<redis_nested_string_value_t> nested_sizer(btree->cache()->get_block_size());

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(oper.location.value.get());
    if(value == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        value = reinterpret_cast<redis_hash_value_t *>(oper.location.value.get());
        value->set_redis_type(REDIS_HASH);
        value->get_root() = NULL_BLOCK_ID;
    } else if(value->get_redis_type() != REDIS_HASH) {
        return redis_error("Hash operation attempted against a non-hash key");
    }
    
    block_id_t root = value->get_root();

    for(std::vector<std::string>::iterator iter = key_fields_values.begin() + 1;
            iter != key_fields_values.end(); ++iter) {
        btree_key_buffer_t field(*iter);

        // The nested superblock will be consumed for each field lookup
        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        got_superblock_t nested_superblock;
        nested_superblock.sb.swap(nested_btree_sb);
        nested_superblock.txn = oper.location.txn;

        keyvalue_location_t<redis_nested_string_value_t> loc;
        find_keyvalue_location_for_write(&nested_sizer, &nested_superblock, field.key(), oper.tstamp, &loc);

        redis_nested_string_value_t *nst_str = loc.value.get();
        if(nst_str == NULL) {
            scoped_malloc<redis_nested_string_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
            memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
            loc.value.swap(smrsv);
            nst_str = reinterpret_cast<redis_nested_string_value_t *>(loc.value.get());
        }

        blob_t blob(nst_str->content, blob::btree_maxreflen);

        // Clear old value
        blob.clear(loc.txn.get());
        blob.append_region(loc.txn.get(), iter->size());

        // Increment to the value of this field
        ++iter;
        blob.write_from_string(*iter, loc.txn.get(), 0);

        apply_keyvalue_change(&nested_sizer, &loc, field.key(), oper.tstamp);

        // This delete may have changed the root which we need for the next operation
        root = reinterpret_cast<virtual_superblock_t *>(loc.sb.get())->get_root_block_id();
    }

    // If any of the deletes change the root we now have to make that change in the hash value
    value->get_root() = root;

    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = OK;
    result->msg = "OK";
    return result;
}

//COMMAND_3(integer, hset, string&, string&, string&)
integer_result redis_actor_t::hset(std::string &key, std::string &field, std::string &f_value) {
    hash_set_oper_t oper(key, field, btree, 1234);
    redis_nested_string_value_t *value = oper.nested_loc.value.get();
    int result = 0;
    if(value == NULL) {
        scoped_malloc<redis_nested_string_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
        oper.nested_loc.value.swap(smrsv);
        value = reinterpret_cast<redis_nested_string_value_t *>(oper.nested_loc.value.get());
        result = 1;
    }    

    blob_t blob(value->content, blob::btree_maxreflen);
    blob.clear(oper.location.txn.get());

    blob.append_region(oper.location.txn.get(), f_value.size());
    blob.write_from_string(f_value, oper.location.txn.get(),  0);

    return integer_result(result);
}

//COMMAND_3(integer, hsetnx, string&, string&, string&)
integer_result redis_actor_t::hsetnx(std::string &key, std::string &field, std::string &f_value) {
    hash_set_oper_t oper(key, field, btree, 1234);
    redis_nested_string_value_t *value = oper.nested_loc.value.get();
    int result = 0;
    if(value == NULL) {
        scoped_malloc<redis_nested_string_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
        oper.nested_loc.value.swap(smrsv);
        value = reinterpret_cast<redis_nested_string_value_t *>(oper.nested_loc.value.get());

        blob_t blob(value->content, blob::btree_maxreflen);
        blob.append_region(oper.location.txn.get(), f_value.size());
        blob.write_from_string(f_value, oper.location.txn.get(),  0);

        result = 1;
    }    

    return integer_result(result);
}

//COMMAND_1(multi_bulk, hvals, string&)
multi_bulk_result redis_actor_t::hvals(std::string &key) {
    boost::shared_ptr<std::vector<std::string> > vals(new std::vector<std::string>());

    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    if(value != NULL) {
        block_id_t root = value->get_root();

        boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> >
            nested_sizer(new value_sizer_t<redis_nested_string_value_t>(btree->cache()->get_block_size()));

        store_key_t none_key;
        none_key.size = 0;

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        slice_keys_iterator_t<redis_nested_string_value_t>
            tree_iter(nested_sizer, location.txn, nested_btree_sb, btree->home_thread(),
            rget_bound_none, none_key, rget_bound_none, none_key);

        while (true) {
            boost::optional<key_value_pair_t<redis_nested_string_value_t> > next = tree_iter.next();
            if(next) {
                boost::shared_array<char> val = next.get().value;
                blob_t blob(val.get(), blob::btree_maxreflen);
                std::string this_val(blob.valuesize(), '\0');
                blob.read_to_string(this_val, location.txn.get(), 0, blob.valuesize());
                vals->push_back(this_val);
            } else {
                break;
            }
        }
    }

    return multi_bulk_result(vals);
}
