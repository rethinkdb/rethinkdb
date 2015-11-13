// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/term_walker.hpp"

#include "rdb_protocol/backtrace.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/pseudo_time.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/term_storage.hpp"

namespace ql {

bool term_type_is_valid(Term::TermType type);
bool term_is_write_or_meta(Term::TermType type);
bool term_forbids_writes(Term::TermType type);

// The minimum amount of stack space we require to be available on a coroutine
// before attempting to walk into another term.
const size_t MIN_WALK_STACK_SPACE = 16 * KILOBYTE;

// Walk the raw JSON term tree, editing it along the way - adding
// backtraces, rewriting certain terms, and verifying correct placement
// of some terms.

class term_walker_t {
public:
    term_walker_t(rapidjson::Value::AllocatorType *_allocator,
                  backtrace_registry_t *_bt_reg) :
        bt_reg(_bt_reg), allocator(_allocator) { }

    void walk(rapidjson::Value *src) {
        r_sanity_check(frames.size() == 0);
        walker_frame_t toplevel_frame(this, true, nullptr);
        toplevel_frame.walk(src, backtrace_id_t::empty());
    }

private:
    class walker_frame_t : public intrusive_list_node_t<walker_frame_t> {
    public:
        walker_frame_t(term_walker_t *_parent,
                       bool is_zeroth_argument,
                       const walker_frame_t *_prev_frame) :
            parent(_parent),
            writes_legal(true),
            prev_frame(_prev_frame)
        {
            if (prev_frame != nullptr) {
                writes_legal = prev_frame->writes_legal &&
                    (is_zeroth_argument || !term_forbids_writes(prev_frame->type));
            }
            parent->frames.push_back(this);
        }

        ~walker_frame_t() {
            parent->frames.remove(this);
        }

        void rewrite(rapidjson::Value *src, Term::TermType _type) {
            type = _type;
            rapidjson::Value rewritten;
            rewritten.SetArray();
            rewritten.Reserve(2, *parent->allocator);
            rewritten.PushBack(rapidjson::Value(static_cast<int>(type)),
                               *parent->allocator);
            src->Swap(rewritten);
            src->PushBack(std::move(rewritten), *parent->allocator);
        }

        void walk(rapidjson::Value *src, backtrace_id_t bt) {
            r_sanity_check(src != nullptr);
            rapidjson::Value *args = nullptr;
            rapidjson::Value *optargs = nullptr;

            if (src->IsArray()) {
                size_t size = src->Size();
                rcheck_src(bt, size >= 1 && size <= 3, base_exc_t::LOGIC,
                    strprintf("Expected between 1 and 3 elements in a raw term, "
                              "but found %zu.", size));
                        
                if (size >= 1) {
                    rapidjson::Value *val = &(*src)[0];
                    rcheck_src(bt, val->IsNumber(), base_exc_t::LOGIC,
                        strprintf("Expected a TermType as a NUMBER but found %s.",
                                  rapidjson_typestr(val->GetType())));
                    type = static_cast<Term::TermType>(val->GetInt());
                    rcheck_src(bt, term_type_is_valid(type), base_exc_t::LOGIC,
                               strprintf("Unrecognized TermType: %d.", val->GetInt()));
                }
                if (size >= 2) {
                    rapidjson::Value *val = &(*src)[1];
                    if (val->IsArray()) {
                        args = val;
                    } else if (val->IsObject() && type != Term::DATUM) {
                        optargs = val;
                    } else {
                        rcheck_src(bt, type == Term::DATUM, base_exc_t::LOGIC,
                                   "Second element in a non-DATUM Term is neither "
                                   "arguments nor optional arguments.");
                    }
                }
                if (size >= 3) {
                    rapidjson::Value *val = &(*src)[2];
                    rcheck_src(bt, val->IsObject(), base_exc_t::LOGIC,
                               "Third element in a non-DATUM Term is not "
                               "optional arguments.");
                    optargs = val;
                }
            } else if (src->IsObject()) {
                rewrite(src, Term::MAKE_OBJ);
                optargs = &(*src)[1];
            } else {
                rewrite(src, Term::DATUM);
            }

            if (type == Term::ASC || type == Term::DESC) {
                rcheck_src(bt,
                    prev_frame != nullptr && prev_frame->type == Term::ORDER_BY,
                    base_exc_t::LOGIC,
                    strprintf("%s may only be used as an argument to ORDER_BY.",
                              (type == Term::ASC ? "ASC" : "DESC")));
            } else if (type == Term::NOW) {
                // Set r.now() terms to the same literal time so it can be deteministic
                type = Term::DATUM;
                rapidjson::Value rewritten;
                rewritten.SetArray();
                rewritten.Reserve(2, *parent->allocator);
                rewritten.PushBack(rapidjson::Value(static_cast<int>(type)),
                                   *parent->allocator);
                rewritten.PushBack(get_time_now().as_json(parent->allocator),
                                   *parent->allocator);
                src->Swap(rewritten);
            }

            // Append a backtrace to the term
            src->PushBack(rapidjson::Value(bt.get()), *parent->allocator);

            rcheck_src(bt, !term_is_write_or_meta(type) || writes_legal,
                base_exc_t::LOGIC,
                strprintf("Cannot nest writes or meta ops in stream operations.  Use "
                          "FOR_EACH instead."));

            if (args != nullptr) {
                r_sanity_check(args->IsArray());
                for (size_t i = 0; i < args->Size(); ++i) {
                    backtrace_id_t child_bt 
                        = make_bt(bt, datum_t(static_cast<double>(i)));
                    walker_frame_t child_frame(parent, i == 0, this);
                    call_with_enough_stack([&]() {
                            child_frame.walk(&(*args)[i], child_bt);
                        }, MIN_WALK_STACK_SPACE);
                }
            }

            if (optargs != nullptr) {
                r_sanity_check(optargs->IsObject());
                for (auto it = optargs->MemberBegin();
                     it != optargs->MemberEnd(); ++it) {
                    backtrace_id_t child_bt = make_bt(bt,
                        datum_t(datum_string_t(it->name.GetStringLength(),
                                               it->name.GetString())));
                    walker_frame_t child_frame(parent, false, this);
                    call_with_enough_stack([&]() {
                            child_frame.walk(&it->value, child_bt);
                        }, MIN_WALK_STACK_SPACE);
                }
            }
        }

