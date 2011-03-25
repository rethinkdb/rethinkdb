#include "serializer/translator.hpp"
#include "concurrency/pmap.hpp"
#include "serializer/types.hpp"

/* serializer_multiplexer_t */

/* The multiplexing serializer slices up the serializers as follows:

- Each proxy is assigned to a serializer. The formula is (proxy_id % n_files)
- Block ID 0 on each serializer is for static configuration information.
- Each proxy uses block IDs of the form (1 + (n * (number of proxies on serializer) +
    (proxy_id / n_files))).
*/

const block_magic_t multiplexer_config_block_t::expected_magic = { { 'c','f','g','_' } };

void prep_serializer(
        const std::vector<serializer_t *> &serializers,
        creation_timestamp_t creation_timestamp,
        int n_proxies,
        int i) {

    serializer_t *ser = serializers[i];

    /* Go to the thread the serializer is running on because it can only be accessed safely from
    that thread */
    on_thread_t thread_switcher(ser->home_thread);

    /* Write the initial configuration block */
    multiplexer_config_block_t *c = reinterpret_cast<multiplexer_config_block_t *>(
        ser->malloc());

    bzero(c, ser->get_block_size().value());
    c->magic = multiplexer_config_block_t::expected_magic;
    c->creation_timestamp = creation_timestamp;
    c->n_files = serializers.size();
    c->this_serializer = i;
    c->n_proxies = n_proxies;

    serializer_t::write_t w = serializer_t::write_t::make(CONFIG_BLOCK_ID.ser_id, repli_timestamp::invalid, c, true, NULL);
    struct : public serializer_t::write_txn_callback_t, public cond_t {
        void on_serializer_write_txn() { pulse(); }
    } write_cb;
    if (!ser->do_write(&w, 1, &write_cb)) write_cb.wait();

    ser->free(c);
}

/* static */
void serializer_multiplexer_t::create(const std::vector<serializer_t *> &underlying, int n_proxies) {
    /* Choose a more-or-less unique ID so that we can hopefully catch the case where files are
    mixed and mismatched. */
    creation_timestamp_t creation_timestamp = time(NULL);

    /* Write a configuration block for each one */
    pmap(underlying.size(), boost::bind(&prep_serializer,
        underlying, creation_timestamp, n_proxies, _1));
}

void create_proxies(const std::vector<serializer_t *> &underlying,
    creation_timestamp_t creation_timestamp, std::vector<translator_serializer_t *> *proxies, int i) {

    serializer_t *ser = underlying[i];

    /* Go to the thread the serializer is running on because it is only safe to access on that
    thread and because the pseudoserializers must be created on that thread */
    on_thread_t thread_switcher(ser->home_thread);

    /* Load config block */
    multiplexer_config_block_t *c = reinterpret_cast<multiplexer_config_block_t *>(ser->malloc());
    struct : public serializer_t::read_callback_t, public cond_t {
        void on_serializer_read() { pulse(); }
    } read_cb;
    if (!ser->do_read(CONFIG_BLOCK_ID.ser_id, c, &read_cb))
        read_cb.wait();

    /* Verify that stuff is sane */
    if (!check_magic<multiplexer_config_block_t>(c->magic)) {
        fail_due_to_user_error("File did not come from 'rethinkdb create'.");
    }
    if (c->creation_timestamp != creation_timestamp) {
        fail_due_to_user_error("The files that the server was started with didn't all come from "
            "the same call to 'rethinkdb create'.");
    }
    if (c->n_files > (int)underlying.size()) {
        fail_due_to_user_error("You forgot to include one of the files/devices that you included "
            "when the database was first created.");
    }
    if (c->n_files < (int)underlying.size()) {
        fail_due_to_user_error("Got more files than expected. Did you give a file that wasn't"
            "included in the call to 'rethinkdb create'? Did you duplicate one of the files?");
    }
    guarantee(c->this_serializer >= 0 && c->this_serializer < (int)underlying.size());
    guarantee(c->n_proxies == (int)proxies->size());

    /* Figure out which serializer we are (in case files were specified to 'rethinkdb serve' in a
    different order than they were specified to 'rethinkdb create') */
    int j = c->this_serializer;

    int num_on_this_serializer = serializer_multiplexer_t::compute_mod_count(
        j, underlying.size(), c->n_proxies);

    /* This is a slightly weird way of phrasing this; it's done this way so I can be sure it's
    equivalent to the old way of doing things */
    for (int k = 0; k < c->n_proxies; k++) {
        /* Are we responsible for creating this proxy? */
        if (k % (int)underlying.size() != j)
            continue;

        rassert(!(*proxies)[k]);
        (*proxies)[k] = new translator_serializer_t(
            ser,
            num_on_this_serializer,
            k / underlying.size(),
            CONFIG_BLOCK_ID   /* Reserve block ID 0 */
            );
    }

    ser->free(c);
}

