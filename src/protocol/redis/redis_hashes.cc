#include "protocol/redis/redis_util.hpp"
#include "btree/iteration.hpp"
#include <boost/lexical_cast.hpp>

// Utility functions for hashes

struct hash_read_oper_t : read_oper_t {
    hash_read_oper_t(std::string &key, btree_slice_t *btr) :
        read_oper_t(key, btr),
        btree(btr)
    {
        redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
        if(!value) {
            root = NULL_BLOCK_ID;
        } else if(value->get_redis_type() != REDIS_HASH) {
            //throw
        } else {
            root = value->get_root();
        }
    }

    std::string *get(std::string &field) {
        value_sizer_t<redis_nested_string_value_t> nested_sizer(btree->cache()->get_block_size());

        got_superblock_t nested_superblock;
        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        nested_superblock.sb.swap(nested_btree_sb);
        nested_superblock.txn = location.txn;

        btree_key_buffer_t nested_key(field);
        keyvalue_location_t<redis_nested_string_value_t> nested_loc;

        find_keyvalue_location_for_read(&nested_sizer, &nested_superblock, nested_key.key(), &nested_loc);

        std::string *result = NULL;
        redis_nested_string_value_t *value = nested_loc.value.get();
        if(value != NULL) {
            result = new std::string();
            blob_t blob(value->content, blob::btree_maxreflen);
            blob.read_to_string(*result, nested_loc.txn.get(), 0, blob.valuesize());
        } // else result remains NULL

        return result;
    }

    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > iterator() {
        boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> >
            sizer_ptr(new value_sizer_t<redis_nested_string_value_t>(btree->cache()->get_block_size()));

        store_key_t none_key;
        none_key.size = 0;

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));

        slice_keys_iterator_t<redis_nested_string_value_t> *tree_iter =
                new slice_keys_iterator_t<redis_nested_string_value_t>(sizer_ptr, location.txn,
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
        blob.read_to_string(str, location.txn.get(), 0, blob.valuesize());

        return std::pair<std::string, std::string>(pair.key, str);
    }

};

struct hash_set_oper_t : set_oper_t {
    hash_set_oper_t(std::string &key, btree_slice_t *btree, int64_t timestamp) :
        set_oper_t(key, btree, timestamp),
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
            nested_superblock.txn = ths->location.txn;

            find_keyvalue_location_for_write(&ths->nested_sizer, &nested_superblock, nested_key.key(), ths->tstamp, &loc);
        }
        
        void apply_change() {
            apply_keyvalue_change(&ths->nested_sizer, &loc, nested_key.key(), ths->tstamp);
            virtual_superblock_t *sb = reinterpret_cast<virtual_superblock_t *>(loc.sb.get());
            ths->root = sb->get_root_block_id();
        }

        int incr(int by) {
           return incr_loc<redis_nested_string_value_t>(loc, by);
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

            blob.clear(loc.txn.get());
            blob.append_region(loc.txn.get(), value.size());
            blob.write_from_string(value, loc.txn.get(), 0);

            return created;
        }

        hash_set_oper_t *ths;
        btree_key_buffer_t nested_key;
        keyvalue_location_t<redis_nested_string_value_t> loc;
    };

    block_id_t root;
    value_sizer_t<redis_nested_string_value_t> nested_sizer;
};

//Hash Operations

//COMMAND_N(integer, hdel)
integer_result redis_actor_t::hdel(std::vector<std::string> &key_fields) {
    int deleted = 0;

    std::string key = key_fields[0];
    hash_set_oper_t oper(key, btree, 1234);

    for(std::vector<std::string>::iterator iter = key_fields.begin() + 1;
            iter != key_fields.end(); ++iter) {
        if(oper.del_field(*iter)) {
            deleted++;
        }
    }

    oper.increment_size(-deleted);
    return integer_result(deleted);
}

//COMMAND_2(integer, hexists, string&, string&)
integer_result redis_actor_t::hexists(std::string &key, std::string &field) {
    hash_read_oper_t oper(key, btree);
    std::string *s = oper.get(field);
    if(s != NULL) delete s;
    return integer_result(!!s);
}

