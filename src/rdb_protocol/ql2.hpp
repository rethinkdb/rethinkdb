// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_QL2_HPP_
#define RDB_PROTOCOL_QL2_HPP_

#include <stack>
#include <string>
#include <vector>
#include <map>

#include "utils.hpp"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "containers/uuid.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/stream_cache.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

struct backtrace_t {
    struct frame_t {
    public:
        frame_t(int _pos) : type(POS), pos(_pos) { }
        frame_t(const std::string &_opt) : type(OPT), opt(_opt) { }
        Response2_Frame toproto() const;
    private:
        enum frame_type_t { POS = 0, OPT = 1 };
        frame_type_t type;
        int pos;
        std::string opt;
    };
    std::list<frame_t> frames;
};

class exc_t : public std::exception {
public:
    exc_t(const std::string &_msg) : msg(_msg) { }
    virtual ~exc_t() throw () { }
    backtrace_t backtrace;
    const char *what() const throw () { return msg.c_str(); }
private:
    const std::string msg;
};

void _runtime_check(const char *test, const char *file, int line,
                    bool pred, std::string msg = "");
#define runtime_check(pred, msg) \
    _runtime_check(stringify(pred), __FILE__, __LINE__, pred, msg)
// TODO: do something smarter?
#define runtime_fail(msg) runtime_check(false, msg)
// TODO: make this crash in debug mode
#define sanity_check(test) runtime_check(test, "SANITY_CHECK")

class term_t;
class func_t {
public:
    func_t(env_t *env, const std::vector<int> &args, const Term2 *body_source);
    val_t *call(env_t *env, const std::vector<datum_t *> &args);
private:
    std::vector<datum_t *> argptrs;
    scoped_ptr_t<term_t> body;
};

term_t *compile_term(env_t *env, const Term2 *t);

class term_t {
public:
    term_t();
    virtual ~term_t();

    virtual const char *name() const = 0;
    val_t *eval(env_t *env, bool use_cached_val = true);
private:
    virtual val_t *eval_impl(env_t *env) = 0;
    val_t *cached_val;
};

class datum_term_t : public term_t {
public:
    datum_term_t(const Datum *datum);
    virtual ~datum_term_t();

    virtual val_t *eval_impl(env_t *env);
    virtual const char *name() const;
private:
    scoped_ptr_t<val_t> raw_val;
};

class op_term_t : public term_t {
public:
    op_term_t(env_t *env, const Term2 *term);
    virtual ~op_term_t();
    virtual val_t *eval_impl(env_t *env);
private:
    virtual val_t *call_impl(env_t *env, boost::ptr_vector<term_t> *args,
                             boost::ptr_map<const std::string, term_t> *optargs) = 0;
    boost::ptr_vector<term_t> args;
    boost::ptr_map<const std::string, term_t> optargs;
    //std::vector<term_t *> args;
    //std::map<const std::string, term_t *> optargs;
};

class simple_op_term_t : public op_term_t {
public:
    simple_op_term_t(env_t *env, const Term2 *term);
    virtual ~simple_op_term_t();
    virtual val_t *call_impl(env_t *env, boost::ptr_vector<term_t> *args,
                             boost::ptr_map<const std::string, term_t> *optargs) = 0;
private:
    virtual val_t *simple_call_impl(env_t *env, std::vector<val_t *> *args) = 0;
};

class predicate_term_t : public simple_op_term_t {
public:
    predicate_term_t(env_t *env, const Term2 *term);
    virtual ~predicate_term_t();
    virtual val_t *simple_call_impl(env_t *env, std::vector<val_t *> *args);
    virtual const char *name() const;
private:
    const char *namestr;
    bool invert;
    bool (datum_t::*pred)(const datum_t &rhs) const;
};

// Fills in [res] with an error of type [type] and message [msg].
void fill_error(Response2 *res, Response2_ResponseType type, std::string msg);

void run(Query2 *q, env_t *env, Response2 *res, stream_cache_t *stream_cache);

} // namespace ql

#endif /* RDB_PROTOCOL_QL2_HPP_ */
