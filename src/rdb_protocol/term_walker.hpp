#ifndef RDB_PROTOCOL_TERM_WALKER_HPP_
#define RDB_PROTOCOL_TERM_WALKER_HPP_

namespace ql {

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/ql2_extensions.pb.h"

// We use this class to walk a term and do something to every node.
class term_walker_t {
public:
    // This constructor fills in the backtraces of a term (`walk`) and checks
    // that it's well-formed with regard to write placement.
    explicit term_walker_t(Term *root) : depth(0), writes_legal(true), bt(0) {
        walk(root, 0, head_frame);
    }

    // This constructor propagates a backtrace down a tree until it hits a node
    // that already has a backtrace (this is used for e.g. rewrite terms so that
    // they return reasonable backtraces in the macroexpanded nodes).
    term_walker_t(Term *root, const Backtrace *_bt)
        : depth(0), writes_legal(true), bt(_bt) {
        propwalk(root, 0, head_frame);
    }
    ~term_walker_t() {
        r_sanity_check(depth == 0);
        r_sanity_check(writes_legal == true);
    }

    void walk(Term *t, Term *parent, backtrace_t::frame_t frame) {
        r_sanity_check(!bt);

        val_pusher_t<int> depth_pusher(&depth, depth+1);
        add_bt(t, parent, frame);

        if (t->type() == Term::ASC || t->type() == Term::DESC) {
            rcheck_src(&t->GetExtension(ql2::extension::backtrace),
                       parent && parent->type() == Term::ORDERBY,
                       strprintf("%s may only be used as an argument to ORDERBY.",
                                 (t->type() == Term::ASC ? "ASC" : "DESC")));
        }

        bool writes_still_legal = writes_are_still_legal(parent, frame);
        rcheck_src(&t->GetExtension(ql2::extension::backtrace),
                   writes_still_legal || !term_is_write_or_meta(t),
                   strprintf("Cannot nest writes or meta ops in stream operations.  "
                             "Use FOREACH instead."));
        val_pusher_t<bool> writes_legal_pusher(&writes_legal, writes_still_legal);

        term_recurse(t, &term_walker_t::walk);
    }

    void propwalk(Term *t, UNUSED Term *parent, UNUSED backtrace_t::frame_t frame) {
        r_sanity_check(bt);

        if (!t->HasExtension(ql2::extension::backtrace)) {
            *t->MutableExtension(ql2::extension::backtrace) = *bt;
            term_recurse(t, &term_walker_t::propwalk);
        }
    }
private:
    // Recurses to child terms.
    void term_recurse(Term *t, void (term_walker_t::*callback)(Term *, Term *,
                                                                backtrace_t::frame_t)) {
        for (int i = 0; i < t->args_size(); ++i) {
            (this->*callback)(t->mutable_args(i), t, backtrace_t::frame_t(i));
        }
        for (int i = 0; i < t->optargs_size(); ++i) {
            Term_AssocPair *ap = t->mutable_optargs(i);
            (this->*callback)(ap->mutable_val(), t, backtrace_t::frame_t(ap->key()));
        }
    }

    // Adds a backtrace to a term.
    void add_bt(Term *t, Term *parent, backtrace_t::frame_t frame) {
        r_sanity_check(t->ExtensionSize(ql2::extension::backtrace) == 0);
        if (parent) {
            *t->MutableExtension(ql2::extension::backtrace)
                = parent->GetExtension(ql2::extension::backtrace);
        } else {
            r_sanity_check(frame.is_head());
        }
        *t->MutableExtension(ql2::extension::backtrace)->add_frames() = frame.toproto();
    }

    // Returns true if `t` is a write or a meta op.
    static bool term_is_write_or_meta(Term *t) {
        return t->type() == Term::UPDATE
            || t->type() == Term::DELETE
            || t->type() == Term::INSERT
            || t->type() == Term::REPLACE

            || t->type() == Term::DB_CREATE
            || t->type() == Term::DB_DROP
            || t->type() == Term::DB_LIST
            || t->type() == Term::TABLE_CREATE
            || t->type() == Term::TABLE_DROP
            || t->type() == Term::TABLE_LIST;
    }

    // Returns true if writes are still legal at this node.  Basically:
    // * Once writes become illegal, they are never legal again.
    // * Writes are legal at the root.
    // * If the parent term forbids writes in its function arguments AND we
    //   aren't inside the 0th argument, writes are forbidden.
    // * Writes are legal in all other cases.
    bool writes_are_still_legal(Term *parent, backtrace_t::frame_t frame) {
        if (!writes_legal) return false; // writes never become legal again
        if (!parent) return true; // writes legal at root of tree
        if (term_forbids_writes(parent) && frame.is_stream_funcall_frame()) {
            return false;
        }
        return true;
    }
    static bool term_forbids_writes(Term *term) {
        return term->type() == Term::REDUCE
            || term->type() == Term::MAP
            || term->type() == Term::FILTER
            || term->type() == Term::CONCATMAP
            || term->type() == Term::GROUPED_MAP_REDUCE
            || term->type() == Term::GROUPBY
            || term->type() == Term::INNER_JOIN
            || term->type() == Term::OUTER_JOIN
            || term->type() == Term::EQ_JOIN

            || term->type() == Term::UPDATE
            || term->type() == Term::DELETE
            || term->type() == Term::REPLACE
            || term->type() == Term::INSERT;
    }

    // We use this class to change a value while recursing, then restore it to
    // its previous value.
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
    const Backtrace *bt;
};

} // namespace ql

#endif // RDB_PROTOCOL_TERM_WALKER_HPP_
