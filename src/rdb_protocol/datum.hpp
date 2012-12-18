#ifndef RDB_PROTOCOL_DATUM_HPP_
#define RDB_PROTOCOL_DATUM_HPP_
#include <string>
#include <vector>
#include <map>

#include "utils.hpp"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>

#include "http/json.hpp"

class Datum;

namespace ql {
class datum_t {
public:
    datum_t(); // R_NULL
    explicit datum_t(bool _bool);
    explicit datum_t(double _num);
    explicit datum_t(const std::string &_str);
    explicit datum_t(const std::vector<const datum_t *> &_array);
    explicit datum_t(const std::map<const std::string, const datum_t *> &_object);
    explicit datum_t(const Datum *d);
    explicit datum_t(cJSON *json);
    explicit datum_t(boost::shared_ptr<scoped_cJSON_t> json);
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
    std::string print() const ;
    void check_type(type_t desired) const;

    bool as_bool() const;
    double as_num() const;
    const std::string &as_str() const;
    const std::vector<const datum_t *> &as_array() const;
    const std::map<const std::string, const datum_t *> &as_object() const;
    cJSON *as_raw_json() const;
    boost::shared_ptr<scoped_cJSON_t> as_json() const;

    int cmp(const datum_t &rhs) const;
    bool operator==(const datum_t &rhs) const;
    bool operator!=(const datum_t &rhs) const;
    bool operator<(const datum_t &rhs) const;
    bool operator<=(const datum_t &rhs) const;
    bool operator>(const datum_t &rhs) const;
    bool operator>=(const datum_t &rhs) const;

    void add(const datum_t *val);
    // Returns whether or not `key` was already present in object.
    MUST_USE bool add(const std::string &key, const datum_t *val, bool clobber = false);
private:
    void init_json(cJSON *json);

    // Listing everything is more debugging-friendly than a boost::variant,
    // but less efficient.  TODO: fix later.
    type_t type;
    bool r_bool;
    double r_num;
    std::string r_str;

    std::vector<const datum_t *> r_array;
    std::map<const std::string, const datum_t *> r_object;
    // Sometimes the `datum_t` owns its members, like when it's a literal
    // `datum_t` provided by the user.
    // TODO: change to environment ownership
    boost::ptr_vector<datum_t> to_free;
};
} //namespace ql
#endif // RDB_PROTOCOL_DATUM_HPP_
