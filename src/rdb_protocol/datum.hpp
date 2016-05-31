// Copyright 2010-2015 RethinkDB, all rights reserved.
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
#include "cjson/json.hpp"
#include "containers/archive/archive.hpp"
#include "containers/counted.hpp"
#include "rdb_protocol/datum_string.hpp"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
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
class datum_t;
class env_t;
class grouped_data_t;
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

// When getting the typename of a datum, this should be YES if the name will be
// used for sorting datums by type, and NO if the name is to be given to a user.
enum class name_for_sorting_t { NO = 0, YES = 1 };

// `r.minval` and `r.maxval` were encoded in keys differently before 2.3. Determines
// which encoding should be used.
enum class extrema_encoding_t { PRE_v2_3, LATEST };
extrema_encoding_t extrema_encoding_from_reql_version_for_sindex(reql_version_t rv);

// When constructing a secondary index key, extremas should not be used.  They
// may be used when constructing secondary index ranges (i.e. for `between`).
enum class extrema_ok_t { NOT_OK = 0, OK = 1 };

// When printing keys, nulls in strings can be escaped or disallowed. Primary
// keys always disallow nulls, and the most recent secondary indexes escape them.
//
// Use escape_nulls_from_reql_version_for_sindex to get a value appropriate for
// sindexes created using a given reql_version_t.
enum class escape_nulls_t { NO = 0, YES = 1 };
escape_nulls_t escape_nulls_from_reql_version_for_sindex(reql_version_t rv);

void debug_print(printf_buffer_t *, const datum_t &);

// The serialization for this is defined in `protocol.cc` and needs to be
// updated if more versions are added.
enum class skey_version_t {
    // We used to distinguish between pre 1.16 and post 1.16 secondary index keys.
    // At the moment we only have one format.
    post_1_16 = 1
};
skey_version_t skey_version_from_reql_version(reql_version_t rv);

struct components_t {
    skey_version_t skey_version;
    std::string secondary;
    std::string primary;
    boost::optional<uint64_t> tag_num;
};

// A `datum_t` is basically a JSON value, with some special handling for
// ReQL pseudo-types.
class datum_t {
private:
    // Placed here so it's kept in sync with type_t. All enum values from
    // type_t must appear in here.
    enum class internal_type_t {
        UNINITIALIZED,
        MINVAL,
        R_ARRAY,
        R_BINARY,
        R_BOOL,
        R_NULL,
        R_NUM,
        R_OBJECT,
        R_STR,
        BUF_R_ARRAY,
        BUF_R_OBJECT,
        MAXVAL
    };
public:
    // This ordering is important, because we use it to sort objects of
    // disparate type.  It should be alphabetical.
    enum type_t {
        UNINITIALIZED = 0,
        MINVAL = 1,
        R_ARRAY = 2,
        R_BINARY = 3,
        R_BOOL = 4,
        R_NULL = 5,
        R_NUM = 6,
        R_OBJECT = 7,
        R_STR = 8,
        MAXVAL = 9
    };

    static datum_t empty_array();
    static datum_t empty_object();
    // Constructs an an R_NULL datum.
    static datum_t null();
    static datum_t boolean(bool value);
    static datum_t binary(datum_string_t &&value);
    static datum_t binary(const datum_string_t &value);

    static datum_t utf8(datum_string_t _data);

    static datum_t minval();
    static datum_t maxval();

    // Construct an uninitialized datum_t. This is to ease the transition from
    // counted_t<const datum_t>
    datum_t();

    // Construct a datum_t from a shared buffer.
    // type can be one of R_BINARY, R_STR, R_OBJECT or R_ARRAY. _buf_ref must point
    // to the data buffer *without* the serialized type tag, but *with* the
    // prefixed serialized size.
    datum_t(type_t type, shared_buf_ref_t<char> &&buf_ref);

    // Strongly prefer datum_t::minval().
    enum class construct_minval_t { };
    explicit datum_t(construct_minval_t);