serializer_multiplexer_t::serializer_multiplexer_t(const std::vector<serializer_t *> &underlying) {
    rassert(underlying.size() > 0);
    for (int i = 0; i < (int)underlying.size(); i++) rassert(underlying[i]);

    /* Figure out how many slices there are gonna be and figure out what the creation magic is */
    {
        on_thread_t thread_switcher(underlying[0]->home_thread);

        /* Load config block */
        multiplexer_config_block_t *c = reinterpret_cast<multiplexer_config_block_t *>(
            underlying[0]->malloc());
        struct : public serializer_t::read_callback_t, public cond_t {
            void on_serializer_read() { pulse(); }
        } read_cb;
        if (!underlying[0]->do_read(CONFIG_BLOCK_ID.ser_id, c, &read_cb)) read_cb.wait();

        rassert(check_magic<multiplexer_config_block_t>(c->magic));
        creation_timestamp = c->creation_timestamp;
        proxies.resize(c->n_proxies);

        underlying[0]->free(c);
    }

    /* Now go to each serializer and verify it individually. We visit the first serializer twice
    (because we already visited it to get the creation magic and stuff) but that's OK. Also, create
    proxies for the serializers (populate the 'proxies' vector) */
    pmap(underlying.size(), boost::bind(&create_proxies,
        underlying, creation_timestamp, &proxies, _1));

    for (int i = 0; i < (int)proxies.size(); i++) rassert(proxies[i]);
}

void destroy_proxy(std::vector<translator_serializer_t *> *proxies, int i) {
    on_thread_t thread_switcher((*proxies)[i]->home_thread);
    delete (*proxies)[i];
}

serializer_multiplexer_t::~serializer_multiplexer_t() {
    pmap(proxies.size(), boost::bind(&destroy_proxy, &proxies, _1));
}

int serializer_multiplexer_t::compute_mod_count(int32_t file_number, int32_t n_files, int32_t n_slices) {
    /* If we have 'n_files', and we distribute 'n_slices' over those 'n_files' such that slice 'n'
    is on file 'n % n_files', then how many slices will be on file 'file_number'? */
    return n_slices / n_files + (n_slices % n_files > file_number);
}

/* translator_serializer_t */

ser_block_id_t translator_serializer_t::translate_block_id(block_id_t id, int mod_count, int mod_id, config_block_id_t cfgid) {
    return ser_block_id_t::make(id * mod_count + mod_id + cfgid.subsequent_ser_id().value);
}

int translator_serializer_t::untranslate_block_id_to_mod_id(ser_block_id_t inner_id, int mod_count, config_block_id_t cfgid) {
    // We know that inner_id == id * mod_count + mod_id + min.
    // Thus inner_id - min == id * mod_count + mod_id.
    // It follows that inner_id - min === mod_id (modulo mod_count).
    // So (inner_id - min) % mod_count == mod_id (since 0 <= mod_id < mod_count).
    // (And inner_id - min >= 0, so '%' works as expected.)
    return (inner_id.value - cfgid.subsequent_ser_id().value) % mod_count;
}

block_id_t translator_serializer_t::untranslate_block_id_to_id(ser_block_id_t inner_id, int mod_count, int mod_id, config_block_id_t cfgid) {
    // (simply dividing by mod_count should be sufficient, but this is cleaner)
    return (inner_id.value - cfgid.subsequent_ser_id().value - mod_id) / mod_count;
}

ser_block_id_t translator_serializer_t::xlate(block_id_t id) {
    return translate_block_id(id, mod_count, mod_id, cfgid);
}

translator_serializer_t::translator_serializer_t(serializer_t *inner_, int mod_count_, int mod_id_, config_block_id_t cfgid_)
    : inner(inner_), mod_count(mod_count_), mod_id(mod_id_), cfgid(cfgid_), read_ahead_callback(NULL) {
    rassert(mod_count > 0);
    rassert(mod_id >= 0);
    rassert(mod_id < mod_count);
}

