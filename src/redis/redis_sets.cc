#include "redis/redis_util.hpp"
#include "btree/iteration.hpp"
#include <boost/bind.hpp>
#include <iostream>

// Set utilities
struct set_set_oper_t : set_oper_t {
    set_set_oper_t(std::string &key, btree_slice_t *btree, timestamp_t timestamp, order_token_t otok) :
        set_oper_t(key, btree, timestamp, otok)
    { 
        redis_set_value_t *value = reinterpret_cast<redis_set_value_t *>(location.value.get());
        if(value == NULL) {
            scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
            location.value.swap(smrsv);
            location.value->set_redis_type(REDIS_SET);
            value = reinterpret_cast<redis_set_value_t *>(location.value.get());
            value->get_root() = NULL_BLOCK_ID;
            value->get_sub_size() = 0;
        } else if(value->get_redis_type() != REDIS_SET) {
            throw "ERR Operation against key holding the wrong kind of value";
        }

        root = value->get_root();
    }

    bool add_member(std::string &member) {
        find_member mem(this, member);
        
        if(mem.loc.value.get() == NULL) {
            scoped_malloc<redis_nested_set_value_t> smrsv(0);
            mem.loc.value.swap(smrsv);
            mem.apply_change();
            return true;
        } // else this operation has no effect

        return false;
    }

    bool remove_member(std::string &member) {
        find_member mem(this, member);

        if(mem.loc.value.get() == NULL) {
            return false;
        }

        scoped_malloc<redis_nested_set_value_t> null;
        mem.loc.value.swap(null);
        mem.apply_change();
        return true;
    }

    void increment_size(int by) {
        reinterpret_cast<redis_set_value_t *>(location.value.get())->get_sub_size() += by;
    }

    ~set_set_oper_t() {
        // Reset root if changed
        redis_set_value_t *value = reinterpret_cast<redis_set_value_t *>(location.value.get());
        value->get_root() = root;
    }

protected:
    struct find_member {
        find_member(set_set_oper_t *ths_ptr, std::string &member):
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
            apply_keyvalue_change(ths->txn.get(), &loc, nested_key.key(), convert_to_repli_timestamp(ths->timestamp));
            virtual_superblock_t *sb = reinterpret_cast<virtual_superblock_t *>(loc.sb.get());
            ths->root = sb->get_root_block_id();
        }

        set_set_oper_t *ths;
        btree_key_buffer_t nested_key;
        keyvalue_location_t<redis_nested_set_value_t> loc;
    };

    block_id_t root;
};

struct set_read_oper_t : read_oper_t {
    set_read_oper_t(std::string &key, btree_slice_t *btr, order_token_t otok) :
        read_oper_t(key, btr, otok),
        btree(btr)
    {
        value = reinterpret_cast<redis_set_value_t *>(location.value.get());
        if(!value) {
            root = NULL_BLOCK_ID;
        } else if(value->get_redis_type() != REDIS_SET) {
            throw "ERR Operation against key holding the wrong kind of value";
        } else {
            root = value->get_root();
        }
    }

    bool contains_member(std::string &member) {
        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        got_superblock_t nested_superblock;
        nested_superblock.sb.swap(nested_btree_sb);

        btree_key_buffer_t nested_key(member);
        keyvalue_location_t<redis_nested_set_value_t> loc;
        find_keyvalue_location_for_read(txn.get(), &nested_superblock, nested_key.key(), &loc);
        return (loc.value.get() != NULL);
    }

    boost::shared_ptr<one_way_iterator_t<std::string> > iterator() {
        boost::shared_ptr<value_sizer_t<redis_nested_set_value_t> >
            sizer_ptr(new value_sizer_t<redis_nested_set_value_t>(sizer.block_size()));

        // We nest a slice_keys iterator inside a transform iterator
        store_key_t none_key;
        none_key.size = 0;

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));

        slice_keys_iterator_t<redis_nested_set_value_t> *tree_iter =
            new slice_keys_iterator_t<redis_nested_set_value_t>(sizer_ptr, txn.get(),
                nested_btree_sb, btree->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);

        boost::shared_ptr<one_way_iterator_t<std::string > > transform_iter(
                new transform_iterator_t<key_value_pair_t<redis_nested_set_value_t>, std::string>(
                boost::bind(&transform_value, _1), tree_iter));

        return transform_iter;
    }

    redis_set_value_t *value;
    block_id_t root;

private:
    btree_slice_t *btree;

    static std::string transform_value(const key_value_pair_t<redis_nested_set_value_t> &pair) {
        return pair.key;
    }
};

// Set operations

//WRITE(sadd)
redis_protocol_t::indicated_key_t redis_protocol_t::sadd::get_keys() {
    return one[0];
}

SHARD_W(sadd)
EXECUTE_W(sadd) {
    int count = 0;
    set_set_oper_t oper(one[0], btree, timestamp, otok);
    for(std::vector<std::string>::iterator iter = one.begin() + 1; iter != one.end(); ++iter) {
        if(oper.add_member(*iter)) {
            count++;
        }
    }

    oper.increment_size(count);
    return int_response(count);
}

//READ(scard)
KEYS(scard)
SHARD_R(scard)

EXECUTE_R(scard) {
    set_read_oper_t oper(one, btree, otok);
    redis_set_value_t *value = reinterpret_cast<redis_set_value_t *>(oper.location.value.get());
    if(value == NULL) return int_response(0);
    return int_response(value->get_sub_size());
}