    // Strongly prefer datum_t::maxval().
    enum class construct_maxval_t { };
    explicit datum_t(construct_maxval_t);

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
    explicit datum_t(construct_binary_t, datum_string_t _data);

    explicit datum_t(double _num);
    explicit datum_t(datum_string_t _str);
    explicit datum_t(const std::string &string);
    explicit datum_t(const char *cstr);
    explicit datum_t(std::vector<datum_t> &&_array,
                     const configured_limits_t &limits);

    enum class no_array_size_limit_check_t { };
    // Constructs a datum_t without checking the array size.  Used by
    // datum_array_builder_t to maintain identical non-checking behavior with insert
    // and splice operations -- see https://github.com/rethinkdb/rethinkdb/issues/2697
    datum_t(std::vector<datum_t> &&_array,
            no_array_size_limit_check_t);
    // This calls maybe_sanitize_ptype(allowed_pts).
    // The complexity of this constructor is O(object.size()).
    explicit datum_t(std::map<datum_string_t, datum_t> &&object,
                     const std::set<std::string> &allowed_pts = _allowed_pts);
    // This calls maybe_sanitize_ptype(allowed_pts).
    explicit datum_t(std::vector<std::pair<datum_string_t, datum_t> > &&object,
                     const std::set<std::string> &allowed_pts = _allowed_pts);

    enum class no_sanitize_ptype_t { };
    // This .. does not call maybe_sanitize_ptype.
    // TODO(2015-01): Remove this constructor, it's a hack.
    datum_t(std::map<datum_string_t, datum_t> &&object, no_sanitize_ptype_t);

    // has() checks whether a datum is uninitialized. reset() makes any datum
    // uninitialized.
    bool has() const;
    void reset();

    type_t get_type() const;
    bool is_ptype() const;
    bool is_ptype(const std::string &reql_type) const;
    std::string get_reql_type() const;
    std::string get_type_name(
        name_for_sorting_t for_sorting = name_for_sorting_t::NO) const;
    std::string print(reql_version_t reql_version = reql_version_t::LATEST) const;
    static const size_t trunc_len = 300;
    std::string trunc_print() const;
    std::string print_primary() const;
    std::string print_primary_internal() const;
    /* TODO: All of this key-mangling logic belongs elsewhere. Maybe
    `print_primary()` belongs there as well. */
    static std::string compose_secondary(skey_version_t skey_version,
                                         const std::string &secondary_key,
                                         const store_key_t &primary_key,
                                         boost::optional<uint64_t> tag_num);
    static std::string mangle_secondary(
        skey_version_t skey_version,
        const std::string &secondary,
        const std::string &primary,
        const std::string &tag);
    static std::string encode_tag_num(uint64_t tag_num);
    // tag_num is used for multi-indexes.
    std::string print_secondary(reql_version_t reql_version,
                                const store_key_t &primary_key,
                                boost::optional<uint64_t> tag_num) const;
    /* An inverse to print_secondary. Returns the primary key. */
    static std::string extract_primary(const std::string &secondary_and_primary);
    static store_key_t extract_primary(const store_key_t &secondary_key);
    static std::string extract_truncated_secondary(
        const std::string &secondary_and_primary);
    static std::string extract_secondary(const std::string &secondary_and_primary);
    static boost::optional<uint64_t> extract_tag(
        const std::string &secondary_and_primary);
    static boost::optional<uint64_t> extract_tag(const store_key_t &key);
    static components_t extract_all(const std::string &secondary_and_primary);
    store_key_t truncated_secondary(
        reql_version_t reql_version,
        extrema_ok_t extrema_ok = extrema_ok_t::NOT_OK) const;
    void check_type(type_t desired, const char *msg = NULL) const;
    NORETURN void type_error(const std::string &msg) const;

    bool as_bool() const;
    double as_num() const;
    int64_t as_int() const;
    const datum_string_t &as_str() const;
    const datum_string_t &as_binary() const;

