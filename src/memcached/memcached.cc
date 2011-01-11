#include "memcached/memcached.hpp"
#include "concurrency/task.hpp"
#include "arch/arch.hpp"
#include "server.hpp"
#include <stdarg.h>

class txt_memcached_get_request_t
{
public:
    txt_memcached_handler_t *rh;
    bool with_cas;
    struct get_t {
        union {
            char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
            btree_key key;
        };
        cond_var_t<store_t::get_result_t> *result;
    };
    std::vector<get_t> gets;

    txt_memcached_get_request_t(txt_memcached_handler_t *rh, bool with_cas, int argc, char **argv)
        : rh(rh), with_cas(with_cas)
    {
        assert(argc >= 1);
        assert(strcmp(argv[0], "get") == 0 || strcmp(argv[0], "gets") == 0);

        /* First parse all of the keys to get */
        gets.reserve(argc - 1);
        for (int i = 1; i < argc; i++) {
            gets.push_back(get_t());
            if (!node_handler::str_to_key(argv[i], &gets.back().key)) {
                rh->writef("CLIENT_ERROR bad command line format\r\n");
                rh->read_next_command();
                delete this;
                return;
            }
        }
        if (gets.size() == 0) {
            rh->writef("ERROR\r\n");
            rh->read_next_command();
            delete this;
            return;
        }

        /* Now that we're sure they're all valid, send off the requests */
        for (int i = 0; i < (int)gets.size(); i++) {
            gets[i].result = task<store_t::get_result_t>(
                with_cas ? &store_t::co_get_cas : &store_t::co_get,
                rh->server->store,
                &gets[i].key);
        }

        coro_t::spawn(&txt_memcached_get_request_t::write_each, this);
    }

    void write_each() {

        for (int i = 0; i < (int)gets.size(); i++) {

            store_t::get_result_t res = gets[i].result->join();

            /* If the write half of the connection has been closed, there's no point in trying
            to send anything */
            if (rh->conn->is_write_open()) {
                btree_key &key = gets[i].key;

                /* If res.buffer is NULL that means the value was not found so we don't write
                anything */
                if (res.buffer) {

                    /* Write the "VALUE ..." header */
                    if (with_cas) {
                        rh->writef("VALUE %*.*s %u %u %llu\r\n",
                            key.size, key.size, key.contents, res.flags, res.buffer->get_size(), res.cas);
                    } else {
                        assert(res.cas == 0);
                        rh->writef("VALUE %*.*s %u %u\r\n",
                            key.size, key.size, key.contents, res.flags, res.buffer->get_size());
                    }

                    /* Write the value itself. If the value is small, write it into the send buffer;
                    otherwise, stream it. */
                    if (res.buffer->get_size() < MAX_BUFFERED_GET_SIZE) {
                        for (int i = 0; i < (signed)res.buffer->buffers.size(); i++) {
                            const_buffer_group_t::buffer_t b = res.buffer->buffers[i];
                            rh->write((const char *)b.data, b.size);
                        }
                        rh->writef("\r\n");

                    } else {
                        bool ok = true;
                        for (int i = 0; i < (signed)res.buffer->buffers.size(); i++) {
                            const_buffer_group_t::buffer_t b = res.buffer->buffers[i];
                            ok = rh->send_buffer.co_write_external(b.size, (const char *)b.data);
                            if (!ok) break;
                        }
                        if (ok) rh->writef("\r\n");
                    }
                }
            }

            /* Call the callback so that the value is released */
            if (res.buffer) res.cb->have_copied_value();
        }

        rh->writef("END\r\n");
        rh->read_next_command();
        delete this;
    }
};

/* The buffered_data_provider_t provides data from an internal buffer. */

struct buffered_data_provider_t :
    public data_provider_t
{
    /* We copy the data we are given, not refer to it. */
    buffered_data_provider_t(void *buf, size_t length)
        : buffer(malloc(length)), length(length)
    {
        memcpy(buffer, buf, length);
    }
    ~buffered_data_provider_t() {
        free(buffer);
    }
    size_t get_size() {
        return length;
    }
    void get_value(buffer_group_t *bg, data_provider_t::done_callback_t *cb) {
        if (bg) {
            int pos = 0;
            for (int i = 0; i < (signed)bg->buffers.size(); i++) {
                memcpy(bg->buffers[i].data, (char*)buffer + pos, bg->buffers[i].size);
                pos += bg->buffers[i].size;
            }
            assert(pos == (signed)length);
        }
        cb->have_provided_value();
    }
private:
    void *buffer;
    size_t length;
};

