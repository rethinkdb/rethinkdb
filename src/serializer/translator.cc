// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/translator.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/new_mutex.hpp"
#include "concurrency/pmap.hpp"
#include "debug.hpp"
#include "serializer/buf_ptr.hpp"
#include "serializer/config.hpp"
#include "serializer/types.hpp"

/* serializer_multiplexer_t */

/* The multiplexing serializer slices up the serializers as follows:

- Each proxy is assigned to a serializer. The formula is (proxy_id % n_files)
- Block ID 0 on each serializer is for static configuration information.
- Each proxy uses block IDs of the form (1 + (n * (number of proxies on serializer) +
    (proxy_id / n_files))).
*/

const block_magic_t multiplexer_config_block_t::expected_magic = { { 'c', 'f', 'g', '_' } };

int compute_mod_count(int32_t file_number, int32_t n_files, int32_t n_slices) {
    /* If we have 'n_files', and we distribute 'n_slices' over those 'n_files' such that slice 'n'
    is on file 'n % n_files', then how many slices will be on file 'file_number'? */
    return n_slices / n_files + (n_slices % n_files > file_number);
}

counted_t<standard_block_token_t> serializer_block_write(serializer_t *ser, const buf_ptr_t &buf,
                                                         block_id_t block_id, file_account_t *io_account) {
    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;

    std::vector<counted_t<standard_block_token_t> > tokens
        = ser->block_writes({ buf_write_info_t(buf.ser_buffer(), buf.block_size(),
                                               block_id) },
                            io_account, &cb);
    guarantee(tokens.size() == 1);
    cb.wait();
    return tokens[0];

}

void prep_serializer(
        const std::vector<serializer_t *>& serializers,
        creation_timestamp_t creation_timestamp,
        int n_proxies,
        int i) {

    serializer_t *ser = serializers[i];

    /* Go to the thread the serializer is running on because it can only be accessed safely from
    that thread */
    on_thread_t thread_switcher(ser->home_thread());

    /* Write the initial configuration block */
    const block_size_t config_block_size = ser->max_block_size();
    buf_ptr_t buf = buf_ptr_t::alloc_zeroed(config_block_size);

    multiplexer_config_block_t *c
        = static_cast<multiplexer_config_block_t *>(buf.cache_data());

    c->magic = multiplexer_config_block_t::expected_magic;
    c->creation_timestamp = creation_timestamp;
    c->n_files = serializers.size();
    c->this_serializer = i;
    c->n_proxies = n_proxies;

    index_write_op_t op(CONFIG_BLOCK_ID.ser_id);
    op.token = serializer_block_write(ser, buf,
                                      CONFIG_BLOCK_ID.ser_id, DEFAULT_DISK_ACCOUNT);
    op.recency = repli_timestamp_t::invalid;
    {
        std::vector<index_write_op_t> ops;
        ops.push_back(std::move(op));

        new_mutex_in_line_t dummy_acq;  // We don't have ordering concerns between
                                        // this and any other index_write call.
        ser->index_write(&dummy_acq, ops);
    }
}

/* static */
void serializer_multiplexer_t::create(const std::vector<serializer_t *>& underlying, int n_proxies) {
    /* Choose a more-or-less unique ID so that we can hopefully catch the case where files are
    mixed and mismatched. */
    creation_timestamp_t creation_timestamp = time(NULL);

    /* Write a configuration block for each one */
    pmap(underlying.size(), boost::bind(&prep_serializer,
        underlying, creation_timestamp, n_proxies, _1));
}

