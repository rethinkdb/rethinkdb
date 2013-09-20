// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DATUM_HPP_
#define RDB_PROTOCOL_DATUM_HPP_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "btree/keys.hpp"
#include "containers/archive/archive.hpp"
#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "http/json.hpp"
#include "rdb_protocol/error.hpp"

class Datum;

RDB_DECLARE_SERIALIZABLE(Datum);

namespace ql {
class datum_stream_t;
class env_t;
class val_t;

namespace pseudo {
class datum_cmp_t;
void time_to_str_key(const datum_t &d, std::string *str_out);
void sanitize_time(datum_t *time);
} // namespace pseudo

// These let us write e.g. `foo(NOTHROW) instead of `foo(false/*nothrow*/)`.
// They should be passed to functions that have multiple behaviors (like `get` or
// `add` below).

// NOTHROW: Return NULL
// THROW: Throw an error
enum throw_bool_t { NOTHROW = 0, THROW = 1};

// NOCLOBBER: Don't overwrite existing values.
// CLOBBER: Overwrite existing values.
enum clobber_bool_t { NOCLOBBER = 0, CLOBBER = 1};

// A `datum_t` is basically a JSON value, although we may extend it later.
class datum_t : public slow_atomic_countable_t<datum_t> {
public:
    // This ordering is important, because we use it to sort objects of
    // disparate type.  It should be alphabetical.
    enum type_t { UNINITIALIZED = 0, R_ARRAY = 1, R_BOOL = 2, R_NULL = 3,
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
    explicit datum_t(std::string &&str);
    explicit datum_t(const char *cstr);
    explicit datum_t(std::vector<counted_t<const datum_t> > &&_array);
    explicit datum_t(std::map<std::string, counted_t<const datum_t> > &&object);

    // These construct a datum from an equivalent representation.
    datum_t();
    explicit datum_t(const Datum *d);
    void init_from_pb(const Datum *d);
    explicit datum_t(cJSON *json);
    explicit datum_t(const scoped_cJSON_t &json);

    ~datum_t();

    void write_to_protobuf(Datum *out) const;

    type_t get_type() const;
    bool is_ptype() const;
    bool is_ptype(const std::string &reql_type) const;
    std::string get_reql_type() const;
    std::string get_type_name() const;
    std::string print() const;
    static const size_t trunc_len = 300;
    std::string trunc_print() const;
    std::string print_primary() const;
    static std::string mangle_secondary(const std::string &secondary,
            const std::string &primary, const std::string &tag);
    std::string print_secondary(const store_key_t &key,
            boost::optional<uint64_t> tag_num = boost::optional<uint64_t>()) const;
    /* An inverse to print_secondary. Returns the primary key. */
    static std::string extract_primary(const std::string &secondary_and_primary);
    static std::string extract_secondary(const std::string &secondary_and_primary);
    static boost::optional<size_t> extract_tag(const std::string &secondary_and_primary);
    store_key_t truncated_secondary() const;
    void check_type(type_t desired, const char *msg = NULL) const;
    void type_error(const std::string &msg) const NORETURN;

    bool as_bool() const;
    double as_num() const;
    int64_t as_int() const;
    const std::string &as_str() const;

    // Use of `size` and `get` is preferred to `as_array` when possible.
    const std::vector<counted_t<const datum_t> > &as_array() const;
    size_t size() const;
    // Access an element of an array.
    counted_t<const datum_t> get(size_t index, throw_bool_t throw_bool = THROW) const;
    // Use of `get` is preferred to `as_object` when possible.
    const std::map<std::string, counted_t<const datum_t> > &as_object() const;

    // Access an element of an object.
    counted_t<const datum_t> get(const std::string &key,
                                 throw_bool_t throw_bool = THROW) const;
    counted_t<const datum_t> merge(counted_t<const datum_t> rhs) const;
    typedef counted_t<const datum_t> (*merge_resoluter_t)(const std::string &key,
                                                          counted_t<const datum_t> l,
                                                          counted_t<const datum_t> r);
    counted_t<const datum_t> merge(counted_t<const datum_t> rhs,
                                   merge_resoluter_t f) const;

    cJSON *as_json_raw() const;
    scoped_cJSON_t as_json() const;
    counted_t<datum_stream_t> as_datum_stream(
            const protob_t<const Backtrace> &backtrace) const;

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

    void runtime_fail(base_exc_t::type_t exc_type,
                      const char *test, const char *file, int line,
                      std::string msg) const NORETURN;

    static size_t max_trunc_size();
    static size_t trunc_size(size_t primary_key_size);
    /* Note key_is_truncated returns true if the key is of max size. This gives
     * a false positive if the sum sizes of the keys is exactly the maximum but
     * not over at all. This means that a key of exactly max_trunc_size counts
     * as truncated by this function. Unfortunately there isn't a general way
     * to tell if keys of max_trunc_size were exactly that size or longer and
     * thus truncated. */
    static bool key_is_truncated(const store_key_t &key);

    void rcheck_is_ptype(const std::string s = "") const;

private:
    friend class datum_ptr_t;
    friend void pseudo::sanitize_time(datum_t *time);
    void add(counted_t<const datum_t> val); // add to an array
    // change an element of an array
    void change(size_t index, counted_t<const datum_t> val);
    void insert(size_t index, counted_t<const datum_t> val); // insert into an array
    void erase(size_t index); // erase from an array
    void erase_range(size_t start, size_t end); // erase a range from an array
    void splice(size_t index, counted_t<const datum_t> values);
    MUST_USE bool add(const std::string &key, counted_t<const datum_t> val,
                      clobber_bool_t clobber_bool = NOCLOBBER); // add to an object
    // Returns true if key was in object.
    MUST_USE bool delete_field(const std::string &key);