/* The memcached_streaming_data_provider_t is a data_provider_t that gets its data by
reading from a net_conn_t. In general it is used for big values that would be expensive
to buffer in memory. The disadvantage is that it requires going back and forth between
the core where the cache is and the core where the request handler is.

It also takes care of checking for a CRLF at the end of the value (the
buffered_data_provider_t doesn't do that). */

class memcached_streaming_data_provider_t :
    public data_provider_t,
    public thread_message_t,
    public home_thread_mixin_t,
    public net_conn_read_external_callback_t
{
private:
    typedef buffer_t<IO_BUFFER_SIZE> iobuf_t;

    /* Mode goes from unused->fill->check_crlf->done or unused->consume->check_crlf->done */
    enum {
        mode_unused,   // mode = mode_unused before get_value() has been called
        mode_fill,   // Fill some buffers with the data from the socket
        mode_consume,   // Discard the data from the socket
        mode_check_crlf,   // Make sure that the terminator is legal (after either mode_fill or mode_consume)
        mode_done
    } mode;

    int requestor_thread;   // thread that callback must be called on
    data_provider_t::done_callback_t *cb;

    /* false if the CRLF was not found or the connection was closed as we read */
    bool success;

    txt_memcached_handler_t *rh;   // Request handler for us to read from
    size_t length;   // Our length

    buffer_group_t *bg;   // Buffers to read into in fill mode
    int bg_seg, bg_seg_pos;   // Segment we're currently reading into and how many bytes are in it

    char *dummy_buf;   // Dummy buf used in consume mode
    size_t bytes_consumed;

    char crlf[2];   // Buf used to hold what should be CRLF

public:
    memcached_streaming_data_provider_t(txt_memcached_handler_t *rh, uint32_t length)
        : mode(mode_unused), rh(rh), length(length)
    {
    }
    
    ~memcached_streaming_data_provider_t() {
        assert(mode == mode_done);
        if (dummy_buf) {
            delete[] dummy_buf;
        }
    }
    
    size_t get_size() {
        return length;
    }
    
    void get_value(buffer_group_t *b, data_provider_t::done_callback_t *c) {
        assert(mode == mode_unused);
        if (b) {
            mode = mode_fill;
            dummy_buf = NULL;
            bg = b;
            bg_seg = bg_seg_pos = 0;
        } else {
            /* We are to read the data off the socket but not do anything with it. */
            mode = mode_consume;
            dummy_buf = new char[IO_BUFFER_SIZE];   // Space to put the garbage data
            bytes_consumed = 0;
        }
        requestor_thread = get_thread_id();
        cb = c;
        
        if (continue_on_thread(home_thread, this)) on_thread_switch();
    }

    void on_thread_switch() {
        switch (mode) {
            case mode_fill:
            case mode_consume:
                /* We're arriving on the thread with the request handler */
                assert_thread();
                on_net_conn_read_external();  // Start the read cycle
                break;
            case mode_done:
                /* We're delivering our response to the btree-FSM that asked for the value */
                assert(get_thread_id() == requestor_thread);
                if (success) cb->have_provided_value();
                else cb->have_failed();
                break;
            case mode_unused:
            case mode_check_crlf:
            default:
                unreachable();
        }
    }

private:
    void on_net_conn_read_external() {
        switch (mode) {
            case mode_fill:
                do_fill();
                break;
            case mode_consume:
                do_consume();
                break;
            case mode_check_crlf:
                do_check_crlf();
                break;
            case mode_unused:
            case mode_done:
            default:
                unreachable();
        }
    }

    void on_net_conn_close() {
        /* We get this callback if the connection is closed while we are reading the data
        from the socket. */
        success = false;
        mode = mode_done;
        if (continue_on_thread(requestor_thread, this)) on_thread_switch();
    }

    void do_consume() {
        if (bytes_consumed < length) {
            size_t bytes_to_transfer = std::min(IO_BUFFER_SIZE, (long int)length - (long int)bytes_consumed);
            bytes_consumed += bytes_to_transfer;
            if (rh->conn->is_read_open()) {
                rh->conn->read_external(dummy_buf, bytes_to_transfer, this);
            } else {
                on_net_conn_close();
            }
        } else {
            check_crlf();
        }
    }

    void do_fill() {
        if (bg_seg < (signed)bg->buffers.size()) {
            buffer_group_t::buffer_t *bg_buf = &bg->buffers[bg_seg];
            uint16_t start_pos = bg_seg_pos;
            uint16_t bytes_to_transfer = std::min(bg_buf->size - start_pos, (ssize_t)length);
            bg_seg_pos += bytes_to_transfer;
            if (bg_seg_pos == (signed)bg_buf->size) {
                bg_seg++;
                bg_seg_pos = 0;
            }
            if (rh->conn->is_read_open()) {
                rh->conn->read_external((char*)bg_buf->data + start_pos, bytes_to_transfer, this);
            } else {
                on_net_conn_close();
            }
        } else {
            check_crlf();
        }
    }

    void check_crlf() {
        /* This is called to start checking the CRLF, *before* we have filled the CRLF buffer. */
        mode = mode_check_crlf;
        if (rh->conn->is_read_open()) {
            rh->conn->read_external(crlf, 2, this);
        } else {
            on_net_conn_close();
        }
    }

    void do_check_crlf() {
        /* This is called in the process of checking the CRLF, *after* we have filled the CRLF
        buffer. */
        success = (memcmp(crlf, "\r\n", 2) == 0);
        mode = mode_done;
        if (continue_on_thread(requestor_thread, this)) on_thread_switch();
    }
};

