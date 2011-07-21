#ifndef __SERVER_NESTED_BTREE_DEMO_HPP__
#define	__SERVER_NESTED_BTREE_DEMO_HPP__

#include <math.h>

#include "db_thread_info.hpp"
#include "memcached/tcp_conn.hpp"
#include "memcached/file.hpp"
#include "diskinfo.hpp"
#include "concurrency/cond_var.hpp"
#include "logger.hpp"
#include "server/cmd_args.hpp"
#include "replication/master.hpp"
#include "replication/slave.hpp"
#include "server/control.hpp"
#include "gated_store.hpp"
#include "concurrency/promise.hpp"
#include "arch/os_signal.hpp"
#include "server/key_value_store.hpp"
#include "server/metadata_store.hpp"
#include "cmd_args.hpp"
#include "containers/scoped_malloc.hpp"
#include "btree/operations.hpp"
#include "btree/iteration.hpp"
#include "containers/iterators.hpp"

#include "server/nested_demo/redis_utils.hpp"
#include "server/nested_demo/redis_list_values.hpp"
#include "server/nested_demo/redis_hash_values.hpp"
#include "nested_demo/redis_hash_values.hpp"
// TODO!

struct demo_value_t {
    int length;
    char contents[];

public:
    int inline_size(UNUSED block_size_t bs) const {
        return sizeof(length) + length;
    }

    int64_t value_size() const {
        return length;
    }

    const char *value_ref() const { return contents; }
    char *value_ref() { return contents; }
};

template <>
class value_sizer_t<demo_value_t> {
public:
    value_sizer_t<demo_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const demo_value_t *value) const {
        return value->inline_size(block_size_);
    }

    bool fits(UNUSED const demo_value_t *value, UNUSED int length_available) const {
        return true;
    }

    int max_possible_size() const {
        return MAX_BTREE_VALUE_SIZE;
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 'l', 'e', 'a', 'f' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};

static void server_shutdown() {
    // Shut down the server
    thread_message_t *old_interrupt_msg = thread_pool_t::set_interrupt_message(NULL);
    /* If the interrupt message already was NULL, that means that either shutdown()
       was for some reason called before we finished starting up or shutdown() was called
       twice and this is the second time. */
    if (old_interrupt_msg) {
        if (continue_on_thread(get_num_threads()-1, old_interrupt_msg))
            call_later_on_this_thread(old_interrupt_msg);
    }
}

