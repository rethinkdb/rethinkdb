// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TERM_STORAGE_HPP_
#define RDB_PROTOCOL_TERM_STORAGE_HPP_

#include <map>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "rapidjson/rapidjson.h"
#include "rdb_protocol/rdb_backtrace.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {

const char *rapidjson_typestr(rapidjson::Type t);

struct generated_term_t;

typedef boost::variant<const rapidjson::Value *, counted_t<generated_term_t> >
    term_variant_t;

struct generated_term_t : public slow_atomic_countable_t<generated_term_t> {
    generated_term_t(Term::TermType _type, backtrace_id_t _bt);

    const Term::TermType type;

    // args and optargs are only valid if type != DATUM
    std::vector<term_variant_t> args;
    std::map<std::string, term_variant_t> optargs;

    // datum is only valid if type != DATUM
    datum_t datum;
    const backtrace_id_t bt;
};

// A `raw_term_t` is an iterator through the original term tree received from the client.
// This allows us to walk the structure without converting it into an intermediary
// format.
class raw_term_t {
public:
    explicit raw_term_t(const term_variant_t &source);
    raw_term_t(const raw_term_t &) = default;

    size_t num_args() const;
    size_t num_optargs() const;
    raw_term_t arg(size_t index) const;
    boost::optional<raw_term_t> optarg(const std::string &name) const;

    template <typename callable_t>
    void each_optarg(callable_t &&cb) const {
        visit_source(
            [&](const json_data_t &source) {
                if (source.optargs != nullptr) {
                    for (auto it = source.optargs->MemberBegin();
                         it != source.optargs->MemberEnd(); ++it) {
                        cb(raw_term_t(&it->value), it->name.GetString());
                    }
                }
            },
            [&](const counted_t<generated_term_t> &source) {
                for (const auto &it : source->optargs) {
                    cb(raw_term_t(it.second), it.first);
                }
            });
    }

    // This parses the datum each time it is called - keep calls to a minimum
    datum_t datum(const configured_limits_t &limits, reql_version_t version) const;

    // Parses the datum using the latest version and with no limits, this should
    // generally only be used during or before term compilation.
    datum_t datum() const;

    Term::TermType type() const;
    backtrace_id_t bt() const;
    term_variant_t get_src() const;

private:
    raw_term_t();
    void init_json(const rapidjson::Value *src);

    struct json_data_t {
        const rapidjson::Value *source;
        const rapidjson::Value *args;
        const rapidjson::Value *optargs;
        const rapidjson::Value *datum;
    };

    boost::variant<json_data_t, counted_t<generated_term_t> > info;

    template <typename json_cb_t, typename generated_cb_t>
    class source_visitor_t : public boost::static_visitor<void> {
    public:
        source_visitor_t(json_cb_t &&_json_cb, generated_cb_t &&_generated_cb) :
            json_cb(std::move(_json_cb)), generated_cb(std::move(_generated_cb)) { }
        void operator() (const json_data_t &source) {
            json_cb(source);
        }
        void operator() (const counted_t<generated_term_t> &source) {
            generated_cb(source);
        }
        json_cb_t json_cb;
        generated_cb_t generated_cb;
    };

    template <typename json_cb_t, typename generated_cb_t>
    void visit_source(json_cb_t &&json_cb, generated_cb_t &&generated_cb) const {
        source_visitor_t<json_cb_t, generated_cb_t> visitor(std::move(json_cb),
                                                            std::move(generated_cb));
        boost::apply_visitor(visitor, info);
    }
};

class term_storage_t {
public:
    virtual ~term_storage_t() { }

    const backtrace_registry_t &backtrace_registry() const;

    // These functions must be implemented by descendants
    virtual raw_term_t root_term() const = 0;

    // These functions are not valid for all descendants
    virtual Query::QueryType query_type() const;
    virtual bool static_optarg_as_bool(const std::string &key,
                                       bool default_value) const;
    virtual void preprocess();
    virtual global_optargs_t global_optargs();

protected:
    backtrace_registry_t bt_reg;
};

class json_term_storage_t : public term_storage_t {
public:
    json_term_storage_t(scoped_array_t<char> &&_original_data,
                        rapidjson::Document &&_query_json);
    Query::QueryType query_type() const;
    bool static_optarg_as_bool(const std::string &key,
                               bool default_value) const;
    void preprocess();
    raw_term_t root_term() const;
    global_optargs_t global_optargs();
private:
    scoped_array_t<char> original_data;
    rapidjson::Document query_json;
};

class wire_term_storage_t : public term_storage_t {
public:
    wire_term_storage_t(scoped_array_t<char> &&_original_data,
                        rapidjson::Document &&_query_json);
    raw_term_t root_term() const;
private:
    scoped_array_t<char> original_data;
    rapidjson::Document func_json;
};

template <typename pb_t>
MUST_USE archive_result_t deserialize_protobuf(read_stream_t *s, pb_t *pb_out);

template <cluster_version_t W>
void serialize_term_tree(write_message_t *wm,
                         const raw_term_t &root_term);

template <cluster_version_t W>
MUST_USE archive_result_t deserialize_term_tree(read_stream_t *s,
    scoped_ptr_t<term_storage_t> *term_storage_out);

} // namespace ql

#endif // RDB_PROTOCOL_TERM_STORAGE_HPP_
