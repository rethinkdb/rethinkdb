#include "server/nested_demo/redis_sortedset_values.hpp"
#include "redis_sortedset_values.hpp"
#include "redis_utils.hpp"

#include <boost/bind.hpp>

/* Implementations...*/

int redis_demo_sortedset_value_t::zcard() const {
    return static_cast<int>(size);
}

boost::optional<float> redis_demo_sortedset_value_t::zscore(value_sizer_t<redis_demo_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &member) const {
    // Find the element
    keyvalue_location_t<redis_nested_float_value_t> kv_location;
    redis_utils::find_nested_keyvalue_location_for_read(super_sizer->block_size(), transaction, member, &kv_location, member_score_nested_root);

    // Get out the score
    if (kv_location.there_originally_was_value) {
        return boost::optional<float>(kv_location.value->value);
    } else {
        return boost::none;
    }
}

// TODO! This has O(n) runtime right now! We need a third index to fix this (also see zrange)
// (that maps from internal node of the score_member tree to the number of keys in the corresponding subtree)
boost::optional<int> redis_demo_sortedset_value_t::zrank(value_sizer_t<redis_demo_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread, const std::string &member) const {
    // For now, we just do an O(n) search to skip the first start elements.
    boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > full_iter = get_full_iter(super_sizer, transaction, slice_home_thread);

    // Count up until we arrive at element
    int rank = 0;
    while (true) {
        boost::optional<std::pair<float, std::string> > next = full_iter->next();
        if (!next) {
            return boost::none;
        } else if (next->second == member) {
            return boost::optional<int>(rank);
        }
        ++rank;
    }
}

bool redis_demo_sortedset_value_t::zrem(value_sizer_t<redis_demo_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &member) {
    // First check member_score_nested_root, to get the score (and delete the entry from that tree)
    float score = 0.0f;
    bool member_score_had_key = false;
    {
        value_sizer_t<redis_nested_float_value_t> sizer(super_sizer->block_size());

        // Find the element
        // TODO! Rather use a proper timestamp
        keyvalue_location_t<redis_nested_float_value_t> kv_location;
        scoped_malloc<btree_key_t> btree_key;
        redis_utils::find_nested_keyvalue_location_for_write(super_sizer->block_size(), transaction, member, repli_timestamp_t::invalid, &kv_location, &btree_key, member_score_nested_root);

        // Get score
        if (kv_location.there_originally_was_value) {
            score = kv_location.value->value;
        }

        // Delete the key
        kv_location.value.reset(); // Remove value
        // TODO! Rather use a proper timestamp
        apply_keyvalue_change(&sizer, &kv_location, btree_key.get(), repli_timestamp_t::invalid);

        // Update the nested root id (in case the root got removed)
        member_score_nested_root = kv_location.sb->get_root_block_id();

        member_score_had_key = kv_location.there_originally_was_value;
    }

    if (!member_score_had_key) {
        // Member's not there...
        return false;
    }

    // We got the score (and already deleted it), now go on and delete from the score_member tree...
    bool score_member_had_key = false;
    {
        value_sizer_t<redis_nested_empty_value_t> sizer(super_sizer->block_size());

        const std::string score_member_key = make_score_member_key(score, member);

        // Find the element
        // TODO! Rather use a proper timestamp
        keyvalue_location_t<redis_nested_empty_value_t> kv_location;
        scoped_malloc<btree_key_t> btree_key;
        redis_utils::find_nested_keyvalue_location_for_write(super_sizer->block_size(), transaction, score_member_key, repli_timestamp_t::invalid, &kv_location, &btree_key, score_member_nested_root);

        // Delete the key
        kv_location.value.reset(); // Remove value
        // TODO! Rather use a proper timestamp
        apply_keyvalue_change(&sizer, &kv_location, btree_key.get(), repli_timestamp_t::invalid);

        // Update the nested root id (in case the root got removed)
        score_member_nested_root = kv_location.sb->get_root_block_id();

        score_member_had_key = kv_location.there_originally_was_value;
    }

    rassert(score_member_had_key, "The member \"%s\" (score: %f) was in the member_score tree, but not in the score_member tree.", member.c_str(), score);
    rassert(size > 0);
    --size;
    return true;
}

