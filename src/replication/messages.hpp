#ifndef __REPLICATION_MESSAGES_HPP__
#define __REPLICATION_MESSAGES_HPP__

namespace replication {

class hello_message_t {
public:
    hello_message_t(role_t role, database_magic_t database_magic, char *name_beg, char *name_end)
        : replication_protocol_version_(replication_protocol_version_), role_(role),
          database_magic_(database_magic), informal_name_(name_beg, name_end) { }

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
    repl_timestamp_t timestamp() const { return timestamp_; }
protected:
    timestamped_message_t(const net_struct_type *p) : timestamp_(p->timestamp) { }
private:
    repl_timestamp_t timestamp_;
    DISABLE_COPYING(timestamped_message_t);
};

class backfill_message_t : private timestamped_message_t<net_backfill_t> {
public:
    backfill_message_t(const net_backfill_t *p) : timestamped_message_t(p) { }
};

class backfilling_message_t {
public:
    typedef net_backfilling_t net_struct_type;
    backfilling_message_t(const net_backfilling_t *p) : from_(p->from), to_(p->to) { }
    repl_timestamp_t from() const { return from_; }
    repl_timestamp_t to() const { return to_; }
private:
    repl_timestamp_t from;
    repl_timestamp_t to;
    DISABLE_COPYING(backfilling_message_t);
};

class nop_message_t : private timestamped_message_t<net_nop_t> {
public:
    nop_message_t(const net_nop_t *p) : timestamped_message_t(p) { }
};

class ack_message_t : private timestamped_message_t<net_ack_t> {
public:
    ack_message_t(const net_ack_t *p) : timestamped_message_t(p) { }
};

class shutting_down_message_t : private timestamped_message_t<net_shutting_down_t> {
public:
    shutting_down_message_t(const net_shutting_down_t *p) : timestamped_message_t(p) { }
};

class goodbye_message_t : private timestamped_message_t<net_goodbye_t> {
public:
    goodbye_message_t(const net_goodbye_t *p) : timestamped_message_t(p) { }
};

class set_message_t {
public:
    typedef net_small_set_t net_struct_type;
    set_message_t(const net_small_set_t *p)
        : timestamp_(p->timestamp),
          key_(p->key().contents, p->key().contents + p->key().size),
          flags_(p->value()->mcflags()),
          exptime_(p->value()->exptime()),
          has_cas_(p->value()->has_cas()),
          cas_(has_cas_ ? p->value()->cas() : 0),
          value_(value_stream_t::copy(p->value()->value(), p->value()->value() + p->value()->value_size())) { }
    set_message_t(const net_large_set_t *p, value_stream_t *stream)
        : timestamp_(p->op_header.timestamp),
          key_(p->key().contents, p->key().contents + p->key().size),
          flags_(p->value()->mcflags()),
          exptime_(p->value()->exptime()),
          has_cas_(p->value()->has_cas()),
          cas_(has_cas_ ? p->value()->cas() : 0),
          value_(stream) { }

    repl_timestamp_t timestamp() const { return timestamp_; }
    const std::string& key() const { return key_; }
    mcflags_t flags() const { return flags_; }
    exptime_t exptime() const { return exptime_; }
    bool has_cas() const { return has_cas_; }
    cas_t cas() const { return cas_; }
    value_stream_t *value() { return value_; }

private:
    repl_timestamp_t timestamp_;
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
protected:
    append_prepend_message_t(const net_small_append_prepend_t *p)
        : timestamp_(p->timestamp),
          key_(p->key().contents, p->key().contents + p->key().size),
          value_(value_stream_t::copy(p->value_beg(), p->value_end())) { }
    append_prepend_message_t(const net_large_append_prepend_t *p, value_stream_t *stream)
        : timestamp_(p->op_header.timestamp),
          key_(p->key().contents, p->key().contents + p->key().size),
          value_(stream) { }

    repl_timestamp_t timestamp() const { return timestamp_; }
    const std::string& key() const { return key_; }
    value_stream_t *value() { return value_; }

private:
    repl_timestamp_t timestamp_;
    std::string key_;
    value_stream_t *value_;
};

class append_message_t : private append_prepend_message_t {
public:
    append_message_t(const net_small_append_prepend_t *p) : append_prepend_message_t(p) { }
};

class prepend_message_t : private append_prepend_message_t {
public:
    append_prepend_message_t(const net_small_append_prepend_t *p) : append_prepend_message_t(p) { }
};






}  // namespace replication



#endif  // __REPLICATION_MESSAGES_HPP__
