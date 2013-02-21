#ifndef RDB_PROTOCOL_TERM_WALKER_
#define RDB_PROTOCOL_TERM_WALKER_

namespace ql {

#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/ql2_extensions.pb.h"

class term_walker_t {
public:
    term_walker_t(Term2 *root) : depth(0), writes_legal(true) {
        walk(root, 0, head_frame);
    }
    ~term_walker_t() {
        r_sanity_check(depth == 0);
        r_sanity_check(writes_legal == true);
    }

    void walk(Term2 *t, Term2 *parent, backtrace_t::frame_t frame) {
        val_pusher_t<int> depth_pusher(&depth, depth+1);
        add_bt(t, parent, frame);

        bool writes_still_legal = writes_are_still_legal(parent, frame);
        rcheck(writes_still_legal || !term_is_write_or_meta(t),
               strprintf("Cannot nest writes or meta ops in stream operations"), t);
        val_pusher_t<bool> writes_legal_pusher(&writes_legal, writes_still_legal);
        for (int i = 0; i < t->args_size(); ++i) {
            walk(t->mutable_args(i), t, backtrace_t::frame_t(i));
        }
        for (int i = 0; i < t->optargs_size(); ++i) {
            Term2_AssocPair *ap = t->mutable_optargs(i);
            walk(ap->mutable_val(), t, backtrace_t::frame_t(ap->key()));
        }
    }
private:
    void add_bt(Term2 *t, Term2 *parent, backtrace_t::frame_t frame) {
        r_sanity_check(t->ExtensionSize(ql2::extension::backtrace) == 0);
        if (parent) {
            for (int i = 0; i < parent->ExtensionSize(ql2::extension::backtrace); ++i) {
                *t->AddExtension(ql2::extension::backtrace)
                    = parent->GetExtension(ql2::extension::backtrace, i);
            }
        } else {
            r_sanity_check(frame.is_head());
        }
        *t->AddExtension(ql2::extension::backtrace) = frame.toproto();
    }

    static bool term_is_write_or_meta(Term2 *t) {
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
        if (term_forbids_writes(parent) && frame.is_stream_funcall_frame()) return false;
        return true;
    }
    static bool term_forbids_writes(Term2 *t) {
        return t->type() == Term2::REDUCE
            || t->type() == Term2::MAP
            || t->type() == Term2::FILTER
            || t->type() == Term2::CONCATMAP
            || t->type() == Term2::GROUPED_MAP_REDUCE
            || t->type() == Term2::GROUPBY
            || t->type() == Term2::INNER_JOIN
            || t->type() == Term2::OUTER_JOIN
            || t->type() == Term2::EQ_JOIN

            || t->type() == Term2::UPDATE
            || t->type() == Term2::DELETE
            || t->type() == Term2::REPLACE
            || t->type() == Term2::INSERT;
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
};

} // namespace ql

#endif // RDB_PROTOCOL_TERM_WALKER_