    // Array interface
    size_t arr_size() const;
    // Access an element of an array.
    datum_t get(size_t index, throw_bool_t throw_bool = THROW) const;

    // Object interface
    size_t obj_size() const;
    // Access an element of an object.
    // get_pair does not perform boundary checking. Its primary use is for
    // iterating over the object in combination with num_pairs().
    std::pair<datum_string_t, datum_t> get_pair(size_t index) const;
    datum_t get_field(const datum_string_t &key,
                      throw_bool_t throw_bool = THROW) const;
    datum_t get_field(const char *key,
                      throw_bool_t throw_bool = THROW) const;
    datum_t merge(const datum_t &rhs) const;
    // "Consumer defined" merge resolutions; these take limits unlike
    // the other merge because the merge resolution can and does (in
    // stats) merge two arrays to form one super array, which can
    // obviously breach limits.
    // This takes std::set<std::string> instead of std::set<const std::string> not
    // because it plans on modifying the strings, but because the latter doesn't work.
    typedef datum_t (*merge_resoluter_t)(const datum_string_t &key,
                                         datum_t l,
                                         datum_t r,
                                         const configured_limits_t &limits,
                                         std::set<std::string> *conditions);
    datum_t merge(const datum_t &rhs,
                  merge_resoluter_t f,
                  const configured_limits_t &limits,
                  std::set<std::string> *conditions) const;

    // json_writer_t can be rapidjson::Writer<rapidjson::StringBuffer>
    // or rapidjson::PrettyWriter<rapidjson::StringBuffer>
    template <class json_writer_t> void write_json(json_writer_t *writer) const;
    rapidjson::Value as_json(rapidjson::Value::AllocatorType *allocator) const;

    // DEPRECATED: Used for backwards compatibility with reql_versions before 2.1
    cJSON *as_json_raw() const;
    scoped_cJSON_t as_json() const;

    counted_t<datum_stream_t> as_datum_stream(backtrace_id_t backtrace) const;

    // These behave as expected and defined in RQL.  Theoretically, two data of the
    // same type should compare appropriately, while disparate types are compared
    // alphabetically by type name.
    int cmp(const datum_t &rhs) const;

    // operator== and operator!= don't take a reql_version_t, unlike other comparison
    // functions, because we know (by inspection) that the behavior of cmp() hasn't
    // changed with respect to the question of equality vs. inequality.
    bool operator==(const datum_t &rhs) const;
    bool operator!=(const datum_t &rhs) const;
    bool operator<(const datum_t &rhs) const;
    bool operator<=(const datum_t &rhs) const;
    bool operator>(const datum_t &rhs) const;
    bool operator>=(const datum_t &rhs) const;

    NORETURN void runtime_fail(base_exc_t::type_t exc_type,
                               const char *test, const char *file, int line,
                               std::string msg) const;

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
    void rcheck_valid_replace(datum_t old_val,
                              datum_t orig_key,
                              const datum_string_t &pkey) const;

    // Used by skey_version code. Returns a pointer to the buf_ref, if
    // the datum is currently backed by one, or NULL otherwise.
    const shared_buf_ref_t<char> *get_buf_ref() const;

private:
    // We have a special version of `call_with_enough_stack` for datums that only uses
    // `call_with_enough_stack` if there a chance of additional recursion (based on
    // the type of this datum). Since some of the datum functions get called a lot,
    // this is valuable since we can often save the overhead of the extra function
    // call.
    template<class result_t, class callable_t>
    inline result_t call_with_enough_stack_datum(callable_t &&fun) const;
    template<class callable_t>
    inline void call_with_enough_stack_datum(callable_t &&fun) const;

    friend void pseudo::sanitize_time(datum_t *time);
    // Must only be used during pseudo type sanitization.
    // The key must already exist.
    void replace_field(const datum_string_t &key, datum_t val);