/* Common superclass for txt_memcached_set_request_t, which handles set/add/replace/cas,
and txt_memcached_append_prepend_request_t, which handles append/prepend. The subclass
should override dispatch(). */
struct txt_memcached_storage_request_t :
    public net_conn_read_external_callback_t,
    public semaphore_available_callback_t
{
    txt_memcached_handler_t *rh;
    data_provider_t *data;

    /* If the command line says "noreply", then noreply = true. If the command line says "noreply"
    and the value is not being streamed, then nowait = true. */
    bool noreply, nowait;

    enum storage_command_t {
        set_command,
        add_command,
        replace_command,
        cas_command,
        append_command,
        prepend_command
    };

private:
    /* Temporary space to store stuff while reading the value from the socket, assuming we don't
    stream the value. */
    storage_command_t storage_command;
    union {
        char key_memory[MAX_KEY_SIZE + sizeof(btree_key)];
        btree_key key;
    };
    mcflags_t mcflags;
    exptime_t exptime;
    size_t value_size;
    cas_t req_cas;
    bool is_streaming;
    char *value_buffer;   // Only used if not streaming

public:
    txt_memcached_storage_request_t()
        : data(NULL)
    {
    }

    void init(
        txt_memcached_handler_t *r,
        storage_command_t sc,
        int argc, char **argv)
    {
        rh = r;

        char *invalid_char;

        /* cmd key flags exptime size [noreply]
        OR "cas" key flags exptime size cas [noreply] */
        if ((sc != cas_command && (argc != 5 && argc != 6)) ||
            (sc == cas_command && (argc != 6 && argc != 7))) {
            rh->writef("ERROR\r\n");
            rh->read_next_command();
            delete this;
            return;
        }

        /* Storage command */
        storage_command = sc;

        /* First parse the key */
        if (!node_handler::str_to_key(argv[1], &key)) {
            rh->writef("CLIENT_ERROR bad command line format\r\n");
            rh->read_next_command();
            delete this;
            return;
        }

        /* Next parse the flags */
        mcflags = strtoul_strict(argv[2], &invalid_char, 10);
        if (*invalid_char != '\0') {
            rh->writef("CLIENT_ERROR bad command line format\r\n");
            rh->read_next_command();
            delete this;
            return;
        }

        /* Now parse the expiration time */
        exptime = strtoul_strict(argv[3], &invalid_char, 10);
        if (*invalid_char != '\0') {
            rh->writef("CLIENT_ERROR bad command line format\r\n");
            rh->read_next_command();
            delete this;
            return;
        }
        
        // This is protocol.txt, verbatim:
        // Some commands involve a client sending some kind of expiration time
        // (relative to an item or to an operation requested by the client) to
        // the server. In all such cases, the actual value sent may either be
        // Unix time (number of seconds since January 1, 1970, as a 32-bit
        // value), or a number of seconds starting from current time. In the
        // latter case, this number of seconds may not exceed 60*60*24*30 (number
        // of seconds in 30 days); if the number sent by a client is larger than
        // that, the server will consider it to be real Unix time value rather
        // than an offset from current time.
        if (exptime <= 60*60*24*30 && exptime > 0) {
            // TODO: Do we need to handle exptimes in the past here?
            exptime += time(NULL);
        }

        /* Now parse the value length */
        value_size = strtoul_strict(argv[4], &invalid_char, 10);
        // Check for signed 32 bit max value for Memcached compatibility...
        if (*invalid_char != '\0' || value_size >= (1u << 31) - 1) {
            rh->writef("CLIENT_ERROR bad command line format\r\n");
            rh->read_next_command();
            delete this;
            return;
        }

        /* If a "cas", parse the cas_command unique */
        if (storage_command == cas_command) {
            req_cas = strtoull_strict(argv[5], &invalid_char, 10);
            if (*invalid_char != '\0') {
                rh->writef("CLIENT_ERROR bad command line format\r\n");
                rh->read_next_command();
                delete this;
                return;
            }
        }

        /* Check for noreply */
        int offset = storage_command == cas_command ? 1 : 0;
        if (argc == 6 + offset) {
            if (strcmp(argv[5 + offset], "noreply") == 0) {
                noreply = true;
            } else {
                // Memcached 1.4.5 ignores invalid tokens here
                noreply = false;
            }
        } else {
            noreply = false;
        }

        /* Make a data provider */
        is_streaming = (value_size > MAX_BUFFERED_SET_SIZE);
        if (!is_streaming) {
            value_buffer = new char[value_size+2];   // +2 for the CRLF
            rh->conn->read_external(value_buffer, value_size+2, this);
        } else {
            data = new memcached_streaming_data_provider_t(rh, value_size);
            on_net_conn_read_external();   // Go ahead immediately; the value will be read later
        }
    }

    void on_net_conn_read_external() {

        /* We finished creating the data provider. */
        if (!is_streaming) {
            /* The buffered_data_provider_t doesn't check the CRLF, so we must check it here.
            The memcached_streaming_data_provider_t checks its own CRLF. */
            if (value_buffer[value_size] != '\r' || value_buffer[value_size + 1] != '\n') {
                rh->writef("CLIENT_ERROR bad data chunk\r\n");
                rh->read_next_command();
                delete this;
                return;
            }
            data = new buffered_data_provider_t(value_buffer, value_size);
            delete[] value_buffer;
        }

        /* Notify the memcached handler that we are starting a write request so that it
        will not let too many go at once */
        rh->begin_write_command(this);
    }

    void on_semaphore_available() {

        /* If we are streaming the value, then we cannot pipeline another command yet. */
        if (noreply && !is_streaming) nowait = true;
        else nowait = false;

        /* Store the request handler in case we get deleted */
        txt_memcached_handler_t *_rh = rh;
        bool _nowait = nowait;

        /* Dispatch the request now that we're sure that it parses properly. dispatch() is
        overriden by the subclass to do the appropriate thing. */
        dispatch(storage_command, &key, data, mcflags, exptime, req_cas);

        if (_nowait) _rh->read_next_command();
    }

    void on_net_conn_close() {

        /* Network connection closed while we read a buffered (non-streaming) value. */

        assert(!is_streaming);
        delete[] value_buffer;

        rh->read_next_command();   // The RH will figure out that the connection is closed
        delete this;
    }

    virtual void dispatch(storage_command_t storage_command,
            btree_key *key, data_provider_t *data,
            mcflags_t flags, exptime_t exptime, cas_t cas) = 0;

    ~txt_memcached_storage_request_t() {
        if (data) delete data;
    }

    /* Called by the subclass after the KVS calls it back */
    void done() {
        if (!nowait) rh->read_next_command();
        rh->end_write_command();
        delete this;
    }
};