void *translator_serializer_t::malloc() {
    return inner->malloc();
}

void *translator_serializer_t::clone(void *data) {
    return inner->clone(data);
}

void translator_serializer_t::free(void *ptr) {
    inner->free(ptr);
}

bool translator_serializer_t::do_read(block_id_t block_id, void *buf, serializer_t::read_callback_t *callback) {
    return inner->do_read(xlate(block_id), buf, callback);
}

ser_transaction_id_t translator_serializer_t::get_current_transaction_id(block_id_t block_id, const void* buf) {
    return inner->get_current_transaction_id(xlate(block_id), buf);
}

struct write_fsm_t : public serializer_t::write_txn_callback_t, public serializer_t::write_tid_callback_t {
    std::vector<serializer_t::write_t> writes;
    serializer_t::write_txn_callback_t *cb;
    serializer_t::write_tid_callback_t *tid_cb;
    void on_serializer_write_txn() {
        cb->on_serializer_write_txn();
        delete this;
    }
    void on_serializer_write_tid() {
        if (tid_cb) {
            tid_cb->on_serializer_write_tid();
        }
    }
};

bool translator_serializer_t::do_write(write_t *writes, int num_writes, serializer_t::write_txn_callback_t *callback, serializer_t::write_tid_callback_t *tid_callback) {
    write_fsm_t *fsm = new write_fsm_t();
    fsm->cb = callback;
    fsm->tid_cb = tid_callback;
    for (int i = 0; i < num_writes; i++) {
        fsm->writes.push_back(serializer_t::write_t(xlate(writes[i].block_id), writes[i].recency_specified, writes[i].recency,
                                                writes[i].buf_specified, writes[i].buf, writes[i].write_empty_deleted_block, writes[i].callback, writes[i].assign_transaction_id));
    }
    if (inner->do_write(fsm->writes.data(), num_writes, fsm, fsm)) {
        delete fsm;
        return true;
    } else {
        return false;
    }
}


block_size_t translator_serializer_t::get_block_size() {
    return inner->get_block_size();
}

block_id_t translator_serializer_t::max_block_id() {
    int64_t x = inner->max_block_id().value - cfgid.subsequent_ser_id().value;
    if (x <= 0) {
        x = 0;
    } else {
        while (x % mod_count != mod_id) x++;
        x /= mod_count;
    }
    rassert(xlate(x).value >= inner->max_block_id().value);

    while (x > 0) {
        --x;
        if (block_in_use(x)) {
            ++x;
            break;
        }
    }
    return x;
}


bool translator_serializer_t::block_in_use(block_id_t id) {
    return inner->block_in_use(xlate(id));
}

repli_timestamp translator_serializer_t::get_recency(block_id_t id) {
    return inner->get_recency(xlate(id));
}

bool translator_serializer_t::offer_read_ahead_buf(ser_block_id_t block_id, void *buf, repli_timestamp recency_timestamp) {
    inner->assert_thread();

    // Offer the buffer if we are the correct shard
    const int buf_mod_id = untranslate_block_id_to_mod_id(block_id, mod_count, cfgid);
    if (buf_mod_id != this->mod_id) {
        // We are not responsible...
        return false;
    }

    if (read_ahead_callback) {
        const block_id_t inner_block_id = untranslate_block_id_to_id(block_id, mod_count, mod_id, cfgid);
        read_ahead_callback->offer_read_ahead_buf(inner_block_id, buf, recency_timestamp);
    } else {
        // Discard the buffer
        inner->free(buf);
    }
    return true;
}

void translator_serializer_t::register_read_ahead_cb(translator_serializer_t::read_ahead_callback_t *cb) {
    on_thread_t(inner->home_thread);

    rassert(!read_ahead_callback);
    inner->register_read_ahead_cb(this);
    read_ahead_callback = cb;
}
void translator_serializer_t::unregister_read_ahead_cb(UNUSED translator_serializer_t::read_ahead_callback_t *cb) {
    on_thread_t(inner->home_thread);

    rassert(read_ahead_callback == NULL || cb == read_ahead_callback);
    inner->unregister_read_ahead_cb(this);
    read_ahead_callback = NULL;
}
