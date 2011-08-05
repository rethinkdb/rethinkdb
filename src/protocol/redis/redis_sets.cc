#include "protocol/redis/redis_util.hpp"
#include "btree/iteration.hpp"
#include <iostream>

// Set utilities
struct set_set_oper_t : set_oper_t {
    set_set_oper_t(std::string &key, btree_slice_t *btree, int64_t timestamp) :
        set_oper_t(key, btree, timestamp),
        nested_sizer(btree->cache()->get_block_size())
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
            // TODO in this case?
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
            nested_superblock.txn = ths->location.txn;

            find_keyvalue_location_for_write(&ths->nested_sizer, &nested_superblock, nested_key.key(), ths->tstamp, &loc);
        }
        
        void apply_change() {
            apply_keyvalue_change(&ths->nested_sizer, &loc, nested_key.key(), ths->tstamp);
            virtual_superblock_t *sb = reinterpret_cast<virtual_superblock_t *>(loc.sb.get());
            ths->root = sb->get_root_block_id();
        }

        set_set_oper_t *ths;
        btree_key_buffer_t nested_key;
        keyvalue_location_t<redis_nested_set_value_t> loc;
    };

    block_id_t root;
    value_sizer_t<redis_nested_set_value_t> nested_sizer;
};

struct set_read_oper_t : read_oper_t {
    set_read_oper_t(std::string &key, btree_slice_t *btr) :
        read_oper_t(key, btr),
        btree(btr)
    {
        value = reinterpret_cast<redis_set_value_t *>(location.value.get());
        if(!value) {
            root = NULL_BLOCK_ID;
        } else if(value->get_redis_type() != REDIS_SET) {

        } else {
            root = value->get_root();
        }
    }

    bool contains_member(std::string &member) {
        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        got_superblock_t nested_superblock;
        nested_superblock.sb.swap(nested_btree_sb);
        nested_superblock.txn = location.txn;

        value_sizer_t<redis_nested_set_value_t> nested_sizer(sizer.block_size());
        btree_key_buffer_t nested_key(member);
        keyvalue_location_t<redis_nested_set_value_t> loc;
        find_keyvalue_location_for_read(&nested_sizer, &nested_superblock, nested_key.key(), &loc);
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
                new slice_keys_iterator_t<redis_nested_set_value_t>(sizer_ptr, location.txn,
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

//COMMAND_N(integer, sadd)
integer_result redis_actor_t::sadd(std::vector<std::string> &key_mems) {
    int count = 0;
    set_set_oper_t oper(key_mems[0], btree, 1234);
    for(std::vector<std::string>::iterator iter = key_mems.begin() + 1; iter != key_mems.end(); ++iter) {
        if(oper.add_member(*iter)) {
            count++;
        }
    }

    oper.increment_size(count);
    return count;
}

//COMMAND_1(integer, scard, string&)
integer_result redis_actor_t::scard(std::string &key) {
    set_read_oper_t oper(key, btree);
    redis_set_value_t *value = reinterpret_cast<redis_set_value_t *>(oper.location.value.get());
    if(value == NULL) return 0;
    return value->get_sub_size();
}

//COMMAND_N(multi_bulk, sdiff)
multi_bulk_result redis_actor_t::sdiff(std::vector<std::string> &keys) {
    merge_ordered_data_iterator_t<std::string> *right =
        new merge_ordered_data_iterator_t<std::string>();

    // Add each set iterator to the merged data iterator
    for(std::vector<std::string>::iterator iter = keys.begin() + 1; iter != keys.end(); ++iter) {
        set_read_oper_t oper(*iter, btree);
        boost::shared_ptr<one_way_iterator_t<std::string> > set_iter = oper.iterator();
        right->add_mergee(set_iter);
    }

    set_read_oper_t oper(keys[0], btree);

    // Why put this into a merged data iterator first? Because the memory for the iterator returned
    // by oper.iterator is managed by a shared ptr it doesn't play well with the diff_filter_iterator
    boost::shared_ptr<one_way_iterator_t<std::string> > left_iter = oper.iterator();
    merge_ordered_data_iterator_t<std::string> *left =
        new merge_ordered_data_iterator_t<std::string>();
    left->add_mergee(left_iter);
    
    diff_filter_iterator_t<std::string> filter(left, right);

    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>());
    while(true) {
        boost::optional<std::string> next = filter.next();
        if(!next) break;

        result->push_back(next.get());
    }

    return multi_bulk_result(result);

}

COMMAND_N(integer, sdiffstore)

//COMMAND_N(multi_bulk, sinter)
multi_bulk_result redis_actor_t::sinter(std::vector<std::string> &keys) {
    merge_ordered_data_iterator_t<std::string> *merged =
        new merge_ordered_data_iterator_t<std::string>();

    // Add each set iterator to the merged data iterator
    for(std::vector<std::string>::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
        set_read_oper_t oper(*iter, btree);
        boost::shared_ptr<one_way_iterator_t<std::string> > set_iter = oper.iterator();
        merged->add_mergee(set_iter);
    }
    
    // Use the repetition filter to get only results common to all keys
    repetition_filter_iterator_t<std::string> filter(merged, keys.size());

    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>());
    while(true) {
        boost::optional<std::string> next = filter.next();
        if(!next) break;

        result->push_back(next.get());
    }

    return multi_bulk_result(result);
}

COMMAND_N(integer, sinterstore)

//COMMAND_2(integer, sismember, string&, string&)
integer_result redis_actor_t::sismember(std::string &key, std::string &member) {
    set_read_oper_t oper(key, btree);
    return integer_result(oper.contains_member(member));
}

//COMMAND_1(multi_bulk, smembers, string&)
multi_bulk_result redis_actor_t::smembers(std::string &key) {
    set_read_oper_t oper(key, btree);

    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>);
    boost::shared_ptr<one_way_iterator_t<std::string> > iter = oper.iterator();
    while(true) {
        boost::optional<std::string> mem = iter->next();
        if(mem) {
            std::string bob = mem.get();
            result->push_back(bob);
        } else break;
    }