bool redis_demo_sortedset_value_t::zadd(value_sizer_t<redis_demo_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &member, float score) {
    // Just to be sure: We first delete any existing member of that name, so we
    // don't have to care about cases like differing scores between existing and new entry
    bool had_key = zrem(super_sizer, transaction, member);

    // First insert into the score_member_tree
    {
        value_sizer_t<redis_nested_empty_value_t> sizer(super_sizer->block_size());

        const std::string score_member_key = make_score_member_key(score, member);

        // Construct the value
        scoped_malloc<redis_nested_empty_value_t> btree_value(sizer.max_possible_size());

        // Find the element
        // TODO! Rather use a proper timestamp
        keyvalue_location_t<redis_nested_empty_value_t> kv_location;
        scoped_malloc<btree_key_t> btree_key;
        redis_utils::find_nested_keyvalue_location_for_write(super_sizer->block_size(), transaction, score_member_key, repli_timestamp_t::invalid, &kv_location, &btree_key, score_member_nested_root);

        // Update/insert the value
        kv_location.value.swap(btree_value);
        // TODO! Rather use a proper timestamp
        apply_keyvalue_change(&sizer, &kv_location, btree_key.get(), repli_timestamp_t::invalid);

        // Update the nested root id (in case this was the first member and a root got added)
        score_member_nested_root = kv_location.sb->get_root_block_id();
    }

    // Then insert into member_score_tree
    {
        value_sizer_t<redis_nested_float_value_t> sizer(super_sizer->block_size());

        // Construct the value
        scoped_malloc<redis_nested_float_value_t> btree_value(sizer.max_possible_size());
        btree_value->value = score;

        // Find the element
        // TODO! Rather use a proper timestamp
        keyvalue_location_t<redis_nested_float_value_t> kv_location;
        scoped_malloc<btree_key_t> btree_key;
        redis_utils::find_nested_keyvalue_location_for_write(super_sizer->block_size(), transaction, member, repli_timestamp_t::invalid, &kv_location, &btree_key, member_score_nested_root);

        // Update/insert the value
        kv_location.value.swap(btree_value);
        // TODO! Rather use a proper timestamp
        apply_keyvalue_change(&sizer, &kv_location, btree_key.get(), repli_timestamp_t::invalid);

        // Update the nested root id (in case this was the first member and a root got added)
        member_score_nested_root = kv_location.sb->get_root_block_id();
    }

    ++size; // We can do this regardless of had_key, because we removed existing entries at the start
    return !had_key;
}

void redis_demo_sortedset_value_t::clear(value_sizer_t<redis_demo_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread) {
    boost::shared_ptr<value_sizer_t<redis_nested_empty_value_t> > sizer_ptr(new value_sizer_t<redis_nested_empty_value_t>(super_sizer->block_size()));

    std::vector<std::string> keys;
    keys.reserve(size);

    // Make an iterator and collect all keys in the nested tree
    store_key_t none_key;
    none_key.size = 0;
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(member_score_nested_root));
    {
        slice_keys_iterator_t<redis_nested_empty_value_t> tree_iter(sizer_ptr, transaction, nested_btree_sb, slice_home_thread, rget_bound_none, none_key, rget_bound_none, none_key);
        while (true) {
            boost::optional<key_value_pair_t<redis_nested_empty_value_t> > next = tree_iter.next();
            if (next) {
                keys.push_back(next.get().key);
            } else {
                break;
            }
        }
    }

    // Delete all keys
    nested_btree_sb.reset(new virtual_superblock_t(member_score_nested_root)); // TODO! Wait, what's this line there for? For releasing locks or something?
    for (size_t i = 0; i < keys.size(); ++i) {
        zrem(super_sizer, transaction, keys[i]);
    }

    // All subtree blocks should have been deleted
    rassert(size == 0);

    // Now delete the root if we still have one
    if (member_score_nested_root != NULL_BLOCK_ID) {
        buf_lock_t root_buf(transaction.get(), member_score_nested_root, rwi_write);
        root_buf.buf()->mark_deleted();
        member_score_nested_root = NULL_BLOCK_ID;
    }
    rassert(member_score_nested_root == NULL_BLOCK_ID);
    if (score_member_nested_root != NULL_BLOCK_ID) {
        buf_lock_t root_buf(transaction.get(), score_member_nested_root, rwi_write);
        root_buf.buf()->mark_deleted();
        score_member_nested_root = NULL_BLOCK_ID;
    }
    rassert(score_member_nested_root == NULL_BLOCK_ID);
}

boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > redis_demo_sortedset_value_t::get_full_iter(value_sizer_t<redis_demo_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread) const {
    boost::shared_ptr<value_sizer_t<redis_nested_empty_value_t> > sizer_ptr(new value_sizer_t<redis_nested_empty_value_t>(super_sizer->block_size()));

    // We nest a slice_keys iterator inside a transform iterator
    store_key_t none_key;
    none_key.size = 0;
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(score_member_nested_root));
    slice_keys_iterator_t<redis_nested_empty_value_t> *tree_iter =
            new slice_keys_iterator_t<redis_nested_empty_value_t>(sizer_ptr, transaction, nested_btree_sb, slice_home_thread, rget_bound_none, none_key, rget_bound_none, none_key);
    boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > transform_iter(
            new transform_iterator_t<key_value_pair_t<redis_nested_empty_value_t>, std::pair<float, std::string> >(
                    boost::bind(&redis_demo_sortedset_value_t::transform_value, this, _1), tree_iter));

    return transform_iter;
}

// TODO! Be cautious, this right now is O(start) in runtime complexity
boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > redis_demo_sortedset_value_t::zrange(value_sizer_t<redis_demo_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread, int start) const {

    // TODO! We require the third nested tree for this to locate the element with index start.
    // It's a bit complicated, and we probably can't use the existing operations btree interface.
    // Implement all of this stuff.

    // For now, we just do an O(n) search to skip the first start elements.
    boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > full_iter = get_full_iter(super_sizer, transaction, slice_home_thread);

    // If start is negative, we start with the element of index size + start
    if (start < 0) {
        start += size;
    }

    // Skip the first start element
    for (int i = 0; i < start; ++i) {
        boost::optional<std::pair<float, std::string> > next = full_iter->next();
        if (!next) {
            break;
        }
    }

    return full_iter;
}

boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > redis_demo_sortedset_value_t::zrangebyscore(value_sizer_t<redis_demo_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread, rget_bound_mode_t min_mode, float min, rget_bound_mode_t max_mode, float max) const {
    boost::shared_ptr<value_sizer_t<redis_nested_empty_value_t> > sizer_ptr(new value_sizer_t<redis_nested_empty_value_t>(super_sizer->block_size()));

    // Construct btree prefix keys
    store_key_t min_key;
    char min_key_buf[redis_utils::LEX_FLOAT_SIZE];
    redis_utils::to_lex_float(min, min_key_buf);
    min_key.assign(redis_utils::LEX_FLOAT_SIZE, min_key_buf);
    store_key_t max_key;
    char max_key_buf[redis_utils::LEX_FLOAT_SIZE];
    redis_utils::to_lex_float(max, max_key_buf);
    max_key.assign(redis_utils::LEX_FLOAT_SIZE, max_key_buf);

    // We nest a slice_keys iterator inside a transform iterator
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(score_member_nested_root));
    slice_keys_iterator_t<redis_nested_empty_value_t> *tree_iter =
            new slice_keys_iterator_t<redis_nested_empty_value_t>(sizer_ptr, transaction, nested_btree_sb, slice_home_thread, min_mode, min_key, max_mode, max_key);
    boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > transform_iter(
            new transform_iterator_t<key_value_pair_t<redis_nested_empty_value_t>, std::pair<float, std::string> >(
                    boost::bind(&redis_demo_sortedset_value_t::transform_value, this, _1), tree_iter));

    return transform_iter;
}
