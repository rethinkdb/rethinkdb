#include "redis/redis.hpp"
#include "redis/redis_util.hpp"
#include "redis/counted/counted.hpp"
#include <boost/lexical_cast.hpp>
#include <deque>
#include <float.h>

struct sorted_set_set_oper_t : set_oper_t {
    sorted_set_set_oper_t(std::string &key, btree_slice_t *btree, timestamp_t timestamp, order_token_t otok) :
        set_oper_t(key, btree, timestamp, otok),
        score_index(btree->cache()->get_block_size())
    {
        value = reinterpret_cast<redis_sorted_set_value_t *>(location.value.get());

        if(value == NULL) {
            // Allocate sorted set
            scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
            location.value.swap(smrsv);
            location.value->set_redis_type(REDIS_SORTED_SET);
            value = reinterpret_cast<redis_sorted_set_value_t *>(location.value.get());

            value->get_sub_size() = 0;
            value->get_member_index_root() = NULL_BLOCK_ID;
            value->get_score_index_root() = NULL_BLOCK_ID;

        } else if(value->get_redis_type() != REDIS_SORTED_SET) {
            throw "ERR Operation against key holding the wrong kind of value";
        }

        member_index_root = value->get_member_index_root();
        counted_ref.count = value->get_sub_size();
        counted_ref.node_id = value->get_score_index_root();
        score_index = counted_btree_t(&counted_ref, btree->cache()->get_block_size(), txn.get());
    }

    ~sorted_set_set_oper_t() {
        // Copy any changes to the root block of the indicies to the value
        value->get_member_index_root() = member_index_root;
        value->get_score_index_root() = counted_ref.node_id;
        value->get_sub_size() = counted_ref.count;
    }

    // Returns whether or not the member was actually added
    bool add_or_update(std::string &member, float score) {
        bool new_value_added;

        find_member mem(txn.get(), this, member);
        if(mem.loc.value.get() == NULL) {
            // This memeber doesn't exist. Add it.
            mem.create(txn.get(), member, score);
            mem.apply_change(txn.get());

            new_value_added = true;
        } else {
            // Update the score for this member. Remove from score index.
            redis_nested_sorted_set_value_t *value = mem.loc.value.get();
            float old_score = value->score;
            value->score = score;

            remove_from_score_index(member, old_score);

            new_value_added = false;
        }

        // Add to score index
        score_index.insert_score(score, member);

        return new_value_added;
    }

    bool remove(std::string &member) {
        // Look up member in member index to get it's score (and remove it)
        find_member mem(txn.get(), this, member);

        if (mem.loc.value.get()) {
            float score = mem.loc.value->score;

            // Delete it
            scoped_malloc<redis_nested_sorted_set_value_t> null;
            mem.loc.value.swap(null);

            remove_from_score_index(member, score);

            return true;
        } else {
            return false;
        }
    }

    int remove_index_range(int start, int end) {
        unsigned u_start = convert_index(start);
        unsigned u_end = convert_index(end);

        int removed = 0;
        // TODO(wmrowan): Fix this for loop.
        for(unsigned i = u_end; i >= u_start; i--) {
            // Find the member name to remove from member index
            const counted_value_t *val = score_index.at(i);
            std::string member;
            blob_t blob(const_cast<char *>(val->blb), blob::btree_maxreflen);
            blob.read_to_string(member, txn.get(), 0, blob.valuesize());

            // Remove from indicies
            score_index.remove(i);

            find_member mem(txn.get(), this, member);
            rassert(mem.loc.value.get());
            scoped_malloc<redis_nested_sorted_set_value_t> null;
            mem.loc.value.swap(null);

            removed++;
        }

        return removed;
    }

    int remove_score_range(float score_min, float score_max) {
        int removed = 0;
        int index = 0;
        for(counted_btree_t::iterator_t iter = score_index.score_iterator(score_min, score_max); iter.is_valid(); iter.next()) {
            if(removed == 0) index = iter.rank();

            std::string member = iter.member();
            find_member mem(txn.get(), this, member);
            rassert(mem.loc.value.get());
            scoped_malloc<redis_nested_sorted_set_value_t> null;
            mem.loc.value.swap(null);
            
            // We can't remove from score index while the iterator is valid, take care of that later

            removed++;
        }

        // Remove from score index
        for(int i = 0; i < removed; i++) {
            // As we remove elements the next one will shift to index
            score_index.remove(index);
        }

        return removed;

    }

