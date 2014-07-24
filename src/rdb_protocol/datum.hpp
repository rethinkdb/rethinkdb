// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DATUM_HPP_
#define RDB_PROTOCOL_DATUM_HPP_

#include <float.h>

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
#include "containers/wire_string.hpp"
#include "http/json.hpp"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/serialize_datum.hpp"
#include "version.hpp"

// Enough precision to reconstruct doubles from their decimal representations.
// Unlike the late DBLPRI, this lacks a percent sign.
#define PR_RECONSTRUCTABLE_DOUBLE ".20g"

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

static const double max_dbl_int = 0x1LL << DBL_MANT_DIG;
static const double min_dbl_int = max_dbl_int * -1;

// These let us write e.g. `foo(NOTHROW) instead of `foo(false/*nothrow*/)`.
// They should be passed to functions that have multiple behaviors (like `get` or
// `add` below).

// NOTHROW: Return NULL
// THROW: Throw an error
enum throw_bool_t { NOTHROW = 0, THROW = 1 };

// NOCLOBBER: Don't overwrite existing values.
// CLOBBER: Overwrite existing values.
enum clobber_bool_t { NOCLOBBER = 0, CLOBBER = 1 };

enum class use_json_t { NO = 0, YES = 1 };

class grouped_data_t;

// A `datum_t` is basically a JSON value, although we may extend it later.
class datum_t : public slow_atomic_countable_t<datum_t> {
public:
    // This ordering is important, because we use it to sort objects of
    // disparate type.  It should be alphabetical.
    enum type_t {
        R_ARRAY = 1,
        R_BINARY = 2,
        R_BOOL = 3,
        R_NULL = 4,
        R_NUM = 5,
        R_OBJECT = 6,
        R_STR = 7
    };

    static counted_t<const datum_t> empty_array();
    static counted_t<const datum_t> empty_object();
    // Constructs a pointer to an R_NULL datum.
    static counted_t<const datum_t> null();
    static counted_t<const datum_t> boolean(bool value);
    static counted_t<const datum_t> binary(scoped_ptr_t<wire_string_t> value);

    // Strongly prefer datum_t::null().
    enum class construct_null_t { };
    explicit datum_t(construct_null_t);

    // These allow you to construct a datum from the type of value it
    // represents.  We have some gotchya-constructors to scare away implicit
    // conversions.
    explicit datum_t(std::nullptr_t) = delete;
    explicit datum_t(bool) = delete;
    explicit datum_t(int) = delete;
    explicit datum_t(float) = delete;

    // You need to explicitly ask to construct a bool (to avoid perilous
    // conversions).  You should prefer datum_t::boolean(..).
    enum class construct_boolean_t { };
    datum_t(construct_boolean_t, bool _bool);

    enum class construct_binary_t { };
    explicit datum_t(construct_binary_t, scoped_ptr_t<wire_string_t> data);

    explicit datum_t(double _num);
    // TODO: Eventually get rid of the std::string constructor (in favor of
    //   scoped_ptr_t<wire_string_t>)
    explicit datum_t(std::string &&str);
    explicit datum_t(scoped_ptr_t<wire_string_t> str);
    explicit datum_t(const char *cstr);
    explicit datum_t(std::vector<counted_t<const datum_t> > &&_array,
                     const configured_limits_t &limits);

    enum class no_array_size_limit_check_t { };
    // Constructs a datum_t without checking the array size.  Used by
    // datum_array_builder_t to maintain identical non-checking behavior with insert
    // and splice operations -- see https://github.com/rethinkdb/rethinkdb/issues/2697
    datum_t(std::vector<counted_t<const datum_t> > &&_array,
            no_array_size_limit_check_t);
    // This calls maybe_sanitize_ptype(allowed_pts).
    explicit datum_t(std::map<std::string, counted_t<const datum_t> > &&object,
                     const std::set<std::string> &allowed_pts = _allowed_pts);

    enum class no_sanitize_ptype_t { };
    // This .. does not call maybe_sanitize_ptype.
    // TODO(2014-08): Remove this constructor, it's a hack.
    datum_t(std::map<std::string, counted_t<const datum_t> > &&object,
            no_sanitize_ptype_t);

    ~datum_t();

    void write_to_protobuf(Datum *out, use_json_t use_json) const;

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
    static store_key_t extract_primary(const store_key_t &secondary_key);
    static std::string extract_secondary(const std::string &secondary_and_primary);
    static boost::optional<uint64_t> extract_tag(
        const std::string &secondary_and_primary);
    static boost::optional<uint64_t> extract_tag(const store_key_t &key);
    store_key_t truncated_secondary() const;
    void check_type(type_t desired, const char *msg = NULL) const;
    void type_error(const std::string &msg) const NORETURN;

    bool as_bool() const;
    double as_num() const;
    int64_t as_int() const;
    const wire_string_t &as_str() const;
    const wire_string_t &as_binary() const;

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
    // "Consumer defined" merge resolutions; these take limits unlike
    // the other merge because the merge resolution can and does (in
    // stats) merge two arrays to form one super array, which can
    // obviously breach limits.
    typedef counted_t<const datum_t> (*merge_resoluter_t)(const std::string &key,
                                                          counted_t<const datum_t> l,
                                                          counted_t<const datum_t> r,
                                                          const configured_limits_t &limits);
    counted_t<const datum_t> merge(counted_t<const datum_t> rhs,
                                   merge_resoluter_t f,
                                   const configured_limits_t &limits) const;

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
    void rcheck_valid_replace(counted_t<const datum_t> old_val,
                              counted_t<const datum_t> orig_key,
                              const std::string &pkey) const;