struct txt_memcached_set_request_t :
    public txt_memcached_storage_request_t,
    public store_t::set_callback_t
{
    txt_memcached_set_request_t(
        txt_memcached_handler_t *rh,
        storage_command_t sc,
        int argc, char **argv)
    {
        init(rh, sc, argc, argv);
    }

    /* Called by the superclass when it's done parsing the value */
    void dispatch(storage_command_t sc,
            btree_key *key, data_provider_t *data,
            mcflags_t flags, exptime_t exptime, cas_t cas) {

        switch (sc) {
            case set_command:
                rh->server->store->set(key, data, flags, exptime, this);
                break;
            case add_command:
                rh->server->store->add(key, data, flags, exptime, this);
                break;
            case replace_command:
                rh->server->store->replace(key, data, flags, exptime, this);
                break;
            case cas_command:
                rh->server->store->cas(key, data, flags, exptime, cas, this);
                break;
            case append_command:
            case prepend_command:
            default:
                unreachable();
        }
    }

    void stored() {
        if (!noreply) rh->writef("STORED\r\n");
        done();
    }
    void not_stored() {
        if (!noreply) rh->writef("NOT_STORED\r\n");
        done();
    }
    void exists() {
        if (!noreply) rh->writef("EXISTS\r\n");
        done();
    }
    void not_found() {
        if (!noreply) rh->writef("NOT_FOUND\r\n");
        done();
    }
    void too_large() {
        if (!noreply) rh->writef("SERVER_ERROR object too large for cache\r\n");
        done();
    }
    void data_provider_failed() {
        // This is called if there was no CRLF at the end of the data block.
        /* Write BAD_BLOB even if we got a "noreply" because it's a syntax error in the
        command, not a result of the command. */
        rh->writef("CLIENT_ERROR bad data chunk\r\n");
        done();
    }
};

