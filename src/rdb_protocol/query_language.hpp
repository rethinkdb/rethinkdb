#ifndef RDB_PROTOCOL_QUERY_LANGUAGE_HPP_
#define RDB_PROTOCOL_QUERY_LANGUAGE_HPP_

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

namespace Type {
    const type_t ERROR = primitive_t(primitive_t::ERROR);
    const type_t JSON = primitive_t(primitive_t::JSON);
    const type_t STREAM = primitive_t(primitive_t::STREAM);
    const type_t VIEW = primitive_t(primitive_t::VIEW);
    const type_t READ = primitive_t(primitive_t::READ);
    const type_t WRITE = primitive_t(primitive_t::WRITE);
    const type_t QUERY = primitive_t(primitive_t::WRITE);
}

class function_t {
public:
    // _n_args==-1 indicates a variadic function
    function_t(const type_t& _arg_type, int _n_args, const type_t& _return_type);

    const type_t& get_arg_type() const;
    const type_t& get_return_type() const;
    bool is_variadic() const;
    int get_n_args() const;
private:
    type_t arg_type;
    int n_args;
    type_t return_type;
};

template <class T>
class variable_scope_t {
public:
    void put_in_scope(const std::string &name, const T &t) {
        rassert(!scopes.empty());
        scopes.front()[name] = t;
    }

    T get(const std::string &name) {
        for (typename scopes_t::iterator it  = scopes.begin();
                                         it != scopes.end();
                                         ++it) {
            typename std::map<std::string, T>::iterator jt = it->find(name);
            if (jt != it->end()) {
                return jt->second;
            }
        }

        unreachable("Variable not in scope, probably because the code fails to call is_in_scope().");
    }

    // Calling this only makes sense in the typechecker. All variables
    // are guranteed by the typechecker to be present at runtime.
    bool is_in_scope(const std::string &name) {
        for (typename scopes_t::iterator it  = scopes.begin();
                                         it != scopes.end();
                                         ++it) {
            typename std::map<std::string, T>::iterator jt = it->find(name);
            if (jt != it->end()) {
                return true;
            }
        }
        return false;
    }

    void push() {
        scopes.push_front(std::map<std::string, T>());
    }

    void pop() {
        scopes.pop_front();
    }

    struct new_scope_t {
        explicit new_scope_t(variable_scope_t<T> *_parent)
            : parent(_parent)
        {
            parent->push();
        }
        ~new_scope_t() {
            parent->pop();
        }

        variable_scope_t<T> *parent;
    };
private:
    typedef std::deque<std::map<std::string, T> > scopes_t;
    scopes_t scopes;
};

typedef variable_scope_t<type_t> variable_type_scope_t;

typedef variable_type_scope_t::new_scope_t new_scope_t;

/* get_type functions assume that the contained value is well defined. */
const type_t get_type(const Term &t, variable_type_scope_t *scope);

const function_t get_type(const Builtin &b, variable_type_scope_t *scope);

const type_t get_type(const Reduction &r, variable_type_scope_t *scope);

const type_t get_type(const Mapping &m, variable_type_scope_t *scope);

const type_t get_type(const Predicate &p, variable_type_scope_t *scope);

const type_t get_type(const View &v, variable_type_scope_t *scope);

const type_t get_type(const ReadQuery &r, variable_type_scope_t *scope);

const type_t get_type(const WriteQuery &w, variable_type_scope_t *scope);

const type_t get_type(const Query &q, variable_type_scope_t *scope);

/* functions to evaluate the queries */

//typedef boost::variant<Response> eval_result;

typedef variable_scope_t<boost::shared_ptr<scoped_cJSON_t> > variable_val_scope_t;

typedef variable_scope_t<boost::shared_ptr<scoped_cJSON_t> >::new_scope_t new_val_scope_t;

class runtime_exc_t {
public:
    explicit runtime_exc_t(std::string _what)
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

boost::shared_ptr<scoped_cJSON_t> eval_cmp(const Term::Call &c, runtime_environment_t *) THROWS_ONLY(runtime_exc_t);

namespace_repo_t<rdb_protocol_t>::access_t eval(const TableRef &t, runtime_environment_t *) THROWS_ONLY(runtime_exc_t);

} //namespace query_language

#endif /* RDB_PROTOCOL_QUERY_LANGUAGE_HPP_ */
