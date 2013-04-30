#ifndef RDB_PROTOCOL_TERMS_SEQ_HPP_
#define RDB_PROTOCOL_TERMS_SEQ_HPP_

#include <string>
#include <utility>
#include <vector>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

// Most of the real logic for these is in datum_stream.cc.

class count_term_t : public op_term_t {
public:
    count_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->count());
    }
    virtual const char *name() const { return "count"; }
};

class map_term_t : public op_term_t {
public:
    map_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->map(arg(1)->as_func()));
    }
    virtual const char *name() const { return "map"; }
};

class concatmap_term_t : public op_term_t {
public:
    concatmap_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->concatmap(arg(1)->as_func()));
    }
    virtual const char *name() const { return "concatmap"; }
};

class filter_term_t : public op_term_t {
public:
    filter_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        val_t *v0 = arg(0), *v1 = arg(1);
        func_t *f = v1->as_func(IDENTITY_SHORTCUT);
        if (v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            std::pair<table_t *, datum_stream_t *> ts = v0->as_selection();
            return new_val(ts.second->filter(f), ts.first);
        } else {
            return new_val(v0->as_seq()->filter(f));
        }
    }
    virtual const char *name() const { return "filter"; }
};

static const char *const reduce_optargs[] = {"base"};
class reduce_term_t : public op_term_t {
public:
    reduce_term_t(env_t *env, const Term *term) :
        op_term_t(env, term, argspec_t(2), optargspec_t(reduce_optargs)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->reduce(optarg("base", 0), arg(1)->as_func()));
    }
    virtual const char *name() const { return "reduce"; }
};

// TODO: this sucks.  Change to use the same macros as rewrites.hpp?
static const char *const between_optargs[] = {"index"};
class between_term_t : public op_term_t {
public:
    between_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(3), optargspec_t(between_optargs)) { }
private:
    virtual val_t *eval_impl() {
        table_t *tbl = arg(0)->as_table();
        const datum_t *lb = arg(1)->as_datum();
        lb = (lb->get_type() != datum_t::R_NULL) ? lb : NULL;
        const datum_t *rb = arg(2)->as_datum();
        rb = (rb->get_type() != datum_t::R_NULL) ? rb : NULL;
        if (!lb && !rb) {
            return new_val(tbl->as_datum_stream(), tbl);
        }

        val_t *sindex = optarg("index", 0);
        if (sindex != NULL) {
            std::string sid = sindex->as_str();
            if (sid != tbl->get_pkey()) {
                return new_val(tbl->get_sindex_rows(lb, rb, sid, this), tbl);
            }
        }

        return new_val(tbl->get_rows(lb, rb, this), tbl);
    }
    virtual const char *name() const { return "between"; }

    scoped_ptr_t<Term> filter_func;
};

class union_term_t : public op_term_t {
public:
    union_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(0, -1)) { }
private:
    virtual val_t *eval_impl() {
        std::vector<datum_stream_t *> streams;
        for (size_t i = 0; i < num_args(); ++i) streams.push_back(arg(i)->as_seq());
        return new_val(new union_datum_stream_t(env, streams, this));
    }
    virtual const char *name() const { return "union"; }
};

class zip_term_t : public op_term_t {
public:
    zip_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->zip());
    }
    virtual const char *name() const { return "zip"; }
};

} //namespace ql

#endif // RDB_PROTOCOL_TERMS_SEQ_HPP_