    float increment_score(std::string &member, float by) {
        find_member mem(txn.get(), this, member);
        float old_score = 0;
        float new_score = by;
        if(mem.loc.value.get()) {
            old_score = mem.loc.value->score;
            new_score += old_score;
            mem.loc.value->score = new_score;

            remove_from_score_index(member, old_score);
        } else {
            mem.create(txn.get(), member, new_score);
        }

        score_index.insert_score(new_score, member);

        return new_score;
    }

private:
    struct find_member {
        find_member(transaction_t *txn, sorted_set_set_oper_t *ths_ptr, std::string &member):
            ths(ths_ptr),
            nested_key(member)
        {
            boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(ths->member_index_root));
            got_superblock_t nested_superblock;
            nested_superblock.sb.swap(nested_btree_sb);

            int fake_eviction_priority = FAKE_EVICTION_PRIORITY;

            find_keyvalue_location_for_write(txn, &nested_superblock, nested_key.key(), &loc, &fake_eviction_priority);
        }

        void create(transaction_t *txn, std::string &member, float score) {
            scoped_malloc<redis_nested_sorted_set_value_t>
                    smrsv(blob::btree_maxreflen + sizeof(redis_nested_sorted_set_value_t));
            loc.value.swap(smrsv);

            redis_nested_sorted_set_value_t *value = loc.value.get();
            value->score = score;
            blob_t b(value->content, blob::btree_maxreflen);
            b.append_region(txn, member.size());
            b.write_from_string(member, txn, 0);
        }

        void apply_change(transaction_t *txn) {
            // TODO hook up timestamp once Tim figures out what to do with the timestamp
            fake_key_modification_callback_t<redis_nested_sorted_set_value_t> fake_cb;

            int fake_eviction_priority = FAKE_EVICTION_PRIORITY;

            apply_keyvalue_change(txn, &loc, nested_key.key(), repli_timestamp_t::invalid /*ths->timestamp*/, &fake_cb, &fake_eviction_priority);
            virtual_superblock_t *sb = reinterpret_cast<virtual_superblock_t *>(loc.sb.get());
            ths->member_index_root = sb->get_root_block_id();
        }

        sorted_set_set_oper_t *ths;
        btree_key_buffer_t nested_key;
        keyvalue_location_t<redis_nested_sorted_set_value_t> loc;
    };

    void remove_from_score_index(std::string &member, float score) {
        // We can only look up a value in the score index by score or index. We have the score but because
        // scores are not unique we have to use a counted_btree iterator over this score to find the correct one

        int index_to_remove = -1;
        for(counted_btree_t::iterator_t iter = score_index.score_iterator(score, score); iter.is_valid(); iter.next()) {
            std::string to_test = iter.member();
            if(to_test == member) {
                // We've found 'er!
                index_to_remove = iter.rank();
                break;
            }
        }

        rassert(index_to_remove >= 0);
        score_index.remove(index_to_remove);
    }

    unsigned convert_index(int index) {
        int size = counted_ref.count;
        if(index < 0) {
            index += size;
        }

        if(index < 0) {
            index = 0;
        } else if(index >= size) {
            index = size - 1;
        }
        
        return index;
    }

    redis_sorted_set_value_t *value;
    block_id_t member_index_root;
    sub_ref_t counted_ref;
    counted_btree_t score_index;
};

struct sorted_set_read_oper_t : read_oper_t {
    sorted_set_read_oper_t(std::string &key, btree_slice_t *btree, order_token_t otok) :
        read_oper_t(key, btree, otok), 
        rank_index(btree->cache()->get_block_size())
    {
        redis_sorted_set_value_t *value = reinterpret_cast<redis_sorted_set_value_t *>(location.value.get());
        if(value == NULL) {
            throw "ERR Key doesn't exist";
        } else if(value->get_redis_type() != REDIS_SORTED_SET) {
            throw "ERR Operation against key holding the wrong kind of value";
        }

        member_index_root = value->get_member_index_root();
        score_index_root = value->get_score_index_root();
        counted_ref.count = value->get_sub_size();
        counted_ref.node_id = value->get_score_index_root();
        rank_index = counted_btree_t(&counted_ref, btree->cache()->get_block_size(), txn.get());
    }

    int get_size() {
        return counted_ref.count;
    }

    const counted_value_t *at_index(unsigned index) {
        return rank_index.at(index);
    }

    int count(float score_min, float score_max) {
        int count = 0;
        for(counted_btree_t::iterator_t iter = rank_index.score_iterator(score_min, score_max); iter.is_valid(); iter.next()) {
            count++;
        }
        return count;
    }

