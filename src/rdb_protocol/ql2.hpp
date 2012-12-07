// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_QL2_HPP_
#define RDB_PROTOCOL_QL2_HPP_

#include <stack>
#include <string>
#include <vector>
#include <map>

#include "utils.hpp"
#include "containers/uuid.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/stream_cache.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/ql2.pb.h"

class json_stream_t; // TODO: include

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

void _runtime_check(bool test, const char *teststr, std::string msg);
#define runtime_check(test, msg) _runtime_check(test, stringify(test), msg)

class datum_t {
public:
    datum_t(); // R_NULL
    datum_t(bool _bool);
    datum_t(double _num);
    datum_t(const std::string &_str);
    datum_t(const std::vector<const datum_t *> &_array);
    datum_t(const std::map<const std::string, const datum_t *> &_object);

    enum type_t {
        R_NULL   = 1,
        R_BOOL   = 2,
        R_NUM    = 3,
        R_STR    = 4,
        R_ARRAY  = 5,
        R_OBJECT = 6
    };
    type_t get_type() const;
    void check_type(type_t desired) const;

    bool as_bool() const;
    double as_num() const;
    const std::string &as_str() const;
    const std::vector<const datum_t *> &as_array() const;
    const std::map<const std::string, const datum_t *> &as_object() const;

    int cmp(const datum_t &rhs) const;
    bool operator==(const datum_t &rhs) const;
    bool operator!=(const datum_t &rhs) const;
    bool operator<(const datum_t &rhs) const;
    bool operator<=(const datum_t &rhs) const;
    bool operator>(const datum_t &rhs) const;
    bool operator>=(const datum_t &rhs) const;

    void add(datum_t *val);
    // Returns whether or not `key` was already present in object.
    MUST_USE bool add(const std::string &key, datum_t *val, bool clobber = false);
private:
    // Listing everything is more debugging-friendly than a boost::variant,
    type_t type;
    bool r_bool;
    double r_num;
    std::string r_str;
    std::vector<const datum_t *> r_array;
    std::map<const std::string, const datum_t *> r_object;
};

class func_t;
class val_t;
class term_t;

// namespace_repo_t<rdb_protocol_t>::access_t *
class table_t;

class val_t {
public:
    class type_t {
        friend class val_t;
    public:
        enum raw_type_t {
            DB               = 1, // db
            TABLE            = 2, // table
            SELECTION        = 3, // table, sequence
            SEQUENCE         = 4, // sequence
            SINGLE_SELECTION = 5, // table, datum (object)
            DATUM            = 6, // datum
            FUNC             = 7  // func
        };
        type_t(raw_type_t _raw_type);
        bool is_convertible(type_t rhs);
    private:
        raw_type_t raw_type;
    };

    uuid_t as_db();
    table_t *as_table();
    std::pair<table_t *, json_stream_t *> as_selection();
    json_stream_t *as_seq();
    std::pair<table_t *, const datum_t *> as_single_selection();
    const datum_t *as_datum();
    func_t *as_func();
private:
    type_t type;
    uuid_t db;
    table_t *table;
    scoped_ptr_t<json_stream_t> sequence;
    scoped_ptr_t<const datum_t> datum;
    scoped_ptr_t<func_t> func;

    bool consumed;
};

class func_t {
public:
    func_t(const std::vector<int> &args, const Term2 *body_source, env_t *env);
    val_t *call(const std::vector<datum_t *> &args);
private:
    std::vector<datum_t *> argptrs;
    scoped_ptr_t<term_t> body;
};

term_t *compile_term(const Term2 *source, env_t *env);

class term_t {
public:
    term_t();
    virtual ~term_t();

    //val_t *eval(bool use_cached_val = true);
    val_t *eval(bool use_cached_val = true) {
        runtime_check(false, strprintf("UNIMPLEMENTED %d", use_cached_val));
        return 0;
    }
private:
    val_t *cached_val;
    virtual val_t *eval_impl() = 0;
};

class datum_term_t : public term_t {
public:
    datum_term_t(const Datum *datum);
    virtual ~datum_term_t();

    virtual val_t *eval_impl();
private:
    scoped_ptr_t<val_t> raw_val;
};

class op_term_t : public term_t {
public:
    op_term_t(const Term2 *opterm);
    virtual ~op_term_t();
    virtual val_t *eval_impl();
private:
    val_t *call(std::vector<val_t *> *args,
                std::map<const std::string, val_t *> *optargs);
    virtual val_t *call_impl(std::vector<val_t *> *args,
                             std::map<const std::string, val_t *> *optargs) = 0;
    std::vector<term_t *> args;
    std::map<const std::string, term_t *> optargs;
};

class simple_op_term_t : public op_term_t {
public:
    simple_op_term_t(const Term2 *opterm);
    virtual ~simple_op_term_t();
    virtual val_t *call_impl(std::vector<val_t *> *args,
                             std::map<const std::string, val_t *> *optargs);
private:
    val_t *call(std::vector<val_t *> *args);
    virtual val_t *call_impl(std::vector<val_t *> *args) = 0;
};

// Fills in [res] with an error of type [type] and message [msg].
void fill_error(Response2 *res, Response2_ResponseType type, std::string msg);

void run(Query2 *q, env_t *env, Response2 *res, stream_cache_t *stream_cache);

} // namespace ql

#endif /* RDB_PROTOCOL_QL2_HPP_ */