    static void check_str_validity(const std::string &str);
    static void check_str_validity(const wire_string_t *str);

private:
    friend void pseudo::sanitize_time(datum_t *time);
    MUST_USE bool add(const std::string &key, counted_t<const datum_t> val,
                      clobber_bool_t clobber_bool = NOCLOBBER); // add to an object

    friend void pseudo::time_to_str_key(const datum_t &d, std::string *str_out);
    void pt_to_str_key(std::string *str_out) const;
    void num_to_str_key(std::string *str_out) const;
    void str_to_str_key(std::string *str_out) const;
    void bool_to_str_key(std::string *str_out) const;
    void array_to_str_key(std::string *str_out) const;
    void binary_to_str_key(std::string *str_out) const;

    int pseudo_cmp(const datum_t &rhs) const;
    static const std::set<std::string> _allowed_pts;
    void maybe_sanitize_ptype(const std::set<std::string> &allowed_pts = _allowed_pts);

    // Helper function for `merge()`:
    // Returns a version of this where all `literal` pseudotypes have been omitted.
    // Might return null, if this is a literal without a value.
    counted_t<const datum_t> drop_literals(bool *encountered_literal_out) const;

    type_t type;
    union {
        bool r_bool;
        double r_num;
        wire_string_t *r_str;
        std::vector<counted_t<const datum_t> > *r_array;
        std::map<std::string, counted_t<const datum_t> > *r_object;
    };

public:
    static const char *const reql_type_string;

private:
    DISABLE_COPYING(datum_t);
};

counted_t<const datum_t> to_datum(const Datum *d, const configured_limits_t &);
counted_t<const datum_t> to_datum(cJSON *json, const configured_limits_t &);

// This should only be used to send responses to the client.
counted_t<const datum_t> to_datum(grouped_data_t &&gd, const configured_limits_t &);

// Converts a double to int, but returns false if it's not an integer or out of range.
bool number_as_integer(double d, int64_t *i_out);

// Converts a double to int, calling number_as_integer and throwing if it fails.
int64_t checked_convert_to_int(const rcheckable_t *target, double d);

// Useful for building an object datum and doing mutation operations -- otherwise,
// you'll have to do check_str_validity checks yourself.
class datum_object_builder_t {
public:
    datum_object_builder_t() { }

    datum_object_builder_t(const std::map<std::string, counted_t<const datum_t> > &m)
        : map(m) { }

    // Returns true if the insertion did _not_ happen because the key was already in
    // the object.
    MUST_USE bool add(const std::string &key, counted_t<const datum_t> val);
    // Inserts a new key or overwrites the existing key's value.
    void overwrite(std::string key, counted_t<const datum_t> val);
    void add_error(const char *msg);

    MUST_USE bool delete_field(const std::string &key);

    counted_t<const datum_t> at(const std::string &key) const;

    // Returns null if the key doesn't exist.
    counted_t<const datum_t> try_get(const std::string &key) const;

    MUST_USE counted_t<const datum_t> to_counted() RVALUE_THIS;

    MUST_USE counted_t<const datum_t> to_counted(
            const std::set<std::string> &permissible_ptypes) RVALUE_THIS;

private:
    std::map<std::string, counted_t<const datum_t> > map;
    DISABLE_COPYING(datum_object_builder_t);
};

// Useful for building an array datum and doing mutation operations -- while having
// array-size checks on the fly.
class datum_array_builder_t {
public:
    datum_array_builder_t(const configured_limits_t &_limits) : limits(_limits) {}
    datum_array_builder_t(std::vector<counted_t<const datum_t> > &&,
                          const configured_limits_t &);

    size_t size() const { return vector.size(); }

    void reserve(size_t n);

    // Note that these methods produce behavior that is actually specific to the
    // definition of certain ReQL terms.
    void add(counted_t<const datum_t> val);
    void change(size_t i, counted_t<const datum_t> val);
    void insert(size_t index, counted_t<const datum_t> val);
    void splice(size_t index, counted_t<const datum_t> values);
    void erase_range(size_t start, size_t end);
    void erase(size_t index);

    counted_t<const datum_t> to_counted() RVALUE_THIS;

private:
    std::vector<counted_t<const datum_t> > vector;
    configured_limits_t limits;

    DISABLE_COPYING(datum_array_builder_t);
};

// This function is used by e.g. foreach to merge statistics from multiple write
// operations.
counted_t<const datum_t> stats_merge(UNUSED const std::string &key,
                                     counted_t<const datum_t> l,
                                     counted_t<const datum_t> r,
                                     const configured_limits_t &limits);

namespace pseudo {
class datum_cmp_t {
public:
    virtual int operator()(const datum_t &x, const datum_t &y) const = 0;
    virtual ~datum_cmp_t() { }
};
} // namespace pseudo

} // namespace ql
#endif // RDB_PROTOCOL_DATUM_HPP_
