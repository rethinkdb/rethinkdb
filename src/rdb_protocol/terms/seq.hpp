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
    count_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->count());
    }
    RDB_NAME("count");
};

class map_term_t : public op_term_t {
public:
    map_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->map(arg(1)->as_func()));
    }
    RDB_NAME("map");
};

class concatmap_term_t : public op_term_t {
public:
    concatmap_term_t(env_t *env, const Term2 *term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->concatmap(arg(1)->as_func()));
    }
    RDB_NAME("concatmap");
};

class filter_term_t : public op_term_t {
public:
    filter_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        val_t *v0 = arg(0), *v1 = arg(1);
        if (v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            std::pair<table_t *, datum_stream_t *> tds = v0->as_selection();
            return new_val(tds.first, tds.second->filter(v1->as_func(FILTER_SHORTCUT)));
        } else {
            return new_val(v0->as_seq()->filter(v1->as_func(FILTER_SHORTCUT)));
        }
    }
    RDB_NAME("filter");
};

static const char *const reduce_optargs[] = {"base"};
class reduce_term_t : public op_term_t {
public:
    reduce_term_t(env_t *env, const Term2 *term) :
        op_term_t(env, term, argspec_t(2), optargspec_t(reduce_optargs)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->reduce(optarg("base", 0), arg(1)->as_func()));
    }
    RDB_NAME("reduce");
};

// TODO: this sucks.  Change to use the same macros as rewrites.hpp?
static const char *const between_optargs[] = {"left_bound", "right_bound"};
class between_term_t : public op_term_t {
public:
    between_term_t(env_t *env, const Term2 *term)
        : op_term_t(env, term, argspec_t(1), optargspec_t(between_optargs)) { }
private:

    static void set_cmp(Term2 *out, int varnum, const std::string &pk,
                        Term2::TermType cmp_fn, const datum_t *cmp_to) {
        std::vector<Term2 *> cmp_args;
        pb::set(out, cmp_fn, &cmp_args, 2); {
            std::vector<Term2 *> ga_args;
            pb::set(cmp_args[0], Term2_TermType_GETATTR, &ga_args, 2); {
                pb::set_var(ga_args[0], varnum);
                scoped_ptr_t<datum_t> pkd(new datum_t(pk));
                pkd->write_to_protobuf(pb::set_datum(ga_args[1]));
            }
            cmp_to->write_to_protobuf(pb::set_datum(cmp_args[1]));
        }
    }

    virtual val_t *eval_impl() {
        std::pair<table_t *, datum_stream_t *> sel = arg(0)->as_selection();
        val_t *lb = optarg("left_bound", 0);
        val_t *rb = optarg("right_bound", 0);
        if (!lb && !rb) return new_val(sel.first, sel.second);

        table_t *tbl = sel.first;
        const std::string &pk = tbl->get_pkey();
        datum_stream_t *seq = sel.second;

        if (!filter_func.has()) {
            filter_func.init(new Term2());
            int varnum = env->gensym();
            Term2 *body = pb::set_func(filter_func.get(), varnum);
            std::vector<Term2 *> args;
            pb::set(body, Term2_TermType_ALL, &args, !!lb + !!rb);
            if (lb) set_cmp(args[0], varnum, pk, Term2_TermType_GE, lb->as_datum());
            if (rb) set_cmp(args[!!lb], varnum, pk, Term2_TermType_LE, rb->as_datum());
        }

        guarantee(filter_func.has());
        //debugf("%s\n", filter_func->DebugString().c_str());
        return new_val(tbl, seq->filter(env->new_func(filter_func.get())));
    }
    RDB_NAME("between");

    scoped_ptr_t<Term2> filter_func;
};

class union_term_t : public op_term_t {
public:
    union_term_t(env_t *env, const Term2 *term)
        : op_term_t(env, term, argspec_t(0, -1)) { }
private:
    virtual val_t *eval_impl() {
        std::vector<datum_stream_t *> streams;
        for (size_t i = 0; i < num_args(); ++i) streams.push_back(arg(i)->as_seq());
        return new_val(new union_datum_stream_t(env, streams, this));
    }
    RDB_NAME("union");
};

class zip_term_t : public op_term_t {
public:
    zip_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->zip());
    }
    RDB_NAME("zip");
};

} //namespace ql

#endif // RDB_PROTOCOL_TERMS_SEQ_HPP_
