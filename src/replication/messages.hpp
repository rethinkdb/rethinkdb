#ifndef __REPLICATION_MESSAGES_HPP__
#define __REPLICATION_MESSAGES_HPP__

#include "btree/serializer_config_block.hpp"
#include "replication/net_structs.hpp"
#include "replication/value_stream.hpp"

namespace replication {

enum role_t { master = 0, new_slave = 1, slave = 2 };

// Why are we using crazy encapsulated classes again?  Seriously, maybe we should just use structs.

class hello_message_t {
public:
    hello_message_t(role_t role_, database_magic_t database_magic_, const char *name_beg_, const char *name_end_)
        : role(role_), database_magic(database_magic_), informal_name(name_beg_, name_end_) { }

    const role_t role;
    const database_magic_t database_magic;
    const std::string informal_name;

private:
    DISABLE_COPYING(hello_message_t);
};

template <class struct_type>
class timestamped_message_t {
public:
    typedef struct_type net_struct_type;

    const repli_timestamp timestamp;
protected:
    timestamped_message_t(const net_struct_type *p) : timestamp(p->timestamp) { }

private:
    DISABLE_COPYING(timestamped_message_t);
};

class backfill_message_t : public timestamped_message_t<net_backfill_t> {
public:
    backfill_message_t(const net_backfill_t *p) : timestamped_message_t<net_backfill_t>(p) { }
};

class announce_message_t {
public:
    typedef net_announce_t net_struct_type;
    announce_message_t(const net_announce_t *p) : from(p->from), to(p->to) { }

    const repli_timestamp from;
    const repli_timestamp to;
private:
    DISABLE_COPYING(announce_message_t);
};

class nop_message_t : public timestamped_message_t<net_nop_t> {
public:
    nop_message_t(const net_nop_t *p) : timestamped_message_t<net_nop_t>(p) { }
};

class ack_message_t : public timestamped_message_t<net_ack_t> {
public:
    ack_message_t(const net_ack_t *p) : timestamped_message_t<net_ack_t>(p) { }
};

class shutting_down_message_t : public timestamped_message_t<net_shutting_down_t> {
public:
    shutting_down_message_t(const net_shutting_down_t *p) : timestamped_message_t<net_shutting_down_t>(p) { }
};

class goodbye_message_t : public timestamped_message_t<net_goodbye_t> {
public:
    goodbye_message_t(const net_goodbye_t *p) : timestamped_message_t<net_goodbye_t>(p) { }
};

class set_message_t {
public:
    typedef net_small_set_t net_struct_type;
    typedef net_large_set_t first_struct_type;
    set_message_t(const net_small_set_t *p)
        : timestamp(p->timestamp),
          key(p->key()->contents, p->key()->contents + p->key()->size),
          flags(p->value()->mcflags()),
          exptime(p->value()->exptime()),
          has_cas(p->value()->has_cas()),
          cas(has_cas ? p->value()->cas() : 0),
          value(new value_stream_t()) {
        const btree_value *val = p->value();
        const char *valbytes = val->value();
        write_charslice(value, const_charslice(valbytes, valbytes + val->value_size()));
    }

    set_message_t(const net_large_set_t *p, value_stream_t *stream)
        : timestamp(p->op_header.timestamp),
          key(p->key()->contents, p->key()->contents + p->key()->size),
          flags(metadata_memcached_flags(p->metadata_flags, p->metadata())),
          exptime(metadata_memcached_exptime(p->metadata_flags, p->metadata())),
          has_cas(false),
          cas(0),
          value(stream) {
        const_cast<bool&>(has_cas) = metadata_memcached_cas(p->metadata_flags, p->metadata(), const_cast<cas_t *>(&cas));
    }

    const repli_timestamp timestamp;
    const std::string key;
    const mcflags_t flags;
    const exptime_t exptime;
    const bool has_cas;
    const cas_t cas;

    value_stream_t * const value;
};

class append_prepend_message_t {
public:
    typedef net_small_append_prepend_t net_struct_type;
    typedef net_large_append_prepend_t first_struct_type;
protected:
    append_prepend_message_t(const net_small_append_prepend_t *p)
        : timestamp(p->timestamp),
          key(p->key()->contents, p->key()->contents + p->key()->size),
          value(new value_stream_t()) {
        write_charslice(value, const_charslice(p->value_beg(), p->value_end()));
    }

    append_prepend_message_t(const net_large_append_prepend_t *p, value_stream_t *stream)
        : timestamp(p->op_header.timestamp),
          key(p->key()->contents, p->key()->contents + p->key()->size),
          value(stream) { }

    const repli_timestamp timestamp;
    const std::string key;
    value_stream_t * const value;
private:
    DISABLE_COPYING(append_prepend_message_t);
};

class append_message_t : public append_prepend_message_t {
public:
    append_message_t(const net_small_append_prepend_t *p) : append_prepend_message_t(p) { }
};

class prepend_message_t : public append_prepend_message_t {
public:
    prepend_message_t(const net_small_append_prepend_t *p) : append_prepend_message_t(p) { }
};






}  // namespace replication



#endif  // __REPLICATION_MESSAGES_HPP__