    return multi_bulk_result(result);
}

COMMAND_3(integer, smove, string&, string&, string&)
COMMAND_1(bulk, spop, string&)
COMMAND_1(bulk, srandmember, string&)

//COMMAND_N(integer, srem)
integer_result redis_actor_t::srem(std::vector<std::string> &key_members) {
    set_set_oper_t oper(key_members[0], btree, 1234);

    int count = 0;
    for(std::vector<std::string>::iterator iter = key_members.begin() + 1; iter != key_members.end(); ++iter) {
        if(oper.remove_member(*iter))
            ++count;
    }

    oper.increment_size(-count);
    return integer_result(count);
}

//COMMAND_N(multi_bulk, sunion)
multi_bulk_result redis_actor_t::sunion(std::vector<std::string> &keys) {
    // We can't declare this as a local variable because the reptition filter iterator
    // will try to delete it it it's destructor. It can't delete something that wasn't
    // malloched. What a waste. I hope there is a good reason for this elsewhere.
    merge_ordered_data_iterator_t<std::string> *merged =
        new merge_ordered_data_iterator_t<std::string>();

    // Add each set iterator to the merged data iterator
    for(std::vector<std::string>::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
        set_read_oper_t oper(*iter, btree);
        boost::shared_ptr<one_way_iterator_t<std::string> > set_iter = oper.iterator();
        merged->add_mergee(set_iter);
    }
    
    // Use the unique filter to eliminate duplicates
    unique_filter_iterator_t<std::string> filter(merged);

    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>());
    while(true) {
        boost::optional<std::string> next = filter.next();
        if(!next) break;

        result->push_back(next.get());
    }

    return multi_bulk_result(result);
}

COMMAND_N(integer, sunionstore)
