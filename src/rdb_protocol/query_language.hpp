#ifndef RDB_PROTOCOL_QUERY_LANGUAGE_HPP__
#define RDB_PROTOCOL_QUERY_LANGUAGE_HPP__

#include <list>
#include <deque>

#include "utils.hpp"
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "http/json.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/query_language.pb.h"

//TODO maybe we can merge well definedness and type checking. */

/* Make sure that the protocol buffers we receive are a well defined type. That
 * is they specifiy which type they are and have the correct optional fields
 * filled in (and none of the others). */

bool is_well_defined(const VarTermTuple &);

bool is_well_defined(const Term &);

bool is_well_defined(const Builtin &);

bool is_well_defined(const Reduction &);

bool is_well_defined(const Mapping &);

bool is_well_defined(const Predicate &);

bool is_well_defined(const View &);

bool is_well_defined(const Builtin &);

bool is_well_defined(const ReadQuery &);

bool is_well_defined(const WriteQuery &);

bool is_well_defined(const Query &);

namespace query_language {
struct error_t;
struct primitive_t;

typedef boost::variant<error_t, primitive_t> type_t;

struct error_t { 
    error_t()
        : desc("Unknown error.")
    { }

    explicit error_t(const std::string &_desc)
        : desc(_desc)
    { }

    std::string desc;

    bool operator==(const error_t &e) const {
        return desc == e.desc;
    }
};

/* TODO in what sense are these primitive types now? maybe these should just be
 * types. */
struct primitive_t {
    enum primitive_type_t {
        ERROR,
        JSON,
        STREAM,
        VIEW,
        READ,
        WRITE,
        QUERY
    } value;

    explicit primitive_t(primitive_type_t _value)
        : value(_value)
    { }

    bool operator==(const primitive_t &p) const {
        return value == p.value;
    }
};

inline type_t ERROR() {
    return type_t(primitive_t(primitive_t::ERROR));
}

inline type_t JSON() {
    return type_t(primitive_t(primitive_t::JSON));
}

inline type_t STREAM() {
    return type_t(primitive_t(primitive_t::STREAM));
}

inline type_t VIEW() {
    return type_t(primitive_t(primitive_t::VIEW));
}

inline type_t READ() {
    return type_t(primitive_t(primitive_t::READ));
}

inline type_t WRITE() {
    return type_t(primitive_t(primitive_t::WRITE));
}

inline type_t QUERY() {
    return type_t(primitive_t(primitive_t::WRITE));
}

typedef std::list<type_t> function_t;

template <class T>
class variable_scope_t {
public:
    void put_in_scope(std::string name, T t) {
        rassert(!scopes.empty());
        scopes.front()[name] = t;
    }

    T get(std::string name) {
        for (typename scopes_t::iterator it  = scopes.begin();
                                         it != scopes.end();
                                         ++it) {
            typename std::map<std::string, T>::iterator jt = it->find(name);
            if (jt != it->end()) {
                return jt->second;
            }
        }
        
        return error_t(strprintf("Symbol %s is not in scope\n", name.c_str()));
    }

    void push() {
        scopes.push_front(std::map<std::string, T>());
    }

    void pop() {
        scopes.pop_front();
    }

    struct new_scope_t {
        new_scope_t(variable_scope_t *_parent) 
            : parent(_parent)
        {
            parent->push();
        }
        ~new_scope_t() {
            parent->pop();
        }

        variable_scope_t *parent;
    };
private:
    typedef std::deque<std::map<std::string, T> > scopes_t;
    scopes_t scopes;
};

typedef variable_scope_t<type_t> variable_type_scope_t;

typedef variable_type_scope_t::new_scope_t new_scope_t;

/* get_type functions assume that the contained value is well defined. */
type_t get_type(const Term &t, variable_type_scope_t *scope);

function_t get_type(const Builtin &b, variable_type_scope_t *scope);

type_t get_type(const Reduction &r, variable_type_scope_t *scope);

type_t get_type(const Mapping &m, variable_type_scope_t *scope);

type_t get_type(const Predicate &p, variable_type_scope_t *scope);

type_t get_type(const View &v, variable_type_scope_t *scope);

type_t get_type(const ReadQuery &r, variable_type_scope_t *scope);

type_t get_type(const WriteQuery &w, variable_type_scope_t *scope);

type_t get_type(const Query &q, variable_type_scope_t *scope);

/* functions to evaluate the queries */

//typedef boost::variant<Response> eval_result;

typedef variable_scope_t<Term> variable_val_scope_t;

class runtime_exc_t {
public:
    runtime_exc_t(std::string _what)
        : what_happened(_what)
    { }

    std::string what() const throw() {
        return what_happened;
    }
private:
    std::string what_happened;
};

class runtime_environment_t {
public:
    runtime_environment_t(namespace_repo_t<rdb_protocol_t> *_ns_repo, 
                          boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _semilattice_metadata)
        : ns_repo(_ns_repo), semilattice_metadata(_semilattice_metadata)
    { }

    variable_val_scope_t scope;
    namespace_repo_t<rdb_protocol_t> *ns_repo;
    //TODO this should really just be the namespace metadata... but
    //constructing views is too hard :-/
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;
    cond_t interruptor;
};

Response eval(const Query &q, runtime_environment_t *);

Response eval(const ReadQuery &r, runtime_environment_t *) THROWS_ONLY(runtime_exc_t);

Response eval(const WriteQuery &r, runtime_environment_t *) THROWS_ONLY(runtime_exc_t);

boost::shared_ptr<scoped_cJSON_t> eval(const Term &t, runtime_environment_t *) THROWS_ONLY(runtime_exc_t);

boost::shared_ptr<scoped_cJSON_t> eval(const Term::Call &c, runtime_environment_t *) THROWS_ONLY(runtime_exc_t);

namespace_repo_t<rdb_protocol_t>::access_t eval(const TableRef &t, runtime_environment_t *) THROWS_ONLY(runtime_exc_t);

} //namespace query_language

#endif
