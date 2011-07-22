#ifndef __SERVER_NESTED_DEMO_REDIS_HASH_VALUES_HPP__
#define	__SERVER_NESTED_DEMO_REDIS_HASH_VALUES_HPP__

// TODO! demo super value (contains block_id_t plus size of hash) with redis interface methods
struct redis_demo_hash_value_t {
    block_id_t nested_root;
    uint32_t size;

public:
    int inline_size(UNUSED block_size_t bs) const {
        return sizeof(nested_root) + sizeof(size);
    }

    int64_t value_size() const {
        return 0;
    }

    const char *value_ref() const { return NULL; }
    char *value_ref() { return NULL; }


    /* Some operations that you can do on a list (resembling redis commands)... */

    boost::optional<std::string> hget(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field) const;

    bool hexists(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field) const;

    int hlen() const;

    // In contrast to the redis command, this deletes only a single field. Call multiple times if necessary.
    // returns true if a field has been deleted (i.e. existed in the hash)
    bool hdel(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field);

    // Returns true iff the field was new
    bool hset(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field, const std::string &value);

    // Deletes all elements from the subtree. Must be called before deleting the hash value in the super tree!
    void clear(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread);

    // Provides an iterator over field->value pairs
    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > hgetall(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread) const;


private:
    // Constructs a btree_key_t from key and puts it into out_buf
    void construct_key(const std::string &key, scoped_malloc<btree_key_t> *out_buf) const {
        scoped_malloc<btree_key_t> tmp(offsetof(btree_key_t, contents) + key.length());
        out_buf->swap(tmp);
        (*out_buf)->size = key.length();
        memcpy((*out_buf)->contents, key.data(), key.length());
    }

    // Transforms from key_value_pair_t<redis_nested_string_value_t> to key_value_pair_t<std::string> for
    // cleaner outside-facing iterator interface
    std::pair<std::string, std::string> transform_value(const key_value_pair_t<redis_nested_string_value_t> &pair) const {
        const redis_nested_string_value_t *value = reinterpret_cast<redis_nested_string_value_t*>(pair.value.get());
        return std::pair<std::string, std::string>(pair.key, std::string(value->contents, value->length));
    }

    // These are shortcuts that operate on the nested btree
    void find_nested_keyvalue_location_for_write(block_size_t block_size, boost::shared_ptr<transaction_t> &transaction, const std::string &field, repli_timestamp_t ts, keyvalue_location_t<redis_nested_string_value_t> *kv_location, scoped_malloc<btree_key_t> *btree_key);
    void find_nested_keyvalue_location_for_read(block_size_t block_size, boost::shared_ptr<transaction_t> &transaction, const std::string &field, keyvalue_location_t<redis_nested_string_value_t> *kv_location) const;
};

template <>
class value_sizer_t<redis_demo_hash_value_t> {
public:
    value_sizer_t<redis_demo_hash_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const redis_demo_hash_value_t *value) const {
        return value->inline_size(block_size_);
    }

    bool fits(UNUSED const redis_demo_hash_value_t *value, UNUSED int length_available) const {
        // It's of constant size...
        return true;
    }

    int max_possible_size() const {
        // It's of constant size...
        return sizeof(redis_demo_hash_value_t);
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 'l', 'e', 'a', 'f' } }; // TODO!
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};


/* TODO! Implementations...*/
boost::optional<std::string> redis_demo_hash_value_t::hget(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field) const {
    // Find the element
    keyvalue_location_t<redis_nested_string_value_t> kv_location;
    find_nested_keyvalue_location_for_read(super_sizer->block_size(), transaction, field, &kv_location);

    // Get out the string value
    if (kv_location.there_originally_was_value) {
        std::string value;
        value.assign(kv_location.value->contents, kv_location.value->length);
        return boost::optional<std::string>(value);
    } else {
        return boost::none;
    }    
}

bool redis_demo_hash_value_t::hexists(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field) const {
    // Find the element
    keyvalue_location_t<redis_nested_string_value_t> kv_location;
    find_nested_keyvalue_location_for_read(super_sizer->block_size(), transaction, field, &kv_location);

    // Swap the transaction back in, we don't need it anymore...
    transaction.swap(kv_location.txn);

    return kv_location.there_originally_was_value;
}

