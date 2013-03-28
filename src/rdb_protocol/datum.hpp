#ifndef RDB_PROTOCOL_DATUM_HPP_
#define RDB_PROTOCOL_DATUM_HPP_
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "utils.hpp"
#include <boost/shared_ptr.hpp>

#include "btree/keys.hpp"
#include "containers/archive/archive.hpp"
#include "containers/ptr_bag.hpp"
#include "http/json.hpp"
#include "rdb_protocol/err.hpp"

class Datum;

namespace ql {
class datum_stream_t;
class env_t;
class val_t;

// These let us write e.g. `foo(NOTHROW) instead of `foo(false/*nothrow*/)`.
// They should be passed to functions that have multiple behaviors (like `el` or
// `add` below).

// NOTHROW: Return NULL
// THROW: Throw an error
enum throw_bool_t { NOTHROW = 0, THROW = 1};

// NOCLOBBER: Don't overwrite existing values.
// CLOBBER: Overwrite existing values.
enum clobber_bool_t { NOCLOBBER = 0, CLOBBER = 1};

// A `datum_t` is basically a JSON value, although we may extend it later.
// TODO: When we optimize for memory, this needs to stop inheriting from `rcheckable_t`
class datum_t : public ptr_baggable_t, public rcheckable_t {
public:
    // This ordering is important, because we use it to sort objects of
    // disparate type.  It should be alphabetical.
    enum type_t { R_ARRAY = 1, R_BOOL = 2, R_NULL = 3,
                  R_NUM = 4, R_OBJECT = 5, R_STR = 6 };
    explicit datum_t(type_t _type);

    // These allow you to construct a datum from the type of value it
    // represents.  We have some gotchya-constructors to scare away implicit
    // conversions.
    explicit datum_t(bool) = delete;
    explicit datum_t(int) = delete;
    explicit datum_t(float) = delete;
    // Need to explicitly ask to construct a bool.
    datum_t(type_t _type, bool _bool);
    explicit datum_t(double _num);
    explicit datum_t(const std::string &_str);
    explicit datum_t(const char *cstr);
    explicit datum_t(const std::vector<const datum_t *> &_array);
    explicit datum_t(const std::map<std::string, const datum_t *> &_object);

    // These construct a datum from an equivalent representation.
    explicit datum_t(const Datum *d); // Undefined, need to pass `env` below.
    datum_t(const Datum *d, env_t *env);
    explicit datum_t(cJSON *json);
    datum_t(cJSON *json, env_t *env);
    explicit datum_t(const boost::shared_ptr<scoped_cJSON_t> &json);
    datum_t(const boost::shared_ptr<scoped_cJSON_t> &json, env_t *env);

    void write_to_protobuf(Datum *out) const;

    type_t get_type() const;
    const char *get_type_name() const;
    std::string print() const;
    std::string print_primary() const;
    std::string print_secondary(const store_key_t &key) const;
    void check_type(type_t desired) const;

    bool as_bool() const;
    double as_num() const;
    int64_t as_int() const;
    const std::string &as_str() const;

    // Use of `size` and `el` is preferred to `as_array` when possible.
    const std::vector<const datum_t *> &as_array() const;
    void add(const datum_t *val); // add to an array
    size_t size() const;
    // Access an element of an array.
    const datum_t *el(size_t index, throw_bool_t throw_bool = THROW) const;

    // Use of `el` is preferred to `as_object` when possible.
    const std::map<std::string, const datum_t *> &as_object() const;
    // Returns true if `key` was already in object.
    MUST_USE bool add(const std::string &key, const datum_t *val,
                      clobber_bool_t clobber_bool = NOCLOBBER); // add to an object
    // Returns true if key was in object.
    MUST_USE bool delete_key(const std::string &key);
    // Access an element of an object.
    const datum_t *el(const std::string &key, throw_bool_t throw_bool = THROW) const;
    const datum_t *merge(const datum_t *rhs) const;
    typedef const datum_t *(*merge_res_f)(env_t *env, const std::string &key,
                                          const datum_t *l, const datum_t *r,
                                          const rcheckable_t *caller);
    const datum_t *merge(env_t *env, const datum_t *rhs, merge_res_f f) const;