    static std::vector<std::pair<datum_string_t, datum_t> > to_sorted_vec(
            std::map<datum_string_t, datum_t> &&map);

    template <class json_writer_t>
    void write_json_unchecked_stack(json_writer_t *writer) const;

    // Same as get_pair() / get(), but don't perform boundary or type checks.
    // For internal use to improve performance.
    std::pair<datum_string_t, datum_t> unchecked_get_pair(size_t index) const;
    datum_t unchecked_get(size_t) const;

    datum_t default_merge_unchecked_stack(const datum_t &rhs) const;
    datum_t custom_merge_unchecked_stack(const datum_t &rhs,
                                         merge_resoluter_t f,
                                         const configured_limits_t &limits,
                                         std::set<std::string> *conditions) const;

    friend void pseudo::time_to_str_key(const datum_t &d, std::string *str_out);
    void pt_to_str_key(std::string *str_out) const;
    void num_to_str_key(std::string *str_out) const;
    void str_to_str_key(escape_nulls_t escape_nulls, std::string *str_out) const;
    void bool_to_str_key(std::string *str_out) const;
    void array_to_str_key(
        extrema_encoding_t extrema_encoding,
        extrema_ok_t extrema_ok,
        escape_nulls_t escape_nulls,
        std::string *str_out) const;
    void binary_to_str_key(std::string *str_out) const;
    void extrema_to_str_key(
        extrema_encoding_t extrema_encoding,
        extrema_ok_t extrema_ok,
        std::string *str_out) const;

    int cmp_unchecked_stack(const datum_t &rhs) const;

    int pseudo_cmp(const datum_t &rhs) const;
    bool pseudo_compares_as_obj() const;
    static const std::set<std::string> _allowed_pts;
    void maybe_sanitize_ptype(const std::set<std::string> &allowed_pts = _allowed_pts);

    // Helper function for `merge()`:
    // Returns a version of this where all `literal` pseudotypes have been omitted.
    // Might return null, if this is a literal without a value.
    datum_t drop_literals(bool *encountered_literal_out) const;
    datum_t drop_literals_unchecked_stack(bool *encountered_literal_out) const;

    // The data_wrapper makes sure we perform proper cleanup when exceptions
    // happen during construction
    class data_wrapper_t {
    public:
        data_wrapper_t(const data_wrapper_t &copyee);
        data_wrapper_t &operator=(const data_wrapper_t &copyee);
        data_wrapper_t(data_wrapper_t &&movee) noexcept;

        // Mirror the same constructors of datum_t
        data_wrapper_t();
        explicit data_wrapper_t(construct_minval_t);
        explicit data_wrapper_t(construct_maxval_t);
        explicit data_wrapper_t(construct_null_t);
        data_wrapper_t(construct_boolean_t, bool _bool);
        data_wrapper_t(construct_binary_t, datum_string_t data);
        explicit data_wrapper_t(double num);
        explicit data_wrapper_t(datum_string_t str);
        explicit data_wrapper_t(const char *cstr);
        explicit data_wrapper_t(std::vector<datum_t> &&array);
        explicit data_wrapper_t(
                std::vector<std::pair<datum_string_t, datum_t> > &&object);
        data_wrapper_t(type_t type, shared_buf_ref_t<char> &&_buf_ref);

        ~data_wrapper_t();

        type_t get_type() const;
        internal_type_t get_internal_type() const;

        union {
            bool r_bool;
            double r_num;
            datum_string_t r_str;
            counted_t<countable_wrapper_t<std::vector<datum_t> > > r_array;
            counted_t<countable_wrapper_t<std::vector< //NOLINT(whitespace/operators)
                std::pair<datum_string_t, datum_t> > > > r_object;
            shared_buf_ref_t<char> buf_ref;
        };
    private:
        void assign_copy(const data_wrapper_t &copyee);
        void assign_move(data_wrapper_t &&movee) noexcept;
        void destruct();

        internal_type_t internal_type;
    } data;

