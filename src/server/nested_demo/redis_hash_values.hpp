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

    boost::optional<std::string> hget(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::scoped_ptr<transaction_t> &transaction, const std::string &field) const;
    int hlen() const;
    // In contrast to the redis command, this deletes only a single field. Call multiple times if necessary.
    // returns true if a field has been deleted (i.e. existed in the hash)
    bool hdel(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::scoped_ptr<transaction_t> &transaction, const std::string &field);
    // Returns true iff the field was new
    bool hset(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::scoped_ptr<transaction_t> &transaction, const std::string &field, const std::string &value);

    // So you can construct an iterator over the nested data
    // TODO! This should go away and instead construct an iterator itself, returning
    // an abstract iterator type. That will allow us to use similar methods for other
    // redis types, such as sorted sets which have to work a bit differently.
    boost::shared_ptr<superblock_t> get_nested_superblock() const {
        return boost::shared_ptr<superblock_t>(new virtual_superblock_t(nested_root));
    }

private:
    // Constructs a btree_key_t from key and puts it into out_buf
    void construct_key(const std::string &key, scoped_malloc<btree_key_t> *out_buf) const {
        scoped_malloc<btree_key_t> tmp(offsetof(btree_key_t, contents) + key.length());
        out_buf->swap(tmp);
        (*out_buf)->size = key.length();
        memcpy((*out_buf)->contents, key.data(), key.length());
    }
};
template <>
class value_sizer_t<redis_demo_hash_value_t> {
public:
    value_sizer_t<redis_demo_hash_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const redis_demo_list_value_t *value) const {
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
        block_magic_t magic = { { 'l', 'r', 'l', 'i' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};


/* TODO! Implementations...*/
boost::optional<std::string> redis_demo_hash_value_t::hget(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::scoped_ptr<transaction_t> &transaction, const std::string &field) const {
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(nested_root));

    // Construct a sizer for the sub tree, using the same block size as the super tree
    value_sizer_t<redis_nested_string_value_t> sizer(super_sizer->block_size());

    got_superblock_t got_superblock;
    got_superblock.sb.swap(nested_btree_sb);
    got_superblock.txn.swap(transaction);
    keyvalue_location_t<redis_nested_string_value_t> kv_location;

    // Construct the key
    scoped_malloc<btree_key_t> btree_key;
    construct_key(field, &btree_key);

    // Find the element
    find_keyvalue_location_for_read(&sizer, &got_superblock, btree_key.get(), &kv_location);

    // Swap the transaction back in, we don't need it anymore...
    transaction.swap(kv_location.txn);

    // Get out the string value
    if (kv_location.there_originally_was_value) {
        std::string value;
        value.assign(kv_location.value->contents, kv_location.value->length);
        return boost::optional<std::string>(value);
    } else {
        return boost::none;
    }    
}

int redis_demo_hash_value_t::hlen() const {
    return static_cast<int>(size);
}

bool redis_demo_hash_value_t::hdel(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::scoped_ptr<transaction_t> &transaction, const std::string &field) {
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(nested_root));

    // Construct a sizer for the sub tree, using the same block size as the super tree
    value_sizer_t<redis_nested_string_value_t> sizer(super_sizer->block_size());

    got_superblock_t got_superblock;
    got_superblock.sb.swap(nested_btree_sb);
    got_superblock.txn.swap(transaction);
    keyvalue_location_t<redis_nested_string_value_t> kv_location;

    // Construct the key
    scoped_malloc<btree_key_t> btree_key;
    construct_key(field, &btree_key);

    // Find the element
    // TODO! Rather use a proper timestamp
    find_keyvalue_location_for_write(&sizer, &got_superblock, btree_key.get(), repli_timestamp_t::invalid , &kv_location);

    // Delete the key
    kv_location.value.reset(); // Remove value
    // TODO! Rather use a proper timestamp
    apply_keyvalue_change(&sizer, &kv_location, btree_key.get(), repli_timestamp_t::invalid);

    // Swap the transaction back in, we don't need it anymore...
    transaction.swap(kv_location.txn);

    // Update the nested root id (in case the root got removed)
    nested_root = kv_location.sb->get_root_block_id();

    return kv_location.there_originally_was_value;
}

bool redis_demo_hash_value_t::hset(value_sizer_t<redis_demo_hash_value_t> *super_sizer, boost::scoped_ptr<transaction_t> &transaction, const std::string &field, const std::string &value) {
    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(nested_root));

    // Construct a sizer for the sub tree, using the same block size as the super tree
    value_sizer_t<redis_nested_string_value_t> sizer(super_sizer->block_size());

    got_superblock_t got_superblock;
    got_superblock.sb.swap(nested_btree_sb);
    got_superblock.txn.swap(transaction);
    keyvalue_location_t<redis_nested_string_value_t> kv_location;

    // Construct the key
    scoped_malloc<btree_key_t> btree_key;
    construct_key(field, &btree_key);

    // Construct the value
    scoped_malloc<redis_nested_string_value_t> btree_value(sizer.max_possible_size());
    // TODO! Should use blobs, plus this check is bad
    guarantee(value.length() < sizer.max_possible_size() - sizeof(btree_value->contents));
    memcpy(btree_value->contents, value.data(), value.length());

    // Find the element
    // TODO! Rather use a proper timestamp
    find_keyvalue_location_for_write(&sizer, &got_superblock, btree_key.get(), repli_timestamp_t::invalid , &kv_location);

    // Update/insert the value
    kv_location.value.swap(btree_value);
    // TODO! Rather use a proper timestamp
    apply_keyvalue_change(&sizer, &kv_location, btree_key.get(), repli_timestamp_t::invalid);

    // Swap the transaction back in, we don't need it anymore...
    transaction.swap(kv_location.txn);

    // Update the nested root id (in case this was the first field and a root got added)
    nested_root = kv_location.sb->get_root_block_id();

    return !kv_location.there_originally_was_value;
}

#endif	/* __SERVER_NESTED_DEMO_REDIS_HASH_VALUES_HPP__ */