void create_proxies(const std::vector<serializer_t *>& underlying,
    creation_timestamp_t creation_timestamp, std::vector<translator_serializer_t *> *proxies, int i) {

    serializer_t *ser = underlying[i];

    /* Go to the thread the serializer is running on because it is only safe to access on that
    thread and because the pseudoserializers must be created on that thread */
    on_thread_t thread_switcher(ser->home_thread());

    /* Load config block */
    buf_ptr_t buf
        = ser->block_read(ser->index_read(CONFIG_BLOCK_ID.ser_id),
                          DEFAULT_DISK_ACCOUNT);
    guarantee(buf.block_size() == ser->max_block_size());

    multiplexer_config_block_t *c
        = reinterpret_cast<multiplexer_config_block_t *>(buf.cache_data());

    /* Verify that stuff is sane */
    if (c->magic != multiplexer_config_block_t::expected_magic) {
        fail_due_to_user_error("File did not come from 'rethinkdb create'.");
    }
    if (c->creation_timestamp != creation_timestamp) {
        fail_due_to_user_error("The files that the server was started with didn't all come from "
            "the same call to 'rethinkdb create'.");
    }

    guarantee(c->n_files == static_cast<int>(underlying.size()));
    guarantee(c->this_serializer >= 0 && c->this_serializer < static_cast<int>(underlying.size()));
    guarantee(c->n_proxies == static_cast<int>(proxies->size()));

    /* Figure out which serializer we are (in case files were specified to 'rethinkdb serve' in a
    different order than they were specified to 'rethinkdb create') */
    int j = c->this_serializer;

    int num_on_this_serializer = compute_mod_count(j, underlying.size(), c->n_proxies);

    /* This is a slightly weird way of phrasing this; it's done this way so I can be sure it's
    equivalent to the old way of doing things */
    for (int k = 0; k < c->n_proxies; k++) {
        /* Are we responsible for creating this proxy? */
        if (k % static_cast<int>(underlying.size()) != j)
            continue;

        rassert(!(*proxies)[k]);
        (*proxies)[k] = new translator_serializer_t(
            ser,
            num_on_this_serializer,
            k / underlying.size(),
            CONFIG_BLOCK_ID   /* Reserve block ID 0 */);
    }
}

serializer_multiplexer_t::serializer_multiplexer_t(const std::vector<serializer_t *>& underlying) {
    rassert(!underlying.empty());
    for (int i = 0; i < static_cast<int>(underlying.size()); ++i) {
        rassert(underlying[i]);
    }

    /* Figure out how many slices there are gonna be and figure out what the creation magic is */
    {
        on_thread_t thread_switcher(underlying[0]->home_thread());

        /* Load config block */
        buf_ptr_t buf = underlying[0]->block_read(underlying[0]->index_read(CONFIG_BLOCK_ID.ser_id), DEFAULT_DISK_ACCOUNT);
        guarantee(buf.block_size() == underlying[0]->max_block_size());

        multiplexer_config_block_t *c
            = reinterpret_cast<multiplexer_config_block_t *>(buf.cache_data());
        guarantee(c->magic == multiplexer_config_block_t::expected_magic,
                  "c->magic is %s", debug_strprint(c->magic).c_str());
        creation_timestamp = c->creation_timestamp;
        proxies.resize(c->n_proxies);
    }

    /* Now go to each serializer and verify it individually. We visit the first serializer twice
    (because we already visited it to get the creation magic and stuff) but that's OK. Also, create
    proxies for the serializers (populate the 'proxies' vector) */
    pmap(underlying.size(), boost::bind(&create_proxies,
        underlying, creation_timestamp, &proxies, _1));

    for (int i = 0; i < static_cast<int>(proxies.size()); ++i) rassert(proxies[i]);
}

void destroy_proxy(std::vector<translator_serializer_t *> *proxies, int i) {
    on_thread_t thread_switcher((*proxies)[i]->home_thread());
    delete (*proxies)[i];
}

serializer_multiplexer_t::~serializer_multiplexer_t() {
    pmap(proxies.size(), boost::bind(&destroy_proxy, &proxies, _1));
}

/* translator_serializer_t */

block_id_t translator_serializer_t::translate_block_id(block_id_t id, int mod_count, int mod_id, config_block_id_t cfgid) {
    return id * mod_count + mod_id + cfgid.subsequent_ser_id();
}

int translator_serializer_t::untranslate_block_id_to_mod_id(block_id_t inner_id, int mod_count, config_block_id_t cfgid) {
    // We know that inner_id == id * mod_count + mod_id + min.
    // Thus inner_id - min == id * mod_count + mod_id.
    // It follows that inner_id - min === mod_id (modulo mod_count).
    // So (inner_id - min) % mod_count == mod_id (since 0 <= mod_id < mod_count).
    // (And inner_id - min >= 0, so '%' works as expected.)
    return (inner_id - cfgid.subsequent_ser_id()) % mod_count;
}

block_id_t translator_serializer_t::untranslate_block_id_to_id(block_id_t inner_id, int mod_count, int mod_id, config_block_id_t cfgid) {
    // (simply dividing by mod_count should be sufficient, but this is cleaner)
    return (inner_id - cfgid.subsequent_ser_id() - mod_id) / mod_count;
}

block_id_t translator_serializer_t::translate_block_id(block_id_t id) const {
    rassert(id != NULL_BLOCK_ID);
    return translate_block_id(id, mod_count, mod_id, cfgid);
}