    cJSON *as_raw_json() const;
    boost::shared_ptr<scoped_cJSON_t> as_json() const;
    datum_stream_t *as_datum_stream(env_t *env, const pb_rcheckable_t *bt_src) const;

    // These behave as expected and defined in RQL.  Theoretically, two data of
    // the same type should compare the same way their printed representations
    // would compare lexicographcally, while dispareate types are compared
    // alphabetically by type name.
    int cmp(const datum_t &rhs) const;
    bool operator==(const datum_t &rhs) const;
    bool operator!=(const datum_t &rhs) const;

    bool operator<(const datum_t &rhs) const;
    bool operator<=(const datum_t &rhs) const;
    bool operator>(const datum_t &rhs) const;
    bool operator>=(const datum_t &rhs) const;

    // Iterate through an object or array with a callback.  (The callback
    // returns whether or not to continue iterating.)  Used for e.g. garbage
    // collection.
    void iter(bool (*callback)(const datum_t *, env_t *), env_t *env) const;

    virtual void runtime_check(const char *test, const char *file, int line,
                               bool pred, std::string msg) const {
        ql::runtime_check(test, file, line, pred, msg);
    }
private:
    void init_json(cJSON *json, env_t *env);

    // TODO: fix later.  Listing everything is more debugging-friendly than a
    // boost::variant, but less efficient.
    type_t type;
    bool r_bool;
    double r_num;
    std::string r_str;
    std::vector<const datum_t *> r_array;
    std::map<std::string, const datum_t *> r_object;
};

RDB_DECLARE_SERIALIZABLE(Datum);
// A `wire_datum_t` is necessary to serialize data over the wire.  See README.md
// for more info.
class wire_datum_t {
public:
    wire_datum_t() : ptr(0), state(INVALID) { }
    explicit wire_datum_t(const datum_t *_ptr) : ptr(_ptr), state(COMPILED) { }
    const datum_t *get() const;
    const datum_t *reset(const datum_t *ptr2);
    const datum_t *compile(env_t *env);

    void finalize();
private:
    const datum_t *ptr;
    Datum ptr_pb;

public:
    friend class write_message_t;
    void rdb_serialize(write_message_t &msg /* NOLINT */) const {
        r_sanity_check(state == READY_TO_WRITE);
        msg << ptr_pb;
    }
    friend class archive_deserializer_t;
    archive_result_t rdb_deserialize(read_stream_t *s) {
        archive_result_t res = deserialize(s, &ptr_pb);
        if (res) return res;
        state = JUST_READ;
        return ARCHIVE_SUCCESS;
    }

private:
    enum { INVALID, JUST_READ, COMPILED, READY_TO_WRITE } state;
};

// This is like a `wire_datum_t` but for gmr.  We need it because gmr allows
// non-strings as keys, while the data model we pinched from JSON doesn't.  See
// README.md for more info.
class wire_datum_map_t {
public:
    wire_datum_map_t() : state(COMPILED) { }
    bool has(const datum_t *key);
    const datum_t *get(const datum_t *key);
    void set(const datum_t *key, const datum_t *val);

    void compile(env_t *env);
    void finalize();

    const datum_t *to_arr(env_t *env) const;
private:
    struct datum_value_compare_t {
        bool operator()(const datum_t *a, const datum_t *b) const {
            return *a < *b;
        }
    };

    std::map<const datum_t *, const datum_t *, datum_value_compare_t> map;
    std::vector<std::pair<Datum, Datum> > map_pb;

public:
    friend class write_message_t;
    void rdb_serialize(write_message_t &msg /* NOLINT */) const {
        r_sanity_check(state == READY_TO_WRITE);
        msg << map_pb;
    }
    friend class archive_deserializer_t;
    archive_result_t rdb_deserialize(read_stream_t *s) {
        archive_result_t res = deserialize(s, &map_pb);
        if (res) return res;
        state = JUST_READ;
        return ARCHIVE_SUCCESS;
    }

private:
    enum { JUST_READ, COMPILED, READY_TO_WRITE } state;
};

} // namespace ql
#endif // RDB_PROTOCOL_DATUM_HPP_