    void init_empty();
    void init_str();
    void init_array();
    void init_object();
    void init_json(cJSON *json);

    void check_str_validity(const std::string &str);

    friend void pseudo::time_to_str_key(const datum_t &d, std::string *str_out);
    void pt_to_str_key(std::string *str_out) const;
    void num_to_str_key(std::string *str_out) const;
    void str_to_str_key(std::string *str_out) const;
    void bool_to_str_key(std::string *str_out) const;
    void array_to_str_key(std::string *str_out) const;

    int pseudo_cmp(const datum_t &rhs) const;
    static const std::set<std::string> _allowed_pts;
    void maybe_sanitize_ptype(const std::set<std::string> &allowed_pts = _allowed_pts);

    type_t type;
    union {
        bool r_bool;
        double r_num;
        // TODO: Make this a char vector
        std::string *r_str;
        std::vector<counted_t<const datum_t> > *r_array;
        std::map<std::string, counted_t<const datum_t> > *r_object;
    };

public:
    static const char* const reql_type_string;

private:
    DISABLE_COPYING(datum_t);
};

size_t serialized_size(const counted_t<const datum_t> &datum);

write_message_t &operator<<(write_message_t &wm, const counted_t<const datum_t> &datum);
archive_result_t deserialize(read_stream_t *s, counted_t<const datum_t> *datum);

write_message_t &operator<<(write_message_t &wm, const empty_ok_t<const counted_t<const datum_t> > &datum);
archive_result_t deserialize(read_stream_t *s, empty_ok_ref_t<counted_t<const datum_t> > datum);

// Converts a double to int, but returns false if it's not an integer or out of range.
bool number_as_integer(double d, int64_t *i_out);

// Converts a double to int, calling number_as_integer and throwing if it fails.
int64_t checked_convert_to_int(const rcheckable_t *target, double d);

// If you need to do mutable operations to a `datum_t`, use one of these (it's
// basically a `scoped_ptr_t` that can access private methods on `datum_t` and
// checks for pseudotype validity when you turn it into a `counted_t<const
// datum_t>`).
class datum_ptr_t {
public:
    template<class... Args>
    explicit datum_ptr_t(Args... args) : ptr_(make_scoped<datum_t>(std::forward<Args>(args)...)) { }
    counted_t<const datum_t> to_counted(
            const std::set<std::string> &allowed_ptypes = std::set<std::string>()) {
        ptr()->maybe_sanitize_ptype(allowed_ptypes);
        return counted_t<const datum_t>(ptr_.release());
    }
    const datum_t *operator->() const { return const_ptr(); }
    void add(counted_t<const datum_t> val) { ptr()->add(val); }
    void change(size_t i, counted_t<const datum_t> val) { ptr()->change(i, val); }
    void insert(size_t i, counted_t<const datum_t> val) { ptr()->insert(i, val); }
    void erase(size_t i) { ptr()->erase(i); }
    void erase_range(size_t start, size_t end) { ptr()->erase_range(start, end); }
    void splice(size_t index, counted_t<const datum_t> values) {
        ptr()->splice(index, values);
    }
    MUST_USE bool add(const std::string &key, counted_t<const datum_t> val,
                      clobber_bool_t clobber_bool = NOCLOBBER) {
        return ptr()->add(key, val, clobber_bool);
    }
    MUST_USE bool delete_field(const std::string &key) {
        return ptr()->delete_field(key);
    }

private:
    datum_t *ptr() {
        r_sanity_check(ptr_.has());
        return ptr_.get();
    }
    const datum_t *const_ptr() const {
        r_sanity_check(ptr_.has());
        return ptr_.get();
    }

    scoped_ptr_t<datum_t> ptr_;
    DISABLE_COPYING(datum_ptr_t);
};

// This is like a `wire_datum_t` but for gmr.  We need it because gmr allows
// non-strings as keys, while the data model we pinched from JSON doesn't.  See
// README.md for more info.
class wire_datum_map_t {
public:
    wire_datum_map_t() : state(COMPILED) { }
    bool has(counted_t<const datum_t> key);
    counted_t<const datum_t> get(counted_t<const datum_t> key);
    void set(counted_t<const datum_t> key, counted_t<const datum_t> val);

    void compile();
    void finalize();

    counted_t<const datum_t> to_arr() const;
private:
    struct datum_value_compare_t {
        bool operator()(counted_t<const datum_t> a, counted_t<const datum_t> b) const {
            return *a < *b;
        }
    };

    std::map<counted_t<const datum_t>,
             counted_t<const datum_t>,
             datum_value_compare_t> map;
    std::vector<std::pair<Datum, Datum> > map_pb;

public:
    friend class write_message_t;
    void rdb_serialize(write_message_t &msg /* NOLINT */) const;
    friend class archive_deserializer_t;
    archive_result_t rdb_deserialize(read_stream_t *s);

private:
    enum { SERIALIZABLE, COMPILED } state;
};

namespace pseudo {
class datum_cmp_t {
public:
    virtual int operator()(const datum_t &x, const datum_t &y) const = 0;
    virtual ~datum_cmp_t() { }
};
} // namespace pseudo

} // namespace ql
#endif // RDB_PROTOCOL_DATUM_HPP_