class txt_memcached_incr_decr_request_t :
    public semaphore_available_callback_t,
    public store_t::incr_decr_callback_t
{
    txt_memcached_handler_t *rh;
    bool increment;
    bool noreply;
    union {
        char key_memory[MAX_KEY_SIZE + sizeof(btree_key)];
        btree_key key;
    };
    unsigned long long delta;

public:
    txt_memcached_incr_decr_request_t(txt_memcached_handler_t *rh, bool i, int argc, char **argv)
        : rh(rh), increment(i)
    {
        /* cmd key delta [noreply] */
        if (argc != 3 && argc != 4) {
            rh->writef("ERROR\r\n");
            rh->read_next_command();
            delete this;
            return;
        }

        /* Parse key */
        if (!node_handler::str_to_key(argv[1], &key)) {
            rh->writef("CLIENT_ERROR bad command line format\r\n");
            rh->read_next_command();
            delete this;
            return;
        }

        /* Parse amount to change by */
        char *invalid_char;
        delta = strtoull_strict(argv[2], &invalid_char, 10);
        if (*invalid_char != '\0') {
            rh->writef("CLIENT_ERROR bad command line format\r\n");
            rh->read_next_command();
            delete this;
            return;
        }

        /* Parse "noreply" */
        if (argc == 4) {
            if (strcmp(argv[3], "noreply") == 0) {
                noreply = true;
            } else {
                // Memcached 1.4.5 ignores invalid tokens here
                noreply = false;
            }
        } else {
            noreply = false;
        }

        rh->begin_write_command(this);
    }

    void on_semaphore_available() {

        /* Store the request handler in case we get deleted */
        txt_memcached_handler_t *_rh = rh;
        bool _noreply = noreply;

        /* Dispatch the request */
        if (increment) rh->server->store->incr(&key, delta, this);
        else rh->server->store->decr(&key, delta, this);

        if (_noreply) _rh->read_next_command();
    }

    ~txt_memcached_incr_decr_request_t() {
    }
    
    void success(unsigned long long new_value) {
        if (!noreply) {
            rh->writef("%llu\r\n", new_value);
            rh->read_next_command();
        }
        rh->end_write_command();
        delete this;
    }
    void not_found() {
        if (!noreply) {
            rh->writef("NOT_FOUND\r\n");
            rh->read_next_command();
        }
        rh->end_write_command();
        delete this;
    }
    void not_numeric() {
        if (!noreply) {
            rh->writef("CLIENT_ERROR cannot increment or decrement non-numeric value\r\n");
            rh->read_next_command();
        }
        rh->end_write_command();
        delete this;
    }
};

struct txt_memcached_append_prepend_request_t :
    public txt_memcached_storage_request_t,
    public store_t::append_prepend_callback_t
{
    txt_memcached_append_prepend_request_t(
        txt_memcached_handler_t *rh,
        storage_command_t sc,
        int argc, char **argv)
    {
        init(rh, sc, argc, argv);
    }

    /* Called by the superclass when it's done parsing the value */
    void dispatch(storage_command_t sc,
            btree_key *key, data_provider_t *data,
            mcflags_t flags, exptime_t exptime, cas_t cas)
    {
        switch (sc) {
            case append_command:
                rh->server->store->append(key, data, this);
                break;
            case prepend_command:
                rh->server->store->prepend(key, data, this);
                break;
            case set_command:
            case add_command:
            case replace_command:
            case cas_command:
            default:
                unreachable();
        }
    }

    void success() {
        if (!noreply) rh->writef("STORED\r\n");
        done();
    }
    void too_large() {
        if (!noreply) rh->writef("NOT_STORED\r\n");
        done();
    }
    void not_found() {
        if (!noreply) rh->writef("NOT_STORED\r\n");
        done();
    }
    void data_provider_failed() {
        // See note in txt_memcached_set_request_t::data_provider_failed()
        rh->writef("CLIENT_ERROR bad data chunk\r\n");
        done();
    }
};

