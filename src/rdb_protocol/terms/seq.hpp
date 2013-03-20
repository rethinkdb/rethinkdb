#ifndef RDB_PROTOCOL_TERMS_SEQ_HPP_
#define RDB_PROTOCOL_TERMS_SEQ_HPP_

#include <string>
#include <utility>
#include <vector>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
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
        : op_term_t(env, term, argspec_t(2)), src_term(term) { }
private:
    virtual val_t *eval_impl() {
        val_t *v0 = arg(0), *v1 = arg(1);
        counted_t<func_t> f = v1->as_func(IDENTITY_SHORTCUT);
        if (v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            std::pair<counted_t<table_t>, counted_t<datum_stream_t> > ts = v0->as_selection();
            return new_val(ts.first, ts.second->filter(f));
        } else {
            return new_val(v0->as_seq()->filter(f));
        }
    }
    virtual const char *name() const { return "filter"; }

    const Term *src_term;
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
static const char *const between_optargs[] = {"left_bound", "right_bound"};
class between_term_t : public op_term_t {
public:
    between_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(1), optargspec_t(between_optargs)) { }
private:

    static void set_cmp(Term *out, int varnum, const std::string &pk,
                        Term::TermType cmp_fn, counted_t<const datum_t> cmp_to) {
        std::vector<Term *> cmp_args;
        pb::set(out, cmp_fn, &cmp_args, 2); {
            std::vector<Term *> ga_args;
            pb::set(cmp_args[0], Term_TermType_GETATTR, &ga_args, 2); {
                pb::set_var(ga_args[0], varnum);
                scoped_ptr_t<datum_t> pkd(new datum_t(pk));
                pkd->write_to_protobuf(pb::set_datum(ga_args[1]));
            }
            cmp_to->write_to_protobuf(pb::set_datum(cmp_args[1]));
        }
    }

    virtual val_t *eval_impl() {
        std::pair<counted_t<table_t>, counted_t<datum_stream_t> > sel = arg(0)->as_selection();
        val_t *lb = optarg("left_bound", 0);
        val_t *rb = optarg("right_bound", 0);
        if (!lb && !rb) return new_val(sel.first, sel.second);

        counted_t<table_t> tbl = sel.first;
        const std::string &pk = tbl->get_pkey();
        counted_t<datum_stream_t> seq = sel.second;

        if (!filter_func.has()) {
            filter_func.init(new Term());
            int varnum = env->gensym();
            Term *body = pb::set_func(filter_func.get(), varnum);
            std::vector<Term *> args;
            pb::set(body, Term_TermType_ALL, &args, !!lb + !!rb);
            if (lb) set_cmp(args[0], varnum, pk, Term_TermType_GE, lb->as_datum());
            if (rb) set_cmp(args[!!lb], varnum, pk, Term_TermType_LE, rb->as_datum());
        }

        guarantee(filter_func.has());
        //debugf("%s\n", filter_func->DebugString().c_str());
        return new_val(tbl, seq->filter(env->new_func(filter_func.get())));
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
        std::vector<counted_t<datum_stream_t> > streams;
        for (size_t i = 0; i < num_args(); ++i) {
            streams.push_back(arg(i)->as_seq());
        }
        counted_t<datum_stream_t> union_stream(new union_datum_stream_t(env, streams, this));
        return new_val(union_stream);
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