translator_serializer_t::translator_serializer_t(serializer_t *_inner, int _mod_count, int _mod_id, config_block_id_t _cfgid)
    : inner(_inner), mod_count(_mod_count), mod_id(_mod_id), cfgid(_cfgid), read_ahead_callback(NULL) {
    rassert(mod_count > 0);
    rassert(mod_id >= 0);
    rassert(mod_id < mod_count);
}

file_account_t *translator_serializer_t::make_io_account(int priority, int outstanding_requests_limit) {
    return inner->make_io_account(priority, outstanding_requests_limit);
}

void translator_serializer_t::index_write(
        new_mutex_in_line_t *mutex_acq,
        const std::vector<index_write_op_t> &write_ops) {
    std::vector<index_write_op_t> translated_ops(write_ops);
    for (auto it = translated_ops.begin(); it < translated_ops.end(); ++it) {
        it->block_id = translate_block_id(it->block_id);
    }
    inner->index_write(mutex_acq, translated_ops);
}

std::vector<counted_t<standard_block_token_t> >
translator_serializer_t::block_writes(const std::vector<buf_write_info_t> &write_infos,
                                      file_account_t *io_account, iocallback_t *cb) {
    std::vector<buf_write_info_t> tmp;
    tmp.reserve(write_infos.size());
    for (auto it = write_infos.begin(); it != write_infos.end(); ++it) {
        guarantee(it->block_id != NULL_BLOCK_ID);
        tmp.push_back(buf_write_info_t(it->buf, it->block_size,
                                       translate_block_id(it->block_id)));
    }

    return inner->block_writes(tmp, io_account, cb);
}


buf_ptr_t translator_serializer_t::block_read(const counted_t<standard_block_token_t> &token,
                                            file_account_t *io_account) {
    return inner->block_read(token, io_account);
}

counted_t<standard_block_token_t> translator_serializer_t::index_read(block_id_t block_id) {
    return inner->index_read(translate_block_id(block_id));
}

max_block_size_t translator_serializer_t::max_block_size() const {
    return inner->max_block_size();
}

bool translator_serializer_t::coop_lock_and_check() {
    return inner->coop_lock_and_check();
}

bool translator_serializer_t::is_gc_active() const {
    return inner->is_gc_active();
}

block_id_t translator_serializer_t::max_block_id() {
    int64_t x = inner->max_block_id() - cfgid.subsequent_ser_id();
    if (x <= 0) {
        x = 0;
    } else {
        while (x % mod_count != mod_id) x++;
        x /= mod_count;
    }
    rassert(translate_block_id(x) >= inner->max_block_id());

    while (x > 0) {
        --x;
        if (!get_delete_bit(x)) {
            ++x;
            break;
        }
    }
    return x;
}

segmented_vector_t<repli_timestamp_t>
translator_serializer_t::get_all_recencies(block_id_t first, block_id_t step) {
    return inner->get_all_recencies(translate_block_id(first),
                                    step * mod_count);
}

bool translator_serializer_t::get_delete_bit(block_id_t id) {
    return inner->get_delete_bit(translate_block_id(id));
}

void translator_serializer_t::offer_read_ahead_buf(
        block_id_t block_id,
        buf_ptr_t *buf,
        const counted_t<standard_block_token_t> &token) {
    inner->assert_thread();

    if (block_id <= CONFIG_BLOCK_ID.ser_id) {
        // Serializer multiplexer config block (or other blocks not untranslateable
        // to cache blocks) is not of interest.
        return;
    }

    // We only want the buffer if we are the correct shard.
    const int buf_mod_id = untranslate_block_id_to_mod_id(block_id, mod_count, cfgid);
    if (buf_mod_id != this->mod_id) {
        // We are not the correct shard.
        return;
    }

    // Okay, we take ownership of the buf, it's ours (even if read_ahead_callback is
    // NULL).
    buf_ptr_t local_buf = std::move(*buf);

    if (read_ahead_callback != NULL) {
        const block_id_t inner_block_id = untranslate_block_id_to_id(block_id, mod_count, mod_id, cfgid);
        read_ahead_callback->offer_read_ahead_buf(inner_block_id, &local_buf,
                                                  token);
    }
}

void translator_serializer_t::register_read_ahead_cb(serializer_read_ahead_callback_t *cb) {
    assert_thread();

    rassert(!read_ahead_callback);
    inner->register_read_ahead_cb(this);
    read_ahead_callback = cb;
}
void translator_serializer_t::unregister_read_ahead_cb(DEBUG_VAR serializer_read_ahead_callback_t *cb) {
    assert_thread();

    rassert(read_ahead_callback == NULL || cb == read_ahead_callback);
    inner->unregister_read_ahead_cb(this);
    read_ahead_callback = NULL;
}
