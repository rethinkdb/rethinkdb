#include "redis/redis_util.hpp"
#include "btree/iteration.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

// Utility functions for hashes

struct hash_read_oper_t : public read_oper_t {
    hash_read_oper_t(std::string &key, btree_slice_t *btr, order_token_t otok) :
        read_oper_t(key, btr, otok),
        btree(btr)
    {
        redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
        if(!value) {
            root = NULL_BLOCK_ID;
        } else if(value->get_redis_type() != REDIS_HASH) {
            throw "Operation against key holding the wrong kind of value";
        } else {
            root = value->get_root();
        }
    }

    bool get(std::string &field, std::string *str_out) {
        got_superblock_t nested_superblock;
        nested_superblock.sb.reset(new virtual_superblock_t(root));

        btree_key_buffer_t nested_key(field);
        keyvalue_location_t<redis_nested_string_value_t> nested_loc;

        find_keyvalue_location_for_read(txn.get(), &nested_superblock, nested_key.key(), &nested_loc);

        redis_nested_string_value_t *value = nested_loc.value.get();
        if(value != NULL) {
            blob_t blob(value->content, blob::btree_maxreflen);
            blob.read_to_string(*str_out, txn.get(), 0, blob.valuesize());
            return true;
        } 

        return false;
    }

    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > iterator() {
        boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> >
            sizer_ptr(new value_sizer_t<redis_nested_string_value_t>(btree->cache()->get_block_size()));

        store_key_t none_key;
        none_key.size = 0;

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));

        slice_keys_iterator_t<redis_nested_string_value_t> *tree_iter =
            new slice_keys_iterator_t<redis_nested_string_value_t>(sizer_ptr, txn.get(),
                nested_btree_sb, btree->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);

        boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > transform_iter(
                new transform_iterator_t<key_value_pair_t<redis_nested_string_value_t>,
                std::pair<std::string, std::string> >(
                boost::bind(&hash_read_oper_t::transform_value, this, _1), tree_iter));

        return transform_iter;
    }

    int hash_size() {
        return reinterpret_cast<redis_hash_value_t *>(location.value.get())->get_sub_size();
    }

protected:
    btree_slice_t *btree;
    block_id_t root;

    std::pair<std::string, std::string> transform_value(const key_value_pair_t<redis_nested_string_value_t> &pair) {
        blob_t blob(pair.value.get(), blob::btree_maxreflen); 
        std::string str;
        blob.read_to_string(str, txn.get(), 0, blob.valuesize());

        return std::pair<std::string, std::string>(pair.key, str);
    }

};

struct hash_set_oper_t : set_oper_t {
    hash_set_oper_t(std::string &key, btree_slice_t *btree, timestamp_t timestamp, order_token_t otok) :
        set_oper_t(key, btree, timestamp, otok)
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
            throw "Operation against key holding the wrong kind of value";
        }
        
        root = value->get_root();
    }

    ~hash_set_oper_t() {
        // Reset root if changed
        redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
        value->get_root() = root;
    }

    void increment_size(int by) {
        reinterpret_cast<redis_set_value_t *>(location.value.get())->get_sub_size() += by;
    }

    int incr_field_by(std::string &field, int by) {
        find_field f(this, field);
        return f.incr(by);
    }

    bool del_field(std::string &field) {
        find_field f(this, field);

        if(f.loc.value.get() == NULL) {
            return false;
        }

        scoped_malloc<redis_nested_string_value_t> null;
        f.loc.value.swap(null);
        f.apply_change();
        return true;
    }

    bool set_field(std::string &field, std::string &value, bool clear_old = true) {
        find_field f(this, field);
        bool result = f.set(value, clear_old);
        f.apply_change();
        return result;
    }

protected:
    struct find_field {
        find_field(hash_set_oper_t *ths_ptr, std::string &member):
            ths(ths_ptr),
            nested_key(member)
        {
            boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(ths->root));
            got_superblock_t nested_superblock;
            nested_superblock.sb.swap(nested_btree_sb);

            find_keyvalue_location_for_write(ths->txn.get(), &nested_superblock, nested_key.key(), &loc);
        }

        void apply_change() {
            // TODO hook up timestamp once Tim figures out what to do with the timestamp
            apply_keyvalue_change(ths->txn.get(), &loc, nested_key.key(), repli_timestamp_t::invalid /*ths->timestamp*/);
            virtual_superblock_t *sb = reinterpret_cast<virtual_superblock_t *>(loc.sb.get());
            ths->root = sb->get_root_block_id();
        }

        int incr(int by) {
            return incr_loc<redis_nested_string_value_t>(ths->txn.get(), loc, by);
        }

        bool set(std::string &value, bool clear_old) {
            redis_nested_string_value_t *nst_str = loc.value.get();
            bool created = false;
            if(nst_str == NULL) {
                scoped_malloc<redis_nested_string_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
                memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
                loc.value.swap(smrsv);
                nst_str = loc.value.get();
                created = true;
            } else if(!clear_old) {
                return false;
            }

            blob_t blob(nst_str->content, blob::btree_maxreflen);

            blob.clear(ths->txn.get());
            blob.append_region(ths->txn.get(), value.size());
            blob.write_from_string(value, ths->txn.get(), 0);

            return created;
        }

        hash_set_oper_t *ths;
        btree_key_buffer_t nested_key;
        keyvalue_location_t<redis_nested_string_value_t> loc;
    };

    block_id_t root;
};

