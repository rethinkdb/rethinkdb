#ifndef __REPLICATION_MESSAGES_HPP__
#define __REPLICATION_MESSAGES_HPP__

#include "replication/net_structs.hpp"

#include "btree/key_value_store.hpp"
#include "replication/value_stream.hpp"

namespace replication {

enum role_t { master = 0, new_slave = 1, slave = 2 };

class hello_message_t {
public:
    hello_message_t(role_t role, database_magic_t database_magic, const char *name_beg, const char *name_end)
        : role_(role), database_magic_(database_magic), informal_name_(name_beg, name_end) { }

    role_t role() const { return role_; }
    database_magic_t database_magic() const { return database_magic_; }
    const std::string& informal_name() const { return informal_name_; }
private:
    role_t role_;
    database_magic_t database_magic_;
    std::string informal_name_;
    DISABLE_COPYING(hello_message_t);
};

template <class struct_type>
class timestamped_message_t {
public:
    typedef struct_type net_struct_type;
    repli_timestamp timestamp() const { return timestamp_; }
protected:
    timestamped_message_t(const net_struct_type *p) : timestamp_(p->timestamp) { }
private:
    repli_timestamp timestamp_;
    DISABLE_COPYING(timestamped_message_t);
};

class backfill_message_t : public timestamped_message_t<net_backfill_t> {
public:
    backfill_message_t(const net_backfill_t *p) : timestamped_message_t<net_backfill_t>(p) { }
};

class announce_message_t {
public:
    typedef net_announce_t net_struct_type;
    announce_message_t(const net_announce_t *p) : from_(p->from), to_(p->to) { }
    repli_timestamp from() const { return from_; }
    repli_timestamp to() const { return to_; }
private:
    repli_timestamp from_;
    repli_timestamp to_;
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
        : timestamp_(p->timestamp),
          key_(p->key()->contents, p->key()->contents + p->key()->size),
          flags_(p->value()->mcflags()),
          exptime_(p->value()->exptime()),
          has_cas_(p->value()->has_cas()),
          cas_(has_cas_ ? p->value()->cas() : 0),
          value_(new value_stream_t()) {
        const btree_value *val = p->value();
        const char *valbytes = val->value();
        write_charslice(value_, const_charslice(valbytes, valbytes + val->value_size()));
    }

    set_message_t(const net_large_set_t *p, value_stream_t *stream)
        : timestamp_(p->op_header.timestamp),
          key_(p->key()->contents, p->key()->contents + p->key()->size),
          flags_(metadata_memcached_flags(p->metadata_flags, p->metadata())),
          exptime_(metadata_memcached_exptime(p->metadata_flags, p->metadata())),
          value_(stream) {
        has_cas_ = metadata_memcached_cas(p->metadata_flags, p->metadata(), &cas_);
    }

    repli_timestamp timestamp() const { return timestamp_; }
    const std::string& key() const { return key_; }
    mcflags_t flags() const { return flags_; }
    exptime_t exptime() const { return exptime_; }
    bool has_cas() const { return has_cas_; }
    cas_t cas() const { return cas_; }
    value_stream_t *value() { return value_; }

private:
    repli_timestamp timestamp_;
    std::string key_;
    mcflags_t flags_;
    exptime_t exptime_;
    bool has_cas_;
    cas_t cas_;

    value_stream_t *value_;
};

class append_prepend_message_t {
public:
    typedef net_small_append_prepend_t net_struct_type;
    typedef net_large_append_prepend_t first_struct_type;
protected:
    append_prepend_message_t(const net_small_append_prepend_t *p)
        : timestamp_(p->timestamp),
          key_(p->key()->contents, p->key()->contents + p->key()->size),
          value_(new value_stream_t()) {
        write_charslice(value_, const_charslice(p->value_beg(), p->value_end()));
    }

    append_prepend_message_t(const net_large_append_prepend_t *p, value_stream_t *stream)
        : timestamp_(p->op_header.timestamp),
          key_(p->key()->contents, p->key()->contents + p->key()->size),
          value_(stream) { }

    repli_timestamp timestamp() const { return timestamp_; }
    const std::string& key() const { return key_; }
    value_stream_t *value() { return value_; }

private:
    repli_timestamp timestamp_;
    std::string key_;
    value_stream_t *value_;
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