//READ(sdiff)
redis_protocol_t::indicated_key_t redis_protocol_t::sdiff::get_keys() {
    return one[0];
}

SHARD_R(sdiff)

EXECUTE_R(sdiff) {
    merge_ordered_data_iterator_t<std::string> *right =
        new merge_ordered_data_iterator_t<std::string>();

    // Add each set iterator to the merged data iterator
    for(std::vector<std::string>::iterator iter = one.begin() + 1; iter != one.end(); ++iter) {
        set_read_oper_t oper(*iter, btree, otok);
        boost::shared_ptr<one_way_iterator_t<std::string> > set_iter = oper.iterator();
        right->add_mergee(set_iter);
    }

    set_read_oper_t oper(one[0], btree, otok);

    // Why put this into a merged data iterator first? Because the memory for the iterator returned
    // by oper.iterator is managed by a shared ptr it doesn't play well with the diff_filter_iterator
    boost::shared_ptr<one_way_iterator_t<std::string> > left_iter = oper.iterator();
    merge_ordered_data_iterator_t<std::string> *left =
        new merge_ordered_data_iterator_t<std::string>();
    left->add_mergee(left_iter);
    
    diff_filter_iterator_t<std::string> filter(left, right);

    std::vector<std::string> result;
    while(true) {
        boost::optional<std::string> next = filter.next();
        if(!next) break;

        result.push_back(next.get());
    }

    return read_response_t(new multi_bulk_result_t(result));
}

//READ(sinter)
redis_protocol_t::indicated_key_t redis_protocol_t::sinter::get_keys() {
    return one[0];
}

SHARD_R(sinter)

EXECUTE_R(sinter) {
    merge_ordered_data_iterator_t<std::string> *merged =
        new merge_ordered_data_iterator_t<std::string>();

    // Add each set iterator to the merged data iterator
    std::list<set_read_oper_t *> opers;
    for(std::vector<std::string>::iterator iter = one.begin(); iter != one.end(); ++iter) {
        set_read_oper_t *oper = new set_read_oper_t(*iter, btree, otok);
        opers.push_back(oper);
        boost::shared_ptr<one_way_iterator_t<std::string> > set_iter = oper->iterator();
        merged->add_mergee(set_iter);
    }
    
    // Use the repetition filter to get only results common to all keys
    repetition_filter_iterator_t<std::string> filter(merged, one.size());

    std::vector<std::string> result;
    while(true) {
        boost::optional<std::string> next = filter.next();
        if(!next) break;

        result.push_back(next.get());
    }

    // Delete opers
    for(std::list<set_read_oper_t *>::iterator iter = opers.begin(); iter != opers.end(); ++iter) {
        delete *iter;
    }

    return read_response_t(new multi_bulk_result_t(result));
}

//READ(sismember)
KEYS(sismember)
SHARD_R(sismember)

EXECUTE_R(sismember) {
    set_read_oper_t oper(one, btree, otok);
    return int_response(oper.contains_member(two));
}

//READ(smembers)
KEYS(smembers)
SHARD_R(smembers)

EXECUTE_R(smembers) {
    set_read_oper_t oper(one, btree, otok);

    std::vector<std::string> result;
    boost::shared_ptr<one_way_iterator_t<std::string> > iter = oper.iterator();
    while(true) {
        boost::optional<std::string> mem = iter->next();
        if(mem) {
            std::string mem_str = mem.get();
            result.push_back(mem_str);
        } else break;
    }

    return read_response_t(new multi_bulk_result_t(result));
}

WRITE(spop)
READ(srandmember)

//WRITE(srem)
redis_protocol_t::indicated_key_t redis_protocol_t::srem::get_keys() {
    return one[0];
}

SHARD_W(srem)

EXECUTE_W(srem) {
    set_set_oper_t oper(one[0], btree, timestamp, otok);

    int count = 0;
    for(std::vector<std::string>::iterator iter = one.begin() + 1; iter != one.end(); ++iter) {
        if(oper.remove_member(*iter))
            ++count;
    }

    oper.increment_size(-count);
    return int_response(count);
}

//READ(sunion)
redis_protocol_t::indicated_key_t redis_protocol_t::sunion::get_keys() {
    return one[0];
}

SHARD_R(sunion)

EXECUTE_R(sunion) {
    merge_ordered_data_iterator_t<std::string> *merged =
        new merge_ordered_data_iterator_t<std::string>();

    // Add each set iterator to the merged data iterator
    std::list<set_read_oper_t *> opers;
    for(std::vector<std::string>::iterator iter = one.begin(); iter != one.end(); ++iter) {
        set_read_oper_t *oper = new set_read_oper_t(*iter, btree, otok);
        boost::shared_ptr<one_way_iterator_t<std::string> > set_iter = oper->iterator();
        merged->add_mergee(set_iter);
    }
    
    // Use the unique filter to eliminate duplicates
    unique_filter_iterator_t<std::string> filter(merged);

    std::vector<std::string> result;
    while(true) {
        boost::optional<std::string> next = filter.next();
        if(!next) break;

        result.push_back(next.get());
    }

    // Delete opers
    for(std::list<set_read_oper_t *>::iterator iter = opers.begin(); iter != opers.end(); ++iter) {
        delete *iter;
    }

    return read_response_t(new multi_bulk_result_t(result));
}

WRITE(sunionstore)