    friend void ::ql::debug_print(printf_buffer_t *, const datum_t &);

public:
    static const datum_string_t reql_type_string;
};

datum_t to_datum(const Datum *d, const configured_limits_t &, reql_version_t);
datum_t to_datum(
    const rapidjson::Value &json,
    const configured_limits_t &,
    reql_version_t);

// DEPRECATED: Used in the r.json term for pre 2.1 backwards compatibility
datum_t to_datum(cJSON *json, const configured_limits_t &, reql_version_t);

// This should only be used to send responses to the client.
datum_t to_datum_for_client_serialization(
    grouped_data_t &&gd, const configured_limits_t &);

// Converts a double to int, but returns false if it's not an integer or out of range.
bool number_as_integer(double d, int64_t *i_out);

// Converts a double to int, calling number_as_integer and throwing if it fails.
int64_t checked_convert_to_int(const rcheckable_t *target, double d);

// Useful for building an object datum and doing mutation operations
class datum_object_builder_t {
public:
    datum_object_builder_t() { }
    explicit datum_object_builder_t(const datum_t &copy_from);

    bool empty() const {
        return map.empty();
    }

    // Returns true if the insertion did _not_ happen because the key was already in
    // the object.
    MUST_USE bool add(const datum_string_t &key, datum_t val);
    MUST_USE bool add(const char *key, datum_t val);
    // Inserts a new key or overwrites the existing key's value.
    void overwrite(const datum_string_t &key, datum_t val);
    void overwrite(const char *key, datum_t val);
    void add_warning(const char *msg, const configured_limits_t &limits);
    void add_warnings(
        const std::set<std::string> &msgs, const configured_limits_t &limits);
    void add_error(const char *msg);

    MUST_USE bool delete_field(const datum_string_t &key);
    MUST_USE bool delete_field(const char *key);

    datum_t at(const datum_string_t &key) const;

    // Returns null if the key doesn't exist.
    datum_t try_get(const datum_string_t &key) const;

    MUST_USE datum_t to_datum() RVALUE_THIS;

    MUST_USE datum_t to_datum(
            const std::set<std::string> &permissible_ptypes) RVALUE_THIS;

private:
    std::map<datum_string_t, datum_t> map;
    DISABLE_COPYING(datum_object_builder_t);
};

// Useful for building an array datum and doing mutation operations -- while having
// array-size checks on the fly.
class datum_array_builder_t {
public:
    explicit datum_array_builder_t(
        const configured_limits_t &_limits) : limits(_limits) {}
    explicit datum_array_builder_t(
        const datum_t &copy_from, const configured_limits_t &);

    bool empty() const { return vector.empty(); }
    size_t size() const { return vector.size(); }

    void reserve(size_t n);

    // Note that these methods produce behavior that is actually specific to the
    // definition of certain ReQL terms.
    void add(datum_t val);
    void change(size_t i, datum_t val);

    // On v1_13, insert and splice don't enforce the array size limit.
    void insert(size_t index, datum_t val);
    void splice(size_t index, datum_t values);

    // On v1_13, erase_range doesn't allow start and end to equal array_size.
    void erase_range(size_t start, size_t end);

    void erase(size_t index);

    datum_t to_datum() RVALUE_THIS;

private:
    std::vector<datum_t> vector;
    configured_limits_t limits;

    DISABLE_COPYING(datum_array_builder_t);
};

// This function is used by e.g. foreach to merge statistics from multiple write
// operations.
datum_t stats_merge(UNUSED const datum_string_t &key,
                    datum_t l,
                    datum_t r,
                    const configured_limits_t &limits,
                    std::set<std::string> *conditions);

namespace pseudo {
class datum_cmp_t {
public:
    virtual int operator()(const datum_t &x, const datum_t &y) const = 0;
    virtual ~datum_cmp_t() { }
};
} // namespace pseudo

} // namespace ql

#endif // RDB_PROTOCOL_DATUM_HPP_