void nested_demo_main(cmd_config_t *cmd_config, thread_pool_t *thread_pool) {
    os_signal_cond_t os_signal_cond;
    try {
        /* Start logger */
        log_controller_t log_controller;

        /* Copy database filenames from private serializer configurations into a single vector of strings */
        std::vector<std::string> db_filenames;
        std::vector<log_serializer_private_dynamic_config_t>& serializer_private = cmd_config->store_dynamic_config.serializer_private;
        std::vector<log_serializer_private_dynamic_config_t>::iterator it;

        for (it = serializer_private.begin(); it != serializer_private.end(); ++it) {
            db_filenames.push_back((*it).db_filename);
        }

        /* Check to see if there is an existing database */
        struct : public btree_key_value_store_t::check_callback_t, public promise_t<bool> {
            void on_store_check(bool ok) { pulse(ok); }
        } check_cb;
        btree_key_value_store_t::check_existing(db_filenames, &check_cb);
        bool existing = check_cb.wait();
        if (existing && cmd_config->create_store && !cmd_config->force_create) {
            fail_due_to_user_error(
                "It looks like there already is a database here. RethinkDB will abort in case you "
                "didn't mean to overwrite it. Run with the '--force' flag to override this warning.");
        } else {
            if (!existing) {
                cmd_config->create_store = true;
            }
        }

        /* Record information about disk drives to log file */
        log_disk_info(cmd_config->store_dynamic_config.serializer_private);

        /* Create store if necessary */
        if (cmd_config->create_store) {
            logINF("Creating database...\n");
            btree_key_value_store_t::create(&cmd_config->store_dynamic_config,
                                            &cmd_config->store_static_config);
            // TODO: Shouldn't do this... Setting up the metadata static config doesn't belong here
            // and it's very hacky to build on the store_static_config.
            btree_key_value_store_static_config_t metadata_static_config = cmd_config->store_static_config;
            metadata_static_config.cache.n_patch_log_blocks = 0;
            btree_metadata_store_t::create(&cmd_config->metadata_store_dynamic_config,
                                            &metadata_static_config);
            logINF("Done creating.\n");
        }

        if (!cmd_config->shutdown_after_creation) {

            /* Start key-value store */
            logINF("Loading database...\n");
            // TODO! should not use a btree_key_value_store_t for this demo
            btree_metadata_store_t metadata_store(cmd_config->metadata_store_dynamic_config);
            btree_key_value_store_t store(cmd_config->store_dynamic_config);

            shard_store_t *shard = store.get_some_shard();

            logINF("Now running demo operations....\n");

            block_id_t nested_btree_1_root = NULL_BLOCK_ID;
            block_id_t nested_btree_2_root = NULL_BLOCK_ID;

            {
                on_thread_t thread(shard->home_thread());

                boost::shared_ptr<transaction_t> transaction(new transaction_t(&shard->cache, rwi_write, 1, repli_timestamp_t::invalid));
                boost::scoped_ptr<superblock_t> nested_btree_1_sb(new virtual_superblock_t());
                nested_btree_1_sb->set_root_block_id(nested_btree_1_root);

                keyvalue_location_t<demo_value_t> kv_location;
                value_sizer_t<demo_value_t> sizer(shard->cache.get_block_size());
                got_superblock_t got_superblock;
                got_superblock.sb.swap(nested_btree_1_sb);
                got_superblock.txn = transaction;

                std::string key_str("key_1");
                std::string value_str("value_1");
                logINF("Inserting %s -> %s into nested_btree_1...\n", key_str.c_str(), value_str.c_str());
                btree_key_t *key = reinterpret_cast<btree_key_t*>(malloc(offsetof(btree_key_t, contents) + key_str.length()));
                key->size = key_str.length();
                memcpy(key->contents, key_str.data(), key_str.length());
                find_keyvalue_location_for_write(&sizer, &got_superblock, key, repli_timestamp_t::invalid, &kv_location);
                scoped_malloc<demo_value_t> value(sizeof(demo_value_t::length) + value_str.length());
                value->length = value_str.length();
                memcpy(value->contents, value_str.data(), value_str.length());
                kv_location.value.swap(value);
                apply_keyvalue_change(&sizer, &kv_location, key, repli_timestamp_t::invalid);
                free(key);

                // Update the root block id in case it has changed
                nested_btree_1_root = kv_location.sb->get_root_block_id();
                logINF("Done, nested_btree_1 now has root %u\n", nested_btree_1_root);
            }
            {
                on_thread_t thread(shard->home_thread());

                boost::shared_ptr<transaction_t> transaction(new transaction_t(&shard->cache, rwi_write, 1, repli_timestamp_t::invalid));
                boost::scoped_ptr<superblock_t> nested_btree_1_sb(new virtual_superblock_t());
                nested_btree_1_sb->set_root_block_id(nested_btree_1_root);

                keyvalue_location_t<demo_value_t> kv_location;
                value_sizer_t<demo_value_t> sizer(shard->cache.get_block_size());
                got_superblock_t got_superblock;
                got_superblock.sb.swap(nested_btree_1_sb);
                got_superblock.txn = transaction;

                std::string key_str("key_3");
                std::string value_str("value_3");
                logINF("Inserting %s -> %s into nested_btree_1...\n", key_str.c_str(), value_str.c_str());
                btree_key_t *key = reinterpret_cast<btree_key_t*>(malloc(offsetof(btree_key_t, contents) + key_str.length()));
                key->size = key_str.length();
                memcpy(key->contents, key_str.data(), key_str.length());
                find_keyvalue_location_for_write(&sizer, &got_superblock, key, repli_timestamp_t::invalid, &kv_location);
                scoped_malloc<demo_value_t> value(sizeof(demo_value_t::length) + value_str.length());
                value->length = value_str.length();
                memcpy(value->contents, value_str.data(), value_str.length());
                kv_location.value.swap(value);
                apply_keyvalue_change(&sizer, &kv_location, key, repli_timestamp_t::invalid);
                free(key);

                // Update the root block id in case it has changed
                nested_btree_1_root = kv_location.sb->get_root_block_id();
                logINF("Done, nested_btree_1 now has root %u\n", nested_btree_1_root);
            }
            {
                on_thread_t thread(shard->home_thread());

                boost::shared_ptr<transaction_t> transaction(new transaction_t(&shard->cache, rwi_write, 1, repli_timestamp_t::invalid));
                boost::scoped_ptr<superblock_t> nested_btree_2_sb(new virtual_superblock_t());
                nested_btree_2_sb->set_root_block_id(nested_btree_2_root);

                keyvalue_location_t<demo_value_t> kv_location;
                value_sizer_t<demo_value_t> sizer(shard->cache.get_block_size());
                got_superblock_t got_superblock;
                got_superblock.sb.swap(nested_btree_2_sb);
                got_superblock.txn = transaction;

                std::string key_str("key_1");
                std::string value_str("value_1");
                logINF("Inserting %s -> %s into nested_btree_2...\n", key_str.c_str(), value_str.c_str());
                btree_key_t *key = reinterpret_cast<btree_key_t*>(malloc(offsetof(btree_key_t, contents) + key_str.length()));
                key->size = key_str.length();
                memcpy(key->contents, key_str.data(), key_str.length());
                find_keyvalue_location_for_write(&sizer, &got_superblock, key, repli_timestamp_t::invalid, &kv_location);
                scoped_malloc<demo_value_t> value(sizeof(demo_value_t::length) + value_str.length());
                value->length = value_str.length();
                memcpy(value->contents, value_str.data(), value_str.length());
                kv_location.value.swap(value);
                apply_keyvalue_change(&sizer, &kv_location, key, repli_timestamp_t::invalid);
                free(key);

                // Update the root block id in case it has changed
                nested_btree_2_root = kv_location.sb->get_root_block_id();
                logINF("Done, nested_btree_2 now has root %u\n", nested_btree_2_root);
            }
            {
                on_thread_t thread(shard->home_thread());

                boost::shared_ptr<transaction_t> transaction(new transaction_t(&shard->cache, rwi_write, 1, repli_timestamp_t::invalid));
                boost::scoped_ptr<superblock_t> nested_btree_2_sb(new virtual_superblock_t());
                nested_btree_2_sb->set_root_block_id(nested_btree_2_root);

                keyvalue_location_t<demo_value_t> kv_location;
                value_sizer_t<demo_value_t> sizer(shard->cache.get_block_size());
                got_superblock_t got_superblock;
                got_superblock.sb.swap(nested_btree_2_sb);
                got_superblock.txn = transaction;

                std::string key_str("key_2");
                std::string value_str("value_2");
                logINF("Inserting %s -> %s into nested_btree_2...\n", key_str.c_str(), value_str.c_str());
                btree_key_t *key = reinterpret_cast<btree_key_t*>(malloc(offsetof(btree_key_t, contents) + key_str.length()));
                key->size = key_str.length();
                memcpy(key->contents, key_str.data(), key_str.length());
                find_keyvalue_location_for_write(&sizer, &got_superblock, key, repli_timestamp_t::invalid, &kv_location);
                scoped_malloc<demo_value_t> value(sizeof(demo_value_t::length) + value_str.length());
                value->length = value_str.length();
                memcpy(value->contents, value_str.data(), value_str.length());
                kv_location.value.swap(value);
                apply_keyvalue_change(&sizer, &kv_location, key, repli_timestamp_t::invalid);
                free(key);

                // Update the root block id in case it has changed
                nested_btree_2_root = kv_location.sb->get_root_block_id();
                logINF("Done, nested_btree_2 now has root %u\n", nested_btree_2_root);
            }
            {
                // (flush logs)
                on_thread_t thread(shard->home_thread());
                coro_t::yield();
            }
            {
                on_thread_t thread(shard->home_thread());

                boost::shared_ptr<transaction_t> transaction(new transaction_t(&shard->cache, rwi_read));
                boost::scoped_ptr<superblock_t> nested_btree_1_sb(new virtual_superblock_t());
                nested_btree_1_sb->set_root_block_id(nested_btree_1_root);
                boost::scoped_ptr<superblock_t> nested_btree_2_sb(new virtual_superblock_t());
                nested_btree_2_sb->set_root_block_id(nested_btree_2_root);

                boost::shared_ptr<value_sizer_t<demo_value_t> > sizer_ptr(new value_sizer_t<demo_value_t>(shard->cache.get_block_size()));
                store_key_t none_key;
                none_key.size = 0;
                slice_keys_iterator_t<demo_value_t> tree1_iter(sizer_ptr, transaction, nested_btree_1_sb, shard->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);
                slice_keys_iterator_t<demo_value_t> tree2_iter(sizer_ptr, transaction, nested_btree_2_sb, shard->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);

                fprintf(stderr, "\nKeys in nested_btree_1\n");
                while (true) {
                    boost::optional<key_value_pair_t<demo_value_t> > next = tree1_iter.next();
                    if (next) {
                        fprintf(stderr, "\t%s\n", next->key.c_str());
                    } else {
                        break;
                    }
                }
                fprintf(stderr, "\nKeys in nested_btree_2\n");
                while (true) {
                    boost::optional<key_value_pair_t<demo_value_t> > next = tree2_iter.next();
                    if (next) {
                        fprintf(stderr, "\t%s\n", next->key.c_str());
                    } else {
                        break;
                    }
                }
            }
            {
                on_thread_t thread(shard->home_thread());

                boost::shared_ptr<transaction_t> transaction(new transaction_t(&shard->cache, rwi_read));
                boost::scoped_ptr<superblock_t> nested_btree_1_sb(new virtual_superblock_t());
                nested_btree_1_sb->set_root_block_id(nested_btree_1_root);
                boost::scoped_ptr<superblock_t> nested_btree_2_sb(new virtual_superblock_t());
                nested_btree_2_sb->set_root_block_id(nested_btree_2_root);

                boost::shared_ptr<value_sizer_t<demo_value_t> > sizer_ptr(new value_sizer_t<demo_value_t>(shard->cache.get_block_size()));
                store_key_t none_key;
                none_key.size = 0;
                slice_keys_iterator_t<demo_value_t> *tree1_iter = new slice_keys_iterator_t<demo_value_t>(sizer_ptr, transaction, nested_btree_1_sb, shard->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);
                slice_keys_iterator_t<demo_value_t> *tree2_iter = new slice_keys_iterator_t<demo_value_t>(sizer_ptr, transaction, nested_btree_2_sb, shard->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);
                merge_ordered_data_iterator_t<key_value_pair_t<demo_value_t> > *merge_iter = new merge_ordered_data_iterator_t<key_value_pair_t<demo_value_t> >();
                merge_iter->add_mergee(tree1_iter);
                merge_iter->add_mergee(tree2_iter);
                unique_filter_iterator_t<key_value_pair_t<demo_value_t> > union_iter(merge_iter);

                fprintf(stderr, "\nUnion of nested_btree_1 and nested_btree_2\n");
                while (true) {
                    boost::optional<key_value_pair_t<demo_value_t> > next = union_iter.next();
                    if (next) {
                        fprintf(stderr, "\t%s\n", next->key.c_str());
                    } else {
                        break;
                    }
                }
            }
            {
                on_thread_t thread(shard->home_thread());

                boost::shared_ptr<transaction_t> transaction(new transaction_t(&shard->cache, rwi_read));
                boost::scoped_ptr<superblock_t> nested_btree_1_sb(new virtual_superblock_t());
                nested_btree_1_sb->set_root_block_id(nested_btree_1_root);
                boost::scoped_ptr<superblock_t> nested_btree_2_sb(new virtual_superblock_t());
                nested_btree_2_sb->set_root_block_id(nested_btree_2_root);

                boost::shared_ptr<value_sizer_t<demo_value_t> > sizer_ptr(new value_sizer_t<demo_value_t>(shard->cache.get_block_size()));
                store_key_t none_key;
                none_key.size = 0;
                slice_keys_iterator_t<demo_value_t> *tree1_iter = new slice_keys_iterator_t<demo_value_t>(sizer_ptr, transaction, nested_btree_1_sb, shard->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);
                slice_keys_iterator_t<demo_value_t> *tree2_iter = new slice_keys_iterator_t<demo_value_t>(sizer_ptr, transaction, nested_btree_2_sb, shard->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);
                merge_ordered_data_iterator_t<key_value_pair_t<demo_value_t> > *merge_iter = new merge_ordered_data_iterator_t<key_value_pair_t<demo_value_t> >();
                merge_iter->add_mergee(tree1_iter);
                merge_iter->add_mergee(tree2_iter);
                // This gives the intersection only because both input iterators already provide sets.
                // Otherwise there could be duplicates and you would have to filter everything through an
                // additional unique_filter_iterator.
                repetition_filter_iterator_t<key_value_pair_t<demo_value_t> > intersection_iter(merge_iter, 2);

                fprintf(stderr, "\nIntersection of nested_btree_1 and nested_btree_2\n");
                while (true) {
                    boost::optional<key_value_pair_t<demo_value_t> > next = intersection_iter.next();
                    if (next) {
                        fprintf(stderr, "\t%s\n", next->key.c_str());
                    } else {
                        break;
                    }
                }
            }
            {
                on_thread_t thread(shard->home_thread());

                boost::shared_ptr<transaction_t> transaction(new transaction_t(&shard->cache, rwi_read));
                boost::scoped_ptr<superblock_t> nested_btree_1_sb(new virtual_superblock_t());
                nested_btree_1_sb->set_root_block_id(nested_btree_1_root);
                boost::scoped_ptr<superblock_t> nested_btree_2_sb(new virtual_superblock_t());
                nested_btree_2_sb->set_root_block_id(nested_btree_2_root);

                boost::shared_ptr<value_sizer_t<demo_value_t> > sizer_ptr(new value_sizer_t<demo_value_t>(shard->cache.get_block_size()));
                store_key_t none_key;
                none_key.size = 0;
                slice_keys_iterator_t<demo_value_t> *tree1_iter = new slice_keys_iterator_t<demo_value_t>(sizer_ptr, transaction, nested_btree_1_sb, shard->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);
                slice_keys_iterator_t<demo_value_t> *tree2_iter = new slice_keys_iterator_t<demo_value_t>(sizer_ptr, transaction, nested_btree_2_sb, shard->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);
                // This works because both input iterators provide a sorted key/value stream.
                diff_filter_iterator_t<key_value_pair_t<demo_value_t> > diff_iter(tree1_iter, tree2_iter);

                fprintf(stderr, "\nDifference of nested_btree_1 and nested_btree_2\n");
                while (true) {
                    boost::optional<key_value_pair_t<demo_value_t> > next = diff_iter.next();
                    if (next) {
                        fprintf(stderr, "\t%s\n", next->key.c_str());
                    } else {
                        break;
                    }
                }
            }

            // Now some redis value type tests...
            block_id_t redis_hash_btree_root = NULL_BLOCK_ID;
            {
                on_thread_t thread(shard->home_thread());

                boost::shared_ptr<transaction_t> transaction(new transaction_t(&shard->cache, rwi_write, 1, repli_timestamp_t::invalid));
                boost::scoped_ptr<superblock_t> redis_hash_btree_sb(new virtual_superblock_t());
                redis_hash_btree_sb->set_root_block_id(redis_hash_btree_root);

                keyvalue_location_t<redis_demo_hash_value_t> kv_location;
                value_sizer_t<redis_demo_hash_value_t> sizer(shard->cache.get_block_size());
                got_superblock_t got_superblock;
                got_superblock.sb.swap(redis_hash_btree_sb);
                got_superblock.txn = transaction;

                std::string key_str("key_1");
                logINF("Inserting %s -> hash into redis_hash_btree...\n", key_str.c_str());
                btree_key_t *key = reinterpret_cast<btree_key_t*>(malloc(offsetof(btree_key_t, contents) + key_str.length()));
                key->size = key_str.length();
                memcpy(key->contents, key_str.data(), key_str.length());
                find_keyvalue_location_for_write(&sizer, &got_superblock, key, repli_timestamp_t::invalid, &kv_location);
                scoped_malloc<redis_demo_hash_value_t> value(sizeof(redis_demo_hash_value_t));
                value->nested_root = NULL_BLOCK_ID;
                value->size = 0;
                kv_location.value.swap(value);
                apply_keyvalue_change(&sizer, &kv_location, key, repli_timestamp_t::invalid);
                free(key);

                // Update the root block id in case it has changed
                redis_hash_btree_root = kv_location.sb->get_root_block_id();
                logINF("Done, redis_hash_btree now has root %u\n", redis_hash_btree_root);
            }
            
            // Ok, for now we are not actually using the redis_demo_hash_value_t we just inserted but use
            // an in-memory one...
            redis_demo_hash_value_t demo_hash_value;
            demo_hash_value.nested_root = NULL_BLOCK_ID;
            demo_hash_value.size = 0;
            {
                on_thread_t thread(shard->home_thread());

                boost::shared_ptr<transaction_t> transaction(new transaction_t(&shard->cache, rwi_write, 1, repli_timestamp_t::invalid));
                value_sizer_t<redis_demo_hash_value_t> sizer(shard->cache.get_block_size());

                demo_hash_value.hset(&sizer, transaction, "field_1", "value_1");
            }

            /* TODO!
             * Build a proper testing framework, that hooks the trees into values
             * Deletion: Deleting a key requires first deleting all keys in the nested tree
             * Implement a smart value type that represents Redis lists (lex. sorted int keys, string values, size in super value)
             * Implement a smart value type that represents Redis hash sets (string keys, string values, size in super value)
             * Implement a smart value type that represents Redis sets (string keys, empty values, size in super value)
             * Implement a smart value type that represents Redis sorted sets (tree 1: string keys, score values; tree 2: lex. sorted score + string keys, empty values; tree 3: block_id_t keys, subtree-size values)
            */

            // TODO!

            logINF("Waiting for changes to flush to disk...\n");
            // Connections closed here
            // Store destructor called here

        } else {
            logINF("Shutting down...\n");
        }

    } catch (tcp_listener_t::address_in_use_exc_t) {
        logERR("Port %d is already in use -- aborting.\n", cmd_config->port);
    }

    /* The penultimate step of shutting down is to make sure that all messages
    have reached their destinations so that they can be freed. The way we do this
    is to send one final message to each core; when those messages all get back
    we know that all messages have been processed properly. Otherwise, logger
    shutdown messages would get "stuck" in the message hub when it shut down,
    leading to memory leaks. */
    for (int i = 0; i < get_num_threads(); i++) {
        on_thread_t thread_switcher(i);
    }

    /* Finally tell the thread pool to stop.
    TODO: We should make it so the thread pool stops automatically when server_main()
    returns. */
    thread_pool->shutdown();
}