int redis_demo_hash_value_t::hlen() const {
    return static_cast<int>(size);
}

bool redis_demo_hash_value_t::hdel(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field) {
    value_sizer_t<redis_nested_string_value_t> sizer(super_sizer->block_size());

    // Find the element
    // TODO! Rather use a proper timestamp
    keyvalue_location_t<redis_nested_string_value_t> kv_location;
    scoped_malloc<btree_key_t> btree_key;
    find_nested_keyvalue_location_for_write(super_sizer->block_size(), transaction, field, repli_timestamp_t::invalid, &kv_location, &btree_key);

    // Delete the key
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

bool redis_demo_hash_value_t::hset(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field, const std::string &value) {
    value_sizer_t<redis_nested_string_value_t> sizer(super_sizer->block_size());

    // Construct the value
    scoped_malloc<redis_nested_string_value_t> btree_value(sizer.max_possible_size());
    // TODO! Should use blobs, plus this check is bad
    guarantee(value.length() < sizer.max_possible_size() - sizeof(btree_value->contents));
    memcpy(btree_value->contents, value.data(), value.length());
    btree_value->length = value.length();

    // Find the element
    // TODO! Rather use a proper timestamp
    keyvalue_location_t<redis_nested_string_value_t> kv_location;
    scoped_malloc<btree_key_t> btree_key;
    find_nested_keyvalue_location_for_write(super_sizer->block_size(), transaction, field, repli_timestamp_t::invalid, &kv_location, &btree_key);

    // Update/insert the value
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

void redis_demo_hash_value_t::clear(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread) {
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
        fprintf(stderr, "Deleting %s\n", keys[i].c_str());
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

boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > redis_demo_hash_value_t::hgetall(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread) const {
    boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> > sizer_ptr(new value_sizer_t<redis_nested_string_value_t>(super_sizer->block_size()));

    // We nest a slice_keys iterator inside a transform iterator
    store_key_t none_key;
    none_key.size = 0;
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(nested_root));
    slice_keys_iterator_t<redis_nested_string_value_t> *tree_iter =
            new slice_keys_iterator_t<redis_nested_string_value_t>(sizer_ptr, transaction, nested_btree_sb, slice_home_thread, rget_bound_none, none_key, rget_bound_none, none_key);
    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > transform_iter(
            new transform_iterator_t<key_value_pair_t<redis_nested_string_value_t>, std::pair<std::string, std::string> >(
                    boost::bind(&redis_demo_hash_value_t::transform_value, this, _1), tree_iter));

    return transform_iter;
}

void redis_demo_hash_value_t::find_nested_keyvalue_location_for_write(block_size_t block_size, boost::shared_ptr<transaction_t> &transaction, const std::string &field, repli_timestamp_t ts, keyvalue_location_t<redis_nested_string_value_t> *kv_location, scoped_malloc<btree_key_t> *btree_key) {
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(nested_root));

    // Construct a sizer for the sub tree, using the same block size as the super tree
    value_sizer_t<redis_nested_string_value_t> sizer(block_size);

    got_superblock_t got_superblock;
    got_superblock.sb.swap(nested_btree_sb);
    got_superblock.txn = transaction;

    // Construct the key
    construct_key(field, btree_key);

    // Find the element
    ::find_keyvalue_location_for_write(&sizer, &got_superblock, btree_key->get(), ts, kv_location);
}

void redis_demo_hash_value_t::find_nested_keyvalue_location_for_read(block_size_t block_size, boost::shared_ptr<transaction_t> &transaction, const std::string &field, keyvalue_location_t<redis_nested_string_value_t> *kv_location) const {
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(nested_root));

    // Construct a sizer for the sub tree, using the same block size as the super tree
    value_sizer_t<redis_nested_string_value_t> sizer(block_size);

    got_superblock_t got_superblock;
    got_superblock.sb.swap(nested_btree_sb);
    got_superblock.txn = transaction;

    // Construct the key
    scoped_malloc<btree_key_t> btree_key;
    construct_key(field, &btree_key);

    // Find the element
    ::find_keyvalue_location_for_read(&sizer, &got_superblock, btree_key.get(), kv_location);
}

#endif	/* __SERVER_NESTED_DEMO_REDIS_HASH_VALUES_HPP__ */