//Hash Operations

//WRITE(hdel)
redis_protocol_t::indicated_key_t redis_protocol_t::hdel::get_keys() {
    return one[0];
}

SHARD_W(hdel)

EXECUTE_W(hdel) {
    int deleted = 0;

    std::string key = one[0];
    hash_set_oper_t oper(key, btree, timestamp, otok);

    for(std::vector<std::string>::iterator iter = one.begin() + 1;
            iter != one.end(); ++iter) {
        if(oper.del_field(*iter)) {
            deleted++;
        }
    }

    oper.increment_size(-deleted);
    return int_response(deleted);
}

//READ(hexists)
KEYS(hexists)
SHARD_R(hexists)
PARALLEL(hexists)

EXECUTE_R(hexists) {
    hash_read_oper_t oper(one, btree, otok);
    std::string s;
    if(oper.get(two, &s))
        return int_response(1);
    return int_response(0);
}

//READ(hget)
KEYS(hget)
SHARD_R(hget)
PARALLEL(hget)

EXECUTE_R(hget) {
    hash_read_oper_t oper(one, btree, otok);
    std::string s;
    if(oper.get(two, &s))
        return bulk_response(s);
    return read_response_t();
}

//READ(hgetall)
KEYS(hgetall)
SHARD_R(hgetall)
PARALLEL(hgetall)

EXECUTE_R(hgetall) {
    hash_read_oper_t oper(one, btree, otok);
    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > iter = oper.iterator();
    std::vector<std::string> result;

    while (true) {
        boost::optional<std::pair<std::string, std::string> > next = iter->next();
        if(next) {
            result.push_back(next.get().first);
            result.push_back(next.get().second);
        } else {
            break;
        }
    }

    return read_response_t(new multi_bulk_result_t(result));
}

//WRITE(hincrby)
KEYS(hincrby)
SHARD_W(hincrby)

EXECUTE_W(hincrby) {
    hash_set_oper_t oper(one, btree, timestamp, otok);
    int result = oper.incr_field_by(two, three);
    return int_response(result);
}

//READ(hkeys)
KEYS(hkeys)
SHARD_R(hkeys)
PARALLEL(hkeys)

EXECUTE_R(hkeys) {
    hash_read_oper_t oper(one, btree, otok);
    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > iter = oper.iterator();

    std::vector<std::string> keys;

    while (true) {
        boost::optional<std::pair<std::string, std::string> > next = iter->next();
        if(next) {
            std::string k = next.get().first;
            keys.push_back(k);
        } else {
            break;
        }
    }

    return read_response_t(new multi_bulk_result_t(keys));
}

//READ(hlen)
KEYS(hlen)
SHARD_R(hlen)
PARALLEL(hlen)

EXECUTE_R(hlen) {
    hash_read_oper_t oper(one, btree, otok);
    return int_response(oper.hash_size());
}

//READ(hmget)
redis_protocol_t::indicated_key_t redis_protocol_t::hmget::get_keys() {
    return one[0];
}

SHARD_R(hmget)
PARALLEL(hmget)

EXECUTE_R(hmget) {
    std::string key = one[0];
    hash_read_oper_t oper(key, btree, otok);

    std::vector<std::string> result;
    for(std::vector<std::string>::iterator iter = one.begin() + 1;
            iter != one.end(); ++iter) {
        std::string val;
        if(oper.get(*iter, &val)) {
            result.push_back(val);
        }
    }

    return read_response_t(new multi_bulk_result_t(result));
}

//WRITE(hmset)
redis_protocol_t::indicated_key_t redis_protocol_t::hmset::get_keys() {
    return one[0];
}

SHARD_W(hmset)

EXECUTE_W(hmset) {
    std::string key = one[0];
    hash_set_oper_t oper(key, btree, timestamp, otok);

    int num_set = 0;
    for(std::vector<std::string>::iterator iter = one.begin() + 1;
            iter != one.end(); ++iter) {
        std::string field = *iter;
        ++iter;
        std::string value = *iter;

        if(oper.set_field(field, value)) {
            num_set++; 
        }
    }

    return write_response_t(new ok_result_t());
}

//WRITE(hset)
KEYS(hset)
SHARD_W(hset)

EXECUTE_W(hset) {
    hash_set_oper_t oper(one, btree, timestamp, otok);
    bool result = oper.set_field(two, three);

    return int_response(result);
}

//WRITE(hsetnx)
KEYS(hsetnx)
SHARD_W(hsetnx)

EXECUTE_W(hsetnx) {
    hash_set_oper_t oper(one, btree, timestamp, otok);
    bool result = oper.set_field(two, three, false);

    return int_response(result);
}

//READ(hvals)
KEYS(hvals)
SHARD_R(hvals)
PARALLEL(hvals)

EXECUTE_R(hvals) {
    hash_read_oper_t oper(one, btree, otok);
    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > iter = oper.iterator();

    std::vector<std::string> vals;

    while (true) {
        boost::optional<std::pair<std::string, std::string> > next = iter->next();
        if(next) {
            std::string k = next.get().second;
            vals.push_back(k);
        } else {
            break;
        }
    }

    return read_response_t(new multi_bulk_result_t(vals));
}
