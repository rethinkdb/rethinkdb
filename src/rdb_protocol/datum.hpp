#ifndef RDB_PROTOCOL_DATUM_HPP_
#define RDB_PROTOCOL_DATUM_HPP_
#include <string>
#include <vector>
#include <map>

#include "utils.hpp"
#include <boost/shared_ptr.hpp>

#include "containers/archive/archive.hpp"
#include <containers/ptr_bag.hpp>
#include "http/json.hpp"

class Datum;

namespace ql {
class env_t;
class datum_stream_t;

class datum_t : public ptr_baggable_t {
public:
    datum_t(); // R_NULL
    explicit datum_t(bool _bool); // undefined, catches implicit conversion errors
    datum_t(bool _bool, bool __bool);
    explicit datum_t(double _num);
    explicit datum_t(int _num);
    explicit datum_t(const std::string &_str);
    explicit datum_t(const std::vector<const datum_t *> &_array);
    explicit datum_t(const std::map<const std::string, const datum_t *> &_object);

    explicit datum_t(const Datum *d); // Undefined, need to pass `env` below.
    explicit datum_t(const Datum *d, env_t *env);
    explicit datum_t(cJSON *json);
    explicit datum_t(cJSON *json, env_t *env);
    explicit datum_t(boost::shared_ptr<scoped_cJSON_t> json);
    explicit datum_t(boost::shared_ptr<scoped_cJSON_t> json, env_t *env);
    void write_to_protobuf(Datum *out) const;

    enum type_t {
        R_NULL   = 1,
        R_BOOL   = 2,
        R_NUM    = 3,
        R_STR    = 4,
        R_ARRAY  = 5,
        R_OBJECT = 6
    };
    explicit datum_t(type_t _type);
    type_t get_type() const;
    const char *get_type_name() const;
    std::string print() const;
    std::string print_primary() const;
    void check_type(type_t desired) const;

    bool as_bool() const;
    double as_num() const;
    int as_int() const;
    const std::string &as_str() const;

    const std::vector<const datum_t *> &as_array() const;
    void add(const datum_t *val);
    size_t size() const;
    const datum_t *el(size_t index, bool throw_if_missing = true) const;

    const std::map<const std::string, const datum_t *> &as_object() const;
    // Returns true if `key` was already in object.
    MUST_USE bool add(const std::string &key, const datum_t *val, bool clobber = false);
    // Returns true if key was in object.
    MUST_USE bool del(const std::string &key);
    const datum_t *el(const std::string &key, bool throw_if_missing = true) const;
    const datum_t *merge(const datum_t *rhs) const;

    cJSON *as_raw_json() const;
    boost::shared_ptr<scoped_cJSON_t> as_json() const;
    datum_stream_t *as_datum_stream(env_t *env) const;

    int cmp(const datum_t &rhs) const;
    bool operator==(const datum_t &rhs) const;
    bool operator!=(const datum_t &rhs) const;

    bool operator<(const datum_t &rhs) const;
    bool operator<=(const datum_t &rhs) const;
    bool operator>(const datum_t &rhs) const;
    bool operator>=(const datum_t &rhs) const;

    void iter(bool (*callback)(const datum_t *, env_t *), env_t *env) const;
private:
    void init_json(cJSON *json, env_t *env);

    // Listing everything is more debugging-friendly than a boost::variant,
    // but less efficient.  TODO: fix later.
    type_t type;
    bool r_bool;
    double r_num;
    std::string r_str;

    std::vector<const datum_t *> r_array;
    std::map<const std::string, const datum_t *> r_object;
};

RDB_DECLARE_SERIALIZABLE(Datum);
class wire_datum_t {
public:
    wire_datum_t() : ptr(0), state(INVALID) { }
    wire_datum_t(const datum_t *_ptr) : ptr(_ptr), state(COMPILED) { }
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

class wire_datum_map_t {
public:
    wire_datum_map_t() : map(lt), state(COMPILED) { }
    bool has(const datum_t *key);
    const datum_t *get(const datum_t *key);
    void set(const datum_t *key, const datum_t *val);

    void compile(env_t *env);
    void finalize();

    const datum_t *to_arr(env_t *env) const;
private:
    static bool lt(const datum_t *a, const datum_t *b) {
        return *a < *b;
    }
    typedef std::map<const datum_t *, const datum_t *,
                     bool (*)(const datum_t *, const datum_t *)
                     > map_t;
    map_t map;
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