//COMMAND_2(bulk, hget, string&, string&)
bulk_result redis_actor_t::hget(std::string &key, std::string &field) {
    hash_read_oper_t oper(key, btree);
    return bulk_result(boost::shared_ptr<std::string>(oper.get(field)));
}

//COMMAND_1(multi_bulk, hgetall, string&)
multi_bulk_result redis_actor_t::hgetall(std::string &key) {
    hash_read_oper_t oper(key, btree);
    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > iter = oper.iterator();
    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>());

    while (true) {
        boost::optional<std::pair<std::string, std::string> > next = iter->next();
        if(next) {
            result->push_back(next.get().first);
            result->push_back(next.get().second);
        } else {
            break;
        }
    }

    return multi_bulk_result(result);
}

//COMMAND_3(integer, hincrby, string&, string&, int)
integer_result redis_actor_t::hincrby(std::string &key, std::string &field, int by) {
    hash_set_oper_t oper(key, btree, 1234);
    int result = oper.incr_field_by(field, by);
    return integer_result(result);
}

//COMMAND_1(multi_bulk, hkeys, string&)
multi_bulk_result redis_actor_t::hkeys(std::string &key) {
    hash_read_oper_t oper(key, btree);
    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > iter = oper.iterator();

    boost::shared_ptr<std::vector<std::string> > keys(new std::vector<std::string>());

    while (true) {
        boost::optional<std::pair<std::string, std::string> > next = iter->next();
        if(next) {
            std::string k = next.get().first;
            keys->push_back(k);
        } else {
            break;
        }
    }

    return multi_bulk_result(keys);
}

//COMMAND_1(integer, hlen, string&)
integer_result redis_actor_t::hlen(std::string &key) {
    hash_read_oper_t oper(key, btree);
    return integer_result(oper.hash_size());
}

//COMMAND_N(multi_bulk, hmget)
multi_bulk_result redis_actor_t::hmget(std::vector<std::string> &key_fields) {
    std::string key = key_fields[0];
    hash_read_oper_t oper(key, btree);

    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>());
    for(std::vector<std::string>::iterator iter = key_fields.begin() + 1;
            iter != key_fields.end(); ++iter) {
        std::string *val = oper.get(*iter);

        if(val != NULL) {
            result->push_back(*val);
            delete val;
        }
    }

    return multi_bulk_result(result);
}

//COMMAND_N(status, hmset)
status_result redis_actor_t::hmset(std::vector<std::string> &key_fields_values) {
    std::string key = key_fields_values[0];
    hash_set_oper_t oper(key, btree, 1234);

    int num_set = 0;
    for(std::vector<std::string>::iterator iter = key_fields_values.begin() + 1;
            iter != key_fields_values.end(); ++iter) {
        std::string field = *iter;
        ++iter;
        std::string value = *iter;

        if(oper.set_field(field, value)) {
            num_set++; 
        }
    }

    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = OK;
    result->msg = "OK";
    return result;
}

//COMMAND_3(integer, hset, string&, string&, string&)
integer_result redis_actor_t::hset(std::string &key, std::string &field, std::string &f_value) {
    hash_set_oper_t oper(key, btree, 1234);
    bool result = oper.set_field(field, f_value);

    return integer_result(result);
}

//COMMAND_3(integer, hsetnx, string&, string&, string&)
integer_result redis_actor_t::hsetnx(std::string &key, std::string &field, std::string &f_value) {
    hash_set_oper_t oper(key, btree, 1234);
    bool result = oper.set_field(field, f_value, false);

    return integer_result(result);
}

//COMMAND_1(multi_bulk, hvals, string&)
multi_bulk_result redis_actor_t::hvals(std::string &key) {
    hash_read_oper_t oper(key, btree);
    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > iter = oper.iterator();

    boost::shared_ptr<std::vector<std::string> > vals(new std::vector<std::string>());

    while (true) {
        boost::optional<std::pair<std::string, std::string> > next = iter->next();
        if(next) {
            std::string k = next.get().second;
            vals->push_back(k);
        } else {
            break;
        }
    }

    return multi_bulk_result(vals);
}