    bool score(std::string &member, float *score_out) {
        find_member mem(this, member);
        if(mem.loc.value.get()) {
            *score_out = mem.loc.value->score;
            return true;
        }

        return false;
    }

    counted_btree_t::iterator_t get_iterator(float start, float stop) {
        return rank_index.score_iterator(start, stop);
    }

    int rank(std::string &member) {
        find_member mem(this, member);
        float score = mem.loc.value->score;
        for(counted_btree_t::iterator_t iter = rank_index.score_iterator(score, score); iter.is_valid(); iter.next()) {
            if(iter.member() == member) {
                return iter.rank();
            }
        }

        // Not found
        return -1;
    }

    unsigned convert_index(int index) {
        int size = counted_ref.count;
        if(index < 0) {
            index += size;
        }

        if(index < 0) {
            index = 0;
        } else if(index >= size) {
            index = size - 1;
        }
        
        return index;
    }

private:
    struct find_member {
        find_member(sorted_set_read_oper_t *ths_ptr, std::string &member):
            ths(ths_ptr),
            nested_key(member)
        {
            boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(ths->member_index_root));
            got_superblock_t nested_superblock;
            nested_superblock.sb.swap(nested_btree_sb);

            int eviction_priority = FAKE_EVICTION_PRIORITY;

            find_keyvalue_location_for_read(ths->txn.get(), &nested_superblock, nested_key.key(), &loc, eviction_priority);
        }

        sorted_set_read_oper_t *ths;
        btree_key_buffer_t nested_key;
        keyvalue_location_t<redis_nested_sorted_set_value_t> loc;
    };

    block_id_t member_index_root;
    block_id_t score_index_root;
    sub_ref_t counted_ref;
    counted_btree_t rank_index;
};

//WRITE(zadd)
redis_protocol_t::indicated_key_t redis_protocol_t::zadd::get_keys() {
    return one[0];
}

SHARD_W(zadd)
EXECUTE_W(zadd) {
    sorted_set_set_oper_t oper(one[0], btree, timestamp, otok);
    unsigned added = 0;
    
    std::vector<std::string>::iterator iter = one.begin() + 1;
    while(iter != one.end()) {
        std::string score_str = *iter;
        iter++;

        if(iter == one.end())
            throw "ERR Protocol Error";
        std::string member = *iter;
        iter++;

        float score;
        try {
            score = boost::lexical_cast<float>(score_str);
        } catch(boost::bad_lexical_cast &) {
            throw "ERR Protocol Error";
        }

        if(oper.add_or_update(member, score)) {
            added++;
        }
    }

    return int_response(added);
}

//READ(zcard)
KEYS(zcard)
SHARD_R(zcard)

EXECUTE_R(zcard) {
    sorted_set_read_oper_t oper(one, btree, otok);
    return int_response(oper.get_size());
}

//READ(zcount)
KEYS(zcount)
SHARD_R(zcount)

EXECUTE_R(zcount) {
    sorted_set_read_oper_t oper(one, btree, otok);
    return int_response(oper.count(two, three));
}

//WRITE(zincrby)
KEYS(zincrby)
SHARD_W(zincrby)

EXECUTE_W(zincrby) {
    sorted_set_set_oper_t oper(one, btree, timestamp, otok);
    float new_value = oper.increment_score(three, two);
    return bulk_response(new_value);
}

WRITE(zinterstore)

//READ(zrange)
redis_protocol_t::indicated_key_t redis_protocol_t::zrange::get_keys() {
    return key;
}

SHARD_R(zrange)

EXECUTE_R(zrange) {
    sorted_set_read_oper_t oper(key, btree, otok);
    std::deque<std::string> result;
    start = oper.convert_index(start);
    stop = oper.convert_index(stop);
    for(int i = start; i <= stop; i++) {
        const counted_value_t *val = oper.at_index(i);
        std::string member;
        blob_t b(const_cast<char *>(val->blb), blob::btree_maxreflen);
        b.read_to_string(member, oper.txn.get(), 0, b.valuesize());

        float f_score = val->score;
        std::string str_score;
        try {
            str_score = boost::lexical_cast<std::string>(f_score);
        } catch(boost::bad_lexical_cast &) {
            unreachable();
        }

        if(with_scores && reverse) {
            result.push_front(str_score);
            result.push_front(member);
        } else if(with_scores) {
            result.push_back(member);
            result.push_back(str_score);
        } else if(reverse) {
            result.push_front(member);
        } else {
            result.push_back(member);
        }
    }

    return redis_protocol_t::read_response_t(new redis_protocol_t::multi_bulk_result_t(result));
}

