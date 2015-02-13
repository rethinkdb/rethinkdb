// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/term_walker.hpp"

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/pseudo_time.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/ql2_extensions.pb.h"

namespace ql {

// We use this class to walk a term and do something to every node.
class term_walker_t {
public:
    // This constructor fills in the backtraces of a term (`walk`) and checks
    // that it's well-formed with regard to write placement.
    explicit term_walker_t(Term *root)
        : depth(0), writes_legal(true), bt(0) {
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

        if (t->type() == Term::NOW && t->args_size() == 0) {
            // Construct curtime the first time we access it
            if (!curtime.has()) {
                curtime = pseudo::time_now();
            }
            *t = r::expr(curtime).get();
        }

        if (t->type() == Term::ASC || t->type() == Term::DESC) {
            rcheck_src(&t->GetExtension(ql2::extension::backtrace),
                       parent && parent->type() == Term::ORDER_BY,
                       base_exc_t::GENERIC,
                       strprintf("%s may only be used as an argument to ORDER_BY.",
                                 (t->type() == Term::ASC ? "ASC" : "DESC")));
        }

        bool writes_still_legal = writes_are_still_legal(parent, frame);
        rcheck_src(&t->GetExtension(ql2::extension::backtrace),
                   writes_still_legal || !term_is_write_or_meta(t),
                   base_exc_t::GENERIC,
                   strprintf("Cannot nest writes or meta ops in stream operations.  "
                             "Use FOR_EACH instead."));
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
        switch (t->type()) {
        case Term::UPDATE:
        case Term::DELETE:
        case Term::INSERT:
        case Term::REPLACE:
        case Term::DB_CREATE:
        case Term::DB_DROP:
        case Term::TABLE_CREATE:
        case Term::TABLE_DROP:
        case Term::WAIT:
        case Term::RECONFIGURE:
        case Term::REBALANCE:
        case Term::SYNC:
        case Term::INDEX_CREATE:
        case Term::INDEX_DROP:
        case Term::INDEX_WAIT:
        case Term::INDEX_RENAME:
            return true;

        case Term::DATUM:
        case Term::MAKE_ARRAY:
        case Term::MAKE_OBJ:
        case Term::BINARY:
        case Term::VAR:
        case Term::JAVASCRIPT:
        case Term::HTTP:
        case Term::ERROR:
        case Term::IMPLICIT_VAR:
        case Term::RANDOM:
        case Term::DB:
        case Term::TABLE:
        case Term::GET:
        case Term::GET_ALL:
        case Term::EQ:
        case Term::NE:
        case Term::LT:
        case Term::LE:
        case Term::GT:
        case Term::GE:
        case Term::NOT:
        case Term::ADD:
        case Term::SUB:
        case Term::MUL:
        case Term::DIV:
        case Term::MOD:
        case Term::APPEND:
        case Term::PREPEND:
        case Term::DIFFERENCE:
        case Term::SET_INSERT:
        case Term::SET_INTERSECTION:
        case Term::SET_UNION:
        case Term::SET_DIFFERENCE:
        case Term::SLICE:
        case Term::OFFSETS_OF:
        case Term::GET_FIELD:
        case Term::HAS_FIELDS:
        case Term::PLUCK:
        case Term::WITHOUT:
        case Term::MERGE:
        case Term::LITERAL:
        case Term::BETWEEN:
        case Term::CHANGES:
        case Term::REDUCE:
        case Term::MAP:
        case Term::FILTER:
        case Term::CONCAT_MAP:
        case Term::GROUP:
        case Term::ORDER_BY:
        case Term::DISTINCT:
        case Term::COUNT:
        case Term::SUM:
        case Term::AVG:
        case Term::MIN:
        case Term::MAX:
        case Term::UNION:
        case Term::NTH:
        case Term::BRACKET:
        case Term::ARGS:
        case Term::LIMIT:
        case Term::SKIP:
        case Term::INNER_JOIN:
        case Term::OUTER_JOIN:
        case Term::EQ_JOIN:
        case Term::ZIP:
        case Term::RANGE:
        case Term::INSERT_AT:
        case Term::DELETE_AT:
        case Term::CHANGE_AT:
        case Term::SPLICE_AT:
        case Term::COERCE_TO:
        case Term::UNGROUP:
        case Term::TYPE_OF:
        case Term::FUNCALL:
        case Term::BRANCH:
        case Term::ANY:
        case Term::ALL:
        case Term::FOR_EACH:
        case Term::FUNC:
        case Term::ASC:
        case Term::DESC:
        case Term::INFO:
        case Term::MATCH:
        case Term::SPLIT:
        case Term::UPCASE:
        case Term::DOWNCASE:
        case Term::SAMPLE:
        case Term::IS_EMPTY:
        case Term::DEFAULT:
        case Term::CONTAINS:
        case Term::KEYS:
        case Term::OBJECT:
        case Term::WITH_FIELDS:
        case Term::JSON:
        case Term::TO_JSON_STRING:
        case Term::ISO8601:
        case Term::TO_ISO8601:
        case Term::EPOCH_TIME:
        case Term::TO_EPOCH_TIME:
        case Term::NOW:
        case Term::IN_TIMEZONE:
        case Term::DURING:
        case Term::DATE:
        case Term::TIME_OF_DAY:
        case Term::TIMEZONE:
        case Term::TIME:
        case Term::YEAR:
        case Term::MONTH:
        case Term::DAY:
        case Term::DAY_OF_WEEK:
        case Term::DAY_OF_YEAR:
        case Term::HOURS:
        case Term::MINUTES:
        case Term::SECONDS:
        case Term::MONDAY:
        case Term::TUESDAY:
        case Term::WEDNESDAY:
        case Term::THURSDAY:
        case Term::FRIDAY:
        case Term::SATURDAY:
        case Term::SUNDAY:
        case Term::JANUARY:
        case Term::FEBRUARY:
        case Term::MARCH:
        case Term::APRIL:
        case Term::MAY:
        case Term::JUNE:
        case Term::JULY:
        case Term::AUGUST:
        case Term::SEPTEMBER:
        case Term::OCTOBER:
        case Term::NOVEMBER:
        case Term::DECEMBER:
        case Term::DB_LIST:
        case Term::TABLE_LIST:
        case Term::CONFIG:
        case Term::STATUS:
        case Term::INDEX_LIST:
        case Term::INDEX_STATUS:
        case Term::GEOJSON:
        case Term::TO_GEOJSON:
        case Term::POINT:
        case Term::LINE:
        case Term::POLYGON:
        case Term::DISTANCE:
        case Term::INTERSECTS:
        case Term::INCLUDES:
        case Term::CIRCLE:
        case Term::GET_INTERSECTING:
        case Term::FILL:
        case Term::GET_NEAREST:
        case Term::UUID:
        case Term::POLYGON_SUB:
            return false;
        default: unreachable();
        }
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
        switch (term->type()) {
        case Term::REDUCE:
        case Term::MAP:
        case Term::FILTER:
        case Term::CONCAT_MAP:
        case Term::GROUP:
        case Term::INNER_JOIN:
        case Term::OUTER_JOIN:
        case Term::EQ_JOIN:
        case Term::UPDATE:
        case Term::DELETE:
        case Term::REPLACE:
        case Term::INSERT:
        case Term::COUNT:
        case Term::SUM:
        case Term::AVG:
        case Term::MIN:
        case Term::MAX:
            return true;

        case Term::DATUM:
        case Term::MAKE_ARRAY:
        case Term::MAKE_OBJ:
        case Term::BINARY:
        case Term::VAR:
        case Term::JAVASCRIPT:
        case Term::HTTP:
        case Term::ERROR:
        case Term::IMPLICIT_VAR:
        case Term::RANDOM:
        case Term::DB:
        case Term::TABLE:
        case Term::GET:
        case Term::GET_ALL:
        case Term::EQ:
        case Term::NE:
        case Term::LT:
        case Term::LE:
        case Term::GT:
        case Term::GE:
        case Term::NOT:
        case Term::ADD:
        case Term::SUB:
        case Term::MUL:
        case Term::DIV:
        case Term::MOD:
        case Term::APPEND:
        case Term::PREPEND:
        case Term::DIFFERENCE:
        case Term::SET_INSERT:
        case Term::SET_INTERSECTION:
        case Term::SET_UNION:
        case Term::SET_DIFFERENCE:
        case Term::SLICE:
        case Term::OFFSETS_OF:
        case Term::GET_FIELD:
        case Term::HAS_FIELDS:
        case Term::PLUCK:
        case Term::WITHOUT:
        case Term::MERGE:
        case Term::ARGS:
        case Term::LITERAL:
        case Term::BETWEEN:
        case Term::CHANGES:
        case Term::ORDER_BY:
        case Term::DISTINCT:
        case Term::UNION:
        case Term::NTH:
        case Term::BRACKET:
        case Term::LIMIT:
        case Term::SKIP:
        case Term::ZIP:
        case Term::RANGE:
        case Term::INSERT_AT:
        case Term::DELETE_AT:
        case Term::CHANGE_AT:
        case Term::SPLICE_AT:
        case Term::COERCE_TO:
        case Term::UNGROUP:
        case Term::TYPE_OF:
        case Term::DB_CREATE:
        case Term::DB_DROP:
        case Term::DB_LIST:
        case Term::TABLE_CREATE:
        case Term::TABLE_DROP:
        case Term::TABLE_LIST:
        case Term::CONFIG:
        case Term::STATUS:
        case Term::WAIT:
        case Term::RECONFIGURE:
        case Term::REBALANCE:
        case Term::SYNC:
        case Term::INDEX_CREATE:
        case Term::INDEX_DROP:
        case Term::INDEX_LIST:
        case Term::INDEX_STATUS:
        case Term::INDEX_WAIT:
        case Term::INDEX_RENAME:
        case Term::FUNCALL:
        case Term::BRANCH:
        case Term::ANY:
        case Term::ALL:
        case Term::FOR_EACH:
        case Term::FUNC:
        case Term::ASC:
        case Term::DESC:
        case Term::INFO:
        case Term::MATCH:
        case Term::SPLIT:
        case Term::UPCASE:
        case Term::DOWNCASE:
        case Term::SAMPLE:
        case Term::IS_EMPTY:
        case Term::DEFAULT:
        case Term::CONTAINS:
        case Term::KEYS:
        case Term::OBJECT:
        case Term::WITH_FIELDS:
        case Term::JSON:
        case Term::TO_JSON_STRING:
        case Term::ISO8601:
        case Term::TO_ISO8601:
        case Term::EPOCH_TIME:
        case Term::TO_EPOCH_TIME:
        case Term::NOW:
        case Term::IN_TIMEZONE:
        case Term::DURING:
        case Term::DATE:
        case Term::TIME_OF_DAY:
        case Term::TIMEZONE:
        case Term::TIME:
        case Term::YEAR:
        case Term::MONTH:
        case Term::DAY:
        case Term::DAY_OF_WEEK:
        case Term::DAY_OF_YEAR:
        case Term::HOURS:
        case Term::MINUTES:
        case Term::SECONDS:
        case Term::MONDAY:
        case Term::TUESDAY:
        case Term::WEDNESDAY:
        case Term::THURSDAY:
        case Term::FRIDAY:
        case Term::SATURDAY:
        case Term::SUNDAY:
        case Term::JANUARY:
        case Term::FEBRUARY:
        case Term::MARCH:
        case Term::APRIL:
        case Term::MAY:
        case Term::JUNE:
        case Term::JULY:
        case Term::AUGUST:
        case Term::SEPTEMBER:
        case Term::OCTOBER:
        case Term::NOVEMBER:
        case Term::DECEMBER:
        case Term::GEOJSON:
        case Term::TO_GEOJSON:
        case Term::POINT:
        case Term::LINE:
        case Term::POLYGON:
        case Term::DISTANCE:
        case Term::INTERSECTS:
        case Term::INCLUDES:
        case Term::CIRCLE:
        case Term::GET_INTERSECTING:
        case Term::FILL:
        case Term::GET_NEAREST:
        case Term::UUID:
        case Term::POLYGON_SUB:
            return false;
        default: unreachable();
        }
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
    const Backtrace *const bt;
    datum_t curtime;
};

void preprocess_term(Term *root) {
    term_walker_t walker(root);
}

void propagate_backtrace(Term *root, const Backtrace *bt) {
    term_walker_t walker(root, bt);
}


}  // namespace ql
