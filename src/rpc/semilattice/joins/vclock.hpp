#ifndef RPC_SEMILATTICE_JOINS_VCLOCK_HPP_
#define RPC_SEMILATTICE_JOINS_VCLOCK_HPP_

#include <map>
#include <vector>
#include <utility>

#include "containers/uuid.hpp"
#include "containers/map_sentries.hpp"
#include "http/json.hpp"
#include "rpc/serialize_macros.hpp"

namespace vclock_details {

// TODO: This should _NOT_ be an int.
// TODO: There are other things named version_map_t.  Some of these need to change.
typedef std::map<uuid_t, int> version_map_t;

bool dominates(const version_map_t &, const version_map_t &);

version_map_t vmap_max(const version_map_t &, const version_map_t &);

void print_version_map(const version_map_t &);
} //namespace vclock_details

class in_conflict_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "Tried to access a vector clock protected value that was in conflict.";
    }

    virtual ~in_conflict_exc_t() throw () { }
};


class vclock_ctx_t;

template <class T>
class vclock_t {
private:
    template <class TT>
    friend cJSON *with_ctx_render_all_values(vclock_t<TT> *, const vclock_ctx_t &);

    template <class TT>
    friend bool operator==(const vclock_t<TT> &, const vclock_t<TT> &);

    template <class TT>
    friend void semilattice_join(vclock_t<TT> *, const vclock_t<TT> &);

    template <class TT>
    friend void debug_print(append_only_printf_buffer_t *buf, const vclock_t<TT> &x);

    typedef std::pair<vclock_details::version_map_t, T> stamped_value_t;

    typedef std::map<vclock_details::version_map_t, T> value_map_t;
    value_map_t values;

    RDB_MAKE_ME_SERIALIZABLE_1(values);

    explicit vclock_t(const stamped_value_t &_value);

    //if there exist 2 values a,b in values s.t. a.first < b.first remove a
    void cull_old_values();

public:
    vclock_t();

    vclock_t(const T &_t);

    vclock_t(const T &_t, const uuid_t &us);

    bool in_conflict() const;

    void throw_if_conflict() const;

    vclock_t<T> make_new_version(const T& t, const uuid_t &us);

    vclock_t<T> make_resolving_version(const T& t, const uuid_t &us);

    void upgrade_version(const uuid_t &us);

    T get() const;

    T &get_mutable();

    std::vector<T> get_all_values() const;
};

//semilattice concept for vclock_t
template <class T>
bool operator==(const vclock_t<T> &, const vclock_t<T> &);

template <class T>
void semilattice_join(vclock_t<T> *, const vclock_t<T> &);

template <class T>
void debug_print(append_only_printf_buffer_t *buf, const vclock_t<T> &x);


// vclock context type for use with json adapters.
class vclock_ctx_t {
public:
    const uuid_t us;
    explicit vclock_ctx_t(uuid_t _us)
        : us(_us)
    { }
};

inline bool operator==(const vclock_ctx_t &x, const vclock_ctx_t &y) {
    return x.us == y.us;
}


#include "rpc/semilattice/joins/vclock.tcc"

#endif /* RPC_SEMILATTICE_JOINS_VCLOCK_HPP_ */
