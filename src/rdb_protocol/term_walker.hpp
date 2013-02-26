#ifndef RDB_PROTOCOL_TERM_WALKER_
#define RDB_PROTOCOL_TERM_WALKER_

namespace ql {

#include "rdb_protocol/err.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/ql2_extensions.pb.h"

class term_walker_t : public rcheckable_t {
public:
    term_walker_t(Term2 *root) : depth(0), writes_legal(true), t(0), bt(0) {
        walk(root, 0, head_frame);
    }
    term_walker_t(Term2 *root, const Backtrace *_bt)
        : depth(0), writes_legal(true), t(0), bt(_bt) {
        propwalk(root, 0, head_frame);
    }
    ~term_walker_t() {
        r_sanity_check(depth == 0);
        r_sanity_check(writes_legal == true);
    }

    void walk(Term2 *_t, Term2 *parent, backtrace_t::frame_t frame) {
        r_sanity_check(!bt);
        t = _t; // THIS IS AVAILABLE IN PRIVATE FUNCTIONS

        val_pusher_t<int> depth_pusher(&depth, depth+1);
        add_bt(parent, frame);

        bool writes_still_legal = writes_are_still_legal(parent, frame);
        rcheck(writes_still_legal || !this_term_is_write_or_meta(),
               strprintf("Cannot nest writes or meta ops in stream operations"));
        val_pusher_t<bool> writes_legal_pusher(&writes_legal, writes_still_legal);

        maybe_datum_recurse();
        term_recurse(&term_walker_t::walk);
    }

    void propwalk(Term2 *_t, UNUSED Term2 *parent, UNUSED backtrace_t::frame_t frame) {
        r_sanity_check(bt);
        t = _t;

        if (t->ExtensionSize(ql2::extension::backtrace) == 0) {
            *t->MutableExtension(ql2::extension::backtrace) = *bt;
        }
    }
private:
    void maybe_datum_recurse() {
        if (t->type() == Term2::DATUM) {
            Datum *d = t->mutable_datum();
            *d->MutableExtension(ql2::extension::datum_backtrace)
                = t->GetExtension(ql2::extension::backtrace);
        }
    }

    void term_recurse(void (term_walker_t::*callback)(Term2 *, Term2 *,
                                                      backtrace_t::frame_t)) {
        for (int i = 0; i < t->args_size(); ++i) {
            (this->*callback)(t->mutable_args(i), t, backtrace_t::frame_t(i));
        }
        for (int i = 0; i < t->optargs_size(); ++i) {
            Term2_AssocPair *ap = t->mutable_optargs(i);
            (this->*callback)(ap->mutable_val(), t, backtrace_t::frame_t(ap->key()));
        }
    }

    // NOTE: This is a thin wrapper around `runtime_check` in err.hpp which adds
    // the source of the current term to the argument list.  It's usually
    // invoked by `rcheck`.
    void runtime_check(const char *test, const char *file, int line,
                       bool pred, std::string msg) const {
        ql::runtime_check(test, file, line, pred, msg,
                          &t->GetExtension(ql2::extension::backtrace));
    }

    void add_bt(Term2 *parent, backtrace_t::frame_t frame) {
        r_sanity_check(t->ExtensionSize(ql2::extension::backtrace) == 0);
        if (parent) {
            *t->MutableExtension(ql2::extension::backtrace)
                = parent->GetExtension(ql2::extension::backtrace);
        } else {
            r_sanity_check(frame.is_head());
        }
        *t->MutableExtension(ql2::extension::backtrace)->add_frames() = frame.toproto();
    }

    bool this_term_is_write_or_meta() {
        return t->type() == Term2::UPDATE
            || t->type() == Term2::DELETE
            || t->type() == Term2::INSERT
            || t->type() == Term2::REPLACE

            || t->type() == Term2::DB_CREATE
            || t->type() == Term2::DB_DROP
            || t->type() == Term2::DB_LIST
            || t->type() == Term2::TABLE_CREATE
            || t->type() == Term2::TABLE_DROP
            || t->type() == Term2::TABLE_LIST;
    }

    bool writes_are_still_legal(Term2 *parent, backtrace_t::frame_t frame) {
        if (!writes_legal) return false; // writes never become legal again
        if (!parent) return true; // writes legal at root of tree
        if (term_forbids_writes(parent) && frame.is_stream_funcall_frame()) {
            return false;
        }
        return true;
    }
    static bool term_forbids_writes(Term2 *term) {
        return term->type() == Term2::REDUCE
            || term->type() == Term2::MAP
            || term->type() == Term2::FILTER
            || term->type() == Term2::CONCATMAP
            || term->type() == Term2::GROUPED_MAP_REDUCE
            || term->type() == Term2::GROUPBY
            || term->type() == Term2::INNER_JOIN
            || term->type() == Term2::OUTER_JOIN
            || term->type() == Term2::EQ_JOIN

            || term->type() == Term2::UPDATE
            || term->type() == Term2::DELETE
            || term->type() == Term2::REPLACE
            || term->type() == Term2::INSERT;
    }

    template<class T>
    class val_pusher_t {
    public:
        val_pusher_t(T *_val_ptr, T new_val) : val_ptr(_val_ptr) {
            old_val = *val_ptr;
            *val_ptr = new_val;
        }
        ~val_pusher_t() {
            *val_ptr = old_val;
        }
    private:
        T old_val;
        T *val_ptr;
    };

    int depth;
    bool writes_legal;
    Term2 *t;
    const Backtrace *bt;
};

} // namespace ql

#endif // RDB_PROTOCOL_TERM_WALKER_