class txt_memcached_delete_request_t :
    public semaphore_available_callback_t,
    public store_t::delete_callback_t
{
    txt_memcached_handler_t *rh;
    bool noreply;
    union {
        char key_memory[MAX_KEY_SIZE + sizeof(btree_key)];
        btree_key key;
    };

public:
    txt_memcached_delete_request_t(txt_memcached_handler_t *rh, int argc, char **argv)
        : rh(rh), noreply(false)
    {
        /* "delete" key [a number] ["noreply"] */
        if (argc < 2 || argc > 4) {
            rh->writef("ERROR\r\n");
            rh->read_next_command();
            delete this;
            return;
        }

        /* Parse key */
        if (!node_handler::str_to_key(argv[1], &key)) {
            rh->writef("CLIENT_ERROR bad command line format\r\n");
            rh->read_next_command();
            delete this;
            return;
        }

        /* Parse "noreply" */
        if (argc > 2) {
            noreply = strcmp(argv[argc - 1], "noreply") == 0;
            bool zero = strcmp(argv[2], "0") == 0;
            bool valid = (argc == 3 && (zero || noreply))
                      || (argc == 4 && (zero && noreply));

            if (!valid) {
                if (!noreply) rh->writef("CLIENT_ERROR bad command line format.  Usage: delete <key> [noreply]\r\n");
                rh->read_next_command();
                delete this;
                return;
            }
        }

        rh->begin_write_command(this);
    }

    void on_semaphore_available() {

        /* Store the request handler in case we get deleted */
        txt_memcached_handler_t *_rh = rh;
        bool _noreply = noreply;

        /* Dispatch the request */
        rh->server->store->delete_key(&key, this);

        if (_noreply) _rh->read_next_command();
    }

    ~txt_memcached_delete_request_t() {}

    void deleted() {
        if (!noreply) {
            rh->writef("DELETED\r\n");
            rh->read_next_command();
        }
        rh->end_write_command();
        delete this;
    }
    void not_found() {
        if (!noreply) {
            rh->writef("NOT_FOUND\r\n");
            rh->read_next_command();
        }
        rh->end_write_command();
        delete this;
    }
};

class txt_memcached_perfmon_request_t :
    public perfmon_callback_t
{
    txt_memcached_handler_t *rh;
    perfmon_stats_t stats;
    std::vector<std::string> fields;
    
public:
    txt_memcached_perfmon_request_t(txt_memcached_handler_t *rh, int argc, char **argv)
        : rh(rh) {

        for (int i = 1; i < argc; i++) {
            fields.push_back(argv[i]);
        }

        if (perfmon_get_stats(&stats, this)) on_perfmon_stats();
    }

    void on_perfmon_stats() {
        if (fields.size() == 0) {
            for (perfmon_stats_t::iterator iter = stats.begin(); iter != stats.end(); iter++) {
                rh->writef("STAT %s %s\r\n", iter->first.c_str(), iter->second.c_str());
            }
        } else {
            for (int i = 0; i < (int)fields.size(); i++) {
                perfmon_stats_t::iterator stat_entry = stats.find(fields[i]);
                if (stat_entry == stats.end()) {
                    rh->writef("NOT FOUND\r\n");
                } else {
                    rh->writef("%s\r\n", stat_entry->second.c_str());
                }
            }
        }
        rh->writef("END\r\n");

        rh->read_next_command();
        delete this;
    }

private:
    DISABLE_COPYING(txt_memcached_perfmon_request_t);
};

class disable_gc_request_t :
    public server_t::all_gc_disabled_callback_t,  // gives us multiple_users_seen
    public home_thread_mixin_t
{
    txt_memcached_handler_t *rh;
    
public:
    explicit disable_gc_request_t(txt_memcached_handler_t *rh)
        : rh(rh) {
        rh->server->disable_gc(this);
    }

    void on_gc_disabled() {
        assert_thread();
        // TODO: Figure out what to print here
        rh->writef("DISABLED%s\r\n", multiple_users_seen ? " (Warning: multiple users think they're the one who disabled the gc.)" : "");
        rh->read_next_command();
        delete this;
    }

private:
    DISABLE_COPYING(disable_gc_request_t);
};

class enable_gc_request_t :
    public server_t::all_gc_enabled_callback_t,  // gives us multiple_users_seen
    public home_thread_mixin_t
{
    txt_memcached_handler_t *rh;
    
public:
    explicit enable_gc_request_t(txt_memcached_handler_t *rh)
        : rh(rh) {
        rh->server->enable_gc(this);
    }

    void on_gc_enabled() {
        assert_thread();
        // TODO: figure out what to print here.
        rh->writef("ENABLED%s\r\n", multiple_users_seen ? " (Warning: gc was already enabled.)" : "");
        rh->read_next_command();
        delete this;
    }

private:
    DISABLE_COPYING(enable_gc_request_t);
};

perfmon_duration_sampler_t
    pm_conns_reading("conns_reading", secs_to_ticks(1)),
    pm_conns_writing("conns_writing", secs_to_ticks(1)),
    pm_conns_acting("conns_acting", secs_to_ticks(1));

txt_memcached_handler_t::txt_memcached_handler_t(conn_acceptor_t *acc, net_conn_t *conn, server_t *server)
    : conn_handler_t(acc, conn),
      conn(conn),
      server(server),
      send_buffer(conn),
      requests_out_sem(MAX_CONCURRENT_QUERIES_PER_CONNECTION),
      shutting_down(false)
{
    logINF("Opened connection %p\n", this);
    pm_conns_acting.begin(&start_time);
    read_next_command();
}