/* Install the shutdown control for thread pool */
struct shutdown_control_t : public control_t
{
    shutdown_control_t(std::string key)
        : control_t(key, "Shut down the server.")
    {}
    std::string call(UNUSED int argc, UNUSED char **argv) {
        server_shutdown();
        // TODO: Only print this if there actually *is* a lot of unsaved data.
        return std::string("Shutting down... this may take time if there is a lot of unsaved data.\r\n");
    }
};

int run_nested_demo(int argc, char *argv[]) {

    // Parse command line arguments
    cmd_config_t config = parse_cmd_args(argc, argv);

    // Open the log file, if necessary.
    if (config.log_file_name[0]) {
        log_file = fopen(config.log_file_name.c_str(), "a");
    }

    // Initial thread message to start server
    struct server_starter_t :
        public thread_message_t
    {
        cmd_config_t *cmd_config;
        thread_pool_t *thread_pool;
        void on_thread_switch() {
            coro_t::spawn(boost::bind(&nested_demo_main, cmd_config, thread_pool));
        }
    } starter;
    starter.cmd_config = &config;


    // Run the server.
    thread_pool_t thread_pool(config.n_workers);
    starter.thread_pool = &thread_pool;
    thread_pool.run(&starter);

    logINF("Server is shut down.\n");

    // Close the log file if necessary.
    if (config.log_file_name[0]) {
        fclose(log_file);
        log_file = stderr;
    }

    return 0;
}


#endif	/* __SERVER_NESTED_BTREE_DEMO_HPP__ */