        backtrace_id_t make_bt(backtrace_id_t prev, const datum_t &d) {
            if (parent->bt_reg == nullptr) {
                return backtrace_id_t::empty();
            }
            return parent->bt_reg->new_frame(prev, d);
        }

        datum_t get_time_now() {
            if (!parent->time_now.has()) {
                parent->time_now = pseudo::time_now();
            }
            return parent->time_now;
        }

        // True if writes are still legal at this node.  Basically:
        // * Once writes become illegal, they are never legal again.
        // * Writes are legal at the root.
        // * If the parent term forbids writes in its function arguments AND we
        //   aren't inside the 0th argument, writes are forbidden.
        // * Writes are legal in all other cases.
        term_walker_t *parent;
        bool writes_legal;
        const walker_frame_t *prev_frame;
        Term::TermType type;
    };

    backtrace_registry_t *bt_reg;
    intrusive_list_t<walker_frame_t> frames;
    rapidjson::Value::AllocatorType *allocator;
    datum_t time_now;
};

void preprocess_term_tree(rapidjson::Value *term_tree,
                          rapidjson::Value::AllocatorType *allocator,
                          backtrace_registry_t *bt_reg) {
    r_sanity_check(term_tree != nullptr);
    r_sanity_check(allocator != nullptr);
    term_walker_t term_walker(allocator, bt_reg);
    term_walker.walk(term_tree);
}

void preprocess_global_optarg(rapidjson::Value *term_tree,
                              rapidjson::Value::AllocatorType *allocator) {
    r_sanity_check(term_tree != nullptr);
    r_sanity_check(allocator != nullptr);
    term_walker_t term_walker(allocator, nullptr);
    term_walker.walk(term_tree);
}

bool term_type_is_valid(Term::TermType type) {
    switch (type) {
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
    case Term::BETWEEN_DEPRECATED:
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
    case Term::OR:
    case Term::AND:
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
    case Term::VALUES:
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
    case Term::MINVAL:
    case Term::MAXVAL:
    case Term::FLOOR:
    case Term::CEIL:
    case Term::ROUND:
        return true;
    default:
        return false;
    }
}

// Returns true if `t` is a write or a meta op.
bool term_is_write_or_meta(Term::TermType type) {
    switch (type) {
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
    case Term::BETWEEN_DEPRECATED:
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
    case Term::OR:
    case Term::AND:
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
    case Term::VALUES:
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
    case Term::MINVAL:
    case Term::MAXVAL:
    case Term::FLOOR:
    case Term::CEIL:
    case Term::ROUND:
        return false;
    default: unreachable();
    }
}

bool term_forbids_writes(Term::TermType type) {
    switch (type) {
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
    case Term::BETWEEN_DEPRECATED:
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
    case Term::OR:
    case Term::AND:
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
    case Term::VALUES:
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
    case Term::MINVAL:
    case Term::MAXVAL:
    case Term::FLOOR:
    case Term::CEIL:
    case Term::ROUND:
        return false;
    default: unreachable();
    }
}

}  // namespace ql
