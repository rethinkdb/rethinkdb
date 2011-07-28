#include "server/nested_demo/redis_hash_values.hpp"
#include "redis_utils.hpp"

#include <boost/bind.hpp>

/* Implementations...*/
boost::optional<std::string> redis_hash_value_t::hget(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field) const {
    // Find the element
    keyvalue_location_t<redis_nested_string_value_t> kv_location;
    redis_utils::find_nested_keyvalue_location_for_read(super_sizer->block_size(), transaction, field, &kv_location, nested_root);

    // Get out the string value
    if (kv_location.there_originally_was_value) {
        blob_t b(kv_location.value->contents, blob::btree_maxreflen);
        std::string value;
        b.read_to_string(value, transaction.get(), 0, b.valuesize());
        return boost::optional<std::string>(value);
    } else {
        return boost::none;
    }
}

bool redis_hash_value_t::hexists(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field) const {
    // Find the element
    keyvalue_location_t<redis_nested_string_value_t> kv_location;
    redis_utils::find_nested_keyvalue_location_for_read(super_sizer->block_size(), transaction, field, &kv_location, nested_root);

    return kv_location.there_originally_was_value;
}

int redis_hash_value_t::hlen() const {
    return static_cast<int>(size);
}

bool redis_hash_value_t::hdel(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field) {
    value_sizer_t<redis_nested_string_value_t> sizer(super_sizer->block_size());

    // Find the element
    // TODO! Rather use a proper timestamp
    keyvalue_location_t<redis_nested_string_value_t> kv_location;
    scoped_malloc<btree_key_t> btree_key;
    redis_utils::find_nested_keyvalue_location_for_write(super_sizer->block_size(), transaction, field, repli_timestamp_t::invalid, &kv_location, &btree_key, nested_root);

    // Delete the key
    if (kv_location.there_originally_was_value) {
        // Clear the existing blob...
        blob_t b_old(kv_location.value->contents, blob::btree_maxreflen);
        b_old.clear(transaction.get());
    }
    kv_location.value.reset(); // Remove value
    // TODO! Rather use a proper timestamp
    apply_keyvalue_change(&sizer, &kv_location, btree_key.get(), repli_timestamp_t::invalid);

    // Update the nested root id (in case the root got removed)
    nested_root = kv_location.sb->get_root_block_id();

    if (kv_location.there_originally_was_value) {
        rassert(size > 0);
        --size;
    }

    return kv_location.there_originally_was_value;
}

bool redis_hash_value_t::hset(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field, const std::string &value) {
    value_sizer_t<redis_nested_string_value_t> sizer(super_sizer->block_size());

    // Construct the value
    scoped_malloc<redis_nested_string_value_t> btree_value(sizer.max_possible_size());
    memset(btree_value.get(), 0, sizer.max_possible_size());
    blob_t b(btree_value->contents, blob::btree_maxreflen);
    b.append_region(transaction.get(), value.length());
    b.write_from_string(value, transaction.get(), 0);

    // Find the element
    // TODO! Rather use a proper timestamp
    keyvalue_location_t<redis_nested_string_value_t> kv_location;
    scoped_malloc<btree_key_t> btree_key;
    redis_utils::find_nested_keyvalue_location_for_write(super_sizer->block_size(), transaction, field, repli_timestamp_t::invalid, &kv_location, &btree_key, nested_root);

    // Update/insert the value
    if (kv_location.there_originally_was_value) {
        // Clear the existing blob...
        blob_t b_old(kv_location.value->contents, blob::btree_maxreflen);
        b_old.clear(transaction.get());
    }
    kv_location.value.swap(btree_value);
    // TODO! Rather use a proper timestamp
    apply_keyvalue_change(&sizer, &kv_location, btree_key.get(), repli_timestamp_t::invalid);

    // Update the nested root id (in case this was the first field and a root got added)
    nested_root = kv_location.sb->get_root_block_id();

    if (!kv_location.there_originally_was_value) {
        ++size;
    }

    return !kv_location.there_originally_was_value;
}

void redis_hash_value_t::clear(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread) {
    boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> > sizer_ptr(new value_sizer_t<redis_nested_string_value_t>(super_sizer->block_size()));

    std::vector<std::string> keys;
    keys.reserve(size);

    // Make an iterator and collect all keys in the nested tree
    store_key_t none_key;
    none_key.size = 0;
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(nested_root));
    {
        slice_keys_iterator_t<redis_nested_string_value_t> tree_iter(sizer_ptr, transaction, nested_btree_sb, slice_home_thread, rget_bound_none, none_key, rget_bound_none, none_key);
        while (true) {
            boost::optional<key_value_pair_t<redis_nested_string_value_t> > next = tree_iter.next();
            if (next) {
                keys.push_back(next.get().key);
            } else {
                break;
            }
        }
    }

    // Delete all keys
    nested_btree_sb.reset(new virtual_superblock_t(nested_root));
    for (size_t i = 0; i < keys.size(); ++i) {
        hdel(super_sizer, transaction, keys[i]);
    }

    // All subtree blocks should have been deleted
    rassert(size == 0);

    // Now delete the root if we still have one
    if (nested_root != NULL_BLOCK_ID) {
        buf_lock_t root_buf(transaction.get(), nested_root, rwi_write);
        root_buf.buf()->mark_deleted();
        nested_root = NULL_BLOCK_ID;
    }
    rassert(nested_root == NULL_BLOCK_ID);
}

boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > redis_hash_value_t::hgetall(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread) const {
    boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> > sizer_ptr(new value_sizer_t<redis_nested_string_value_t>(super_sizer->block_size()));

    // We nest a slice_keys iterator inside a transform iterator
    store_key_t none_key;
    none_key.size = 0;
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(nested_root));
    slice_keys_iterator_t<redis_nested_string_value_t> *tree_iter =
            new slice_keys_iterator_t<redis_nested_string_value_t>(sizer_ptr, transaction, nested_btree_sb, slice_home_thread, rget_bound_none, none_key, rget_bound_none, none_key);
    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > transform_iter(
            new transform_iterator_t<key_value_pair_t<redis_nested_string_value_t>, std::pair<std::string, std::string> >(
                    boost::bind(&redis_hash_value_t::transform_value, this, transaction, _1), tree_iter));

    return transform_iter;
}