txt_memcached_handler_t::~txt_memcached_handler_t() {
    logINF("Closed connection %p\n", this);

    if (conn->is_write_open()) conn->shutdown_write();
    assert(!conn->is_read_open());
}

void txt_memcached_handler_t::write(const char *buffer, size_t bytes) {
    send_buffer.write(bytes, buffer);
}

void txt_memcached_handler_t::writef(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[1000];
    size_t bytes = vsnprintf(buffer, sizeof(buffer), format, args);
    assert(bytes < sizeof(buffer));
    send_buffer.write(bytes, buffer);
    va_end(args);
}

void txt_memcached_handler_t::quit() {

    /* Any pending read operations will be interrupted. If the read operation is in *_request_t,
    then it will eventually call read_next_command(), which will see that the net_conn_t is closed,
    close it for writing, and then delete us. If the read operation is in us, then we will close
    for writing and delete ourself immediately. */
    if (conn->is_read_open()) conn->shutdown_read();
}

void txt_memcached_handler_t::shut_down() {

    /* This is called after the connection has closed. */
    assert_thread();
    assert(!conn->is_read_open());

    /* Acquire the lock in exclusive mode to wait until all requests are done */
    shutting_down = true;
    if (prevent_shutdown_lock.lock(rwi_write, this)) on_lock_available();
}

void txt_memcached_handler_t::read_next_command() {

    pm_conns_acting.end(&start_time);

    /* Prevent arbitrarily deep stacks when reading many commands that come quickly */
    call_later_on_this_thread(this);
}

void txt_memcached_handler_t::on_thread_switch() {

    /* If !conn->is_read_open(), then the socket died during a read or write done by a *_request_t,
    and that's why we didn't find out about it immediately. */
    if (!conn->is_read_open()) {
        shut_down();

    } else {
        /* Before we read another command off the socket, we must make sure that there isn't
        any data in the send buffer that we should flush first. */
        pm_conns_writing.begin(&start_time);
        if (conn->is_write_open()) {
            send_buffer.flush(this);
        } else {
            /* The peer is not letting us write to the socket for some reason. Usually this
            means that the connection was closed, but there is still some data in the incoming
            buffer. We don't want to abort because then we couldn't process the data in the
            incoming buffer, so instead we skip the writing step. */
            send_buffer.discard();
            on_send_buffer_flush();
        }
    }
}

void txt_memcached_handler_t::on_lock_available() {

    /* We just acquired the prevent_shutdown_lock in exclusive mode, which means there
    are no outstanding write requests that refer to request_out_sem and we can safely
    delete ourself now */

    assert(shutting_down);
    delete this;
}

void txt_memcached_handler_t::on_send_buffer_flush() {

    /* Now that we're sure that the send buffer is empty, start reading a new line off of
    the socket. */
    pm_conns_writing.end(&start_time);
    pm_conns_reading.begin(&start_time);
    conn->read_buffered(this);
}

void txt_memcached_handler_t::on_send_buffer_socket_closed() {

    /* Socket write half closed as we were flushing the send buffer. We ignore the error and keep
    going; we don't actually shut down until the read half has been shut down. */

    on_send_buffer_flush();
}