//READ(zrangebyscore)
redis_protocol_t::indicated_key_t redis_protocol_t::zrangebyscore::get_keys() {
    return key;
}

SHARD_R(zrangebyscore)

float parse_limit(std::string &str, bool *open) {
    if(str == std::string("+inf")) {
        return FLT_MAX;
    } else if(str == std::string("-inf")) {
        return FLT_MIN;
    } else if(str[0] == '(') {
        *open = true;
        str = std::string(str.begin() + 1, str.end());
    }

    try {
        return boost::lexical_cast<float>(str);
    } catch(boost::bad_lexical_cast &) {
        throw "ERR Protocol Error";
    }
}

EXECUTE_R(zrangebyscore) {
    // Parse our limits
    bool min_open = false;
    float min_val = parse_limit(min, &min_open);

    bool max_open = false;
    float max_val = parse_limit(max, &max_open);

    // Do the lookup
    sorted_set_read_oper_t oper(key, btree, otok);

    std::deque<std::string> result;
    for(counted_btree_t::iterator_t iter = oper.get_iterator(min_val, max_val); iter.is_valid(); iter.next()) {
        // Handle limits
        if(limit) {
            if(offset > 0) {
                // We still need to skip until we can start recording results
                offset--;
                continue;
            }
            if(count > 0) {
                count--;
            } else {
                // We've already taken as many results as we're supposed to
                break;
            }
        }

        // Handle open ends
        if(min_open && iter.score() == min_val) {
            continue;
        }
        if(max_open && iter.score() == max_val) {
            break;
        }

        std::string member = iter.member();
        float f_score = iter.score();
        std::string str_score;
        try {
            str_score = boost::lexical_cast<std::string>(f_score);
        } catch(boost::bad_lexical_cast &) {
            unreachable();
        }

        if(with_scores && reverse) {
            result.push_front(str_score);
            result.push_front(member);
        } else if(with_scores) {
            result.push_back(member);
            result.push_back(str_score);
        } else if(reverse) {
            result.push_front(member);
        } else {
            result.push_back(member);
        }
    }

    return redis_protocol_t::read_response_t(new redis_protocol_t::multi_bulk_result_t(result));
}

//READ(zrank)
redis_protocol_t::indicated_key_t redis_protocol_t::zrank::get_keys() {
    return key;
}

SHARD_R(zrank)

EXECUTE_R(zrank) {
    sorted_set_read_oper_t oper(key, btree, otok);
    int rank = oper.rank(member);
    if(rank >= 0) {
        if(reverse) {
            return int_response(oper.get_size() - rank);
        } else {
            return int_response(rank);
        }
    }

    return redis_protocol_t::read_response_t();
}

//WRITE(zrem)
redis_protocol_t::indicated_key_t redis_protocol_t::zrem::get_keys() {
    return one[0];
}

SHARD_W(zrem)

EXECUTE_W(zrem) {
    int num_removed = 0;

    sorted_set_set_oper_t oper(one[0], btree, timestamp, otok);
    for(std::vector<std::string>::iterator iter = one.begin() + 1; iter != one.end(); iter++) {
        if(oper.remove(*iter)) num_removed++;
    }

    return int_response(num_removed);
}

//WRITE(zremrangebyrank)
KEYS(zremrangebyrank)
SHARD_W(zremrangebyrank)

EXECUTE_W(zremrangebyrank) {
    sorted_set_set_oper_t oper(one, btree, timestamp, otok);
    int removed = oper.remove_index_range(two, three);
    return int_response(removed);
}

//WRITE(zremrangebyscore)
KEYS(zremrangebyscore)
SHARD_W(zremrangebyscore)

EXECUTE_W(zremrangebyscore) {
    sorted_set_set_oper_t oper(one, btree, timestamp, otok);
    int removed = oper.remove_score_range(two, three);
    return int_response(removed);
}

//READ(zscore)
KEYS(zscore)
SHARD_R(zscore)

EXECUTE_R(zscore) {
    sorted_set_read_oper_t oper(one, btree, otok);
    float score = 0.0;
    if(oper.score(two, &score)) {
        return bulk_response(score);
    }

    return redis_protocol_t::read_response_t();
}

WRITE(zunionstore)