void txt_memcached_handler_t::on_net_conn_read_buffered(const char *buffer, size_t size) {

    void *crlf_loc = memmem(buffer, size, "\r\n", 2);
    if (!crlf_loc) {
        // There is not a complete line on the socket yet
        
        /* If a horribly malfunctioning client sends a lot of data without sending a CRLF, our
        buffer will just grow and grow and grow. If this happens, then close the connection.
        (This doesn't apply to large values because they are read from the socket via a
        different mechanism.) Is there a better way to handle this situation? */
        size_t threshold = MEGABYTE;
        if (size > threshold) {
            logERR("Aborting connection %p because we got more than %d bytes without a CRLF\n",
                this, (int)threshold);
            conn->accept_buffer(0);   // So that we can legally call shutdown_*()
            conn->shutdown_read();
            shut_down();
            return;
        
        } else {
            /* If we return from on_net_conn_read_buffered() without calling accept_buffer(),
            then the net_conn_t will call on_net_conn_read_buffered() again with a bigger buffer
            when the data is available. This way we keep reading until we get a complete line. */
            return;
        }
    }

    pm_conns_reading.end(&start_time);
    pm_conns_acting.begin(&start_time);

    /* We must duplicate the line because accept_buffer() may invalidate "buffer" */
    int line_size = (char*)crlf_loc - buffer + 2;
    std::vector<char> line_storage;   // Easy way to make sure 'line' gets freed when we exit this function
    line_storage.resize(line_size);
    memcpy(line_storage.data(), buffer, line_size);
    conn->accept_buffer(line_size);
    line_storage.push_back('\0');   // Null terminator
    char *line = line_storage.data();

    /* Tokenize the line */
    std::vector<char *> args;
    char *l = line, *state = NULL;
    while (char *cmd_str = strtok_r(l, " \r\n\t", &state)) {
        args.push_back(cmd_str);
        l = NULL;
    }

    if (args.size() == 0) {
        writef("ERROR\r\n");
        read_next_command();
        return;
    }

    // Execute command
    if(!strcmp(args[0], "get")) {    // check for retrieval commands
        new txt_memcached_get_request_t(this, false, args.size(), args.data());
    } else if(!strcmp(args[0], "gets")) {
        new txt_memcached_get_request_t(this, true, args.size(), args.data());
    } else if(!strcmp(args[0], "set")) {     // check for storage commands
        new txt_memcached_set_request_t(this, txt_memcached_storage_request_t::set_command, args.size(), args.data());
    } else if(!strcmp(args[0], "add")) {
        new txt_memcached_set_request_t(this, txt_memcached_storage_request_t::add_command, args.size(), args.data());
    } else if(!strcmp(args[0], "replace")) {
        new txt_memcached_set_request_t(this, txt_memcached_storage_request_t::replace_command, args.size(), args.data());
    } else if(!strcmp(args[0], "append")) {
        new txt_memcached_append_prepend_request_t(this, txt_memcached_storage_request_t::append_command, args.size(), args.data());
    } else if(!strcmp(args[0], "prepend")) {
        new txt_memcached_append_prepend_request_t(this, txt_memcached_storage_request_t::prepend_command, args.size(), args.data());
    } else if(!strcmp(args[0], "cas")) {
        new txt_memcached_set_request_t(this, txt_memcached_storage_request_t::cas_command, args.size(), args.data());
    } else if(!strcmp(args[0], "delete")) {
        new txt_memcached_delete_request_t(this, args.size(), args.data());
    } else if(!strcmp(args[0], "incr")) {
        new txt_memcached_incr_decr_request_t(this, true, args.size(), args.data());
    } else if(!strcmp(args[0], "decr")) {
        new txt_memcached_incr_decr_request_t(this, false, args.size(), args.data());
    } else if(!strcmp(args[0], "quit")) {
        // Make sure there's no more tokens
        if (args.size() > 1) {
            writef("ERROR\r\n");
            read_next_command();
            return;
        }
        // Quit the connection
        conn->shutdown_read();
        shut_down();
    } else if(!strcmp(args[0], "stats") || !strcmp(args[0], "stat")) {
        new txt_memcached_perfmon_request_t(this, args.size(), args.data());
    } else if(!strcmp(args[0], "rethinkdb")) {
        if (args.size() == 2 && !strcmp(args[1], "shutdown")) {
            // Shut down the server
            server->shutdown();
            conn->shutdown_read();
            shut_down();
        // TODO: `rethinkdb gc` crashes the server, and isn't very important, so it's comented out for now.
        //} else if (args.size() == 3 && strcmp(args[1], "gc") == 0 && strcmp(args[2], "enable") == 0) {
        //    new enable_gc_request_t(this);
        //} else if (args.size() == 3 && strcmp(args[1], "gc") == 0 && strcmp(args[2], "disable") == 0) {
        //    new disable_gc_request_t(this);
        } else {
            writef("Available commands:\r\n");
            writef("rethinkdb shutdown\r\n");
            //writef("rethinkdb gc enable\r\n");
            //writef("rethinkdb gc disable\r\n");
            read_next_command();
        }
    } else if(!strcmp(args[0], "version")) {
        if (args.size() == 2) {
            writef("VERSION rethinkdb-%s\r\n", RETHINKDB_VERSION);
        } else {
            writef("ERROR\r\n");
        }
        read_next_command();
    } else {
        writef("ERROR\r\n");
        read_next_command();
    }
}

void txt_memcached_handler_t::on_net_conn_close() {

    /* The connection was closed as we were waiting for a complete line to buffer */

    pm_conns_reading.end(&start_time);
    shut_down();
}

void txt_memcached_handler_t::begin_write_command(semaphore_available_callback_t *cb) {

    /* Called whenever a write command begins to make sure that not too many
     * write commands are being dispatched to the key-value store. */

    /* This should only be called when a command is actually sent to the
     * key-value store ie it should not be called if the command fails in the
     * parsing stage (Issue 153)*/

    assert_thread();
    assert(!shutting_down);

    bool locked __attribute__((unused)) = prevent_shutdown_lock.lock(rwi_read, NULL);
    assert(locked);

    requests_out_sem.lock(cb);
}

void txt_memcached_handler_t::end_write_command() {

    /* Called once for each call to begin_write_command() */

    assert_thread();

    prevent_shutdown_lock.unlock();
    requests_out_sem.unlock();
}

