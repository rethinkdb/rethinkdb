// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/stream.hpp"

#include "btree/keys.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/ql2.hpp"
#include "rdb_protocol/transform_visitors.hpp"
#include "rdb_protocol/datum_stream.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace query_language {

batched_rget_stream_t::batched_rget_stream_t(
    const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
    counted_t<const ql::datum_t> left_bound, bool left_bound_open,
    counted_t<const ql::datum_t> right_bound, bool right_bound_open,
    const std::map<std::string, ql::wire_func_t> &_optargs,
    bool _use_outdated, sorting_t _sorting,
    ql::rcheckable_t *_parent)
    : ns_access(_ns_access),
      finished(false), started(false), optargs(_optargs), use_outdated(_use_outdated),
      range(left_bound_open ? key_range_t::open : key_range_t::closed,
            left_bound.has()
              ? store_key_t(left_bound->print_primary())
              : store_key_t::min(),
            right_bound_open ? key_range_t::open : key_range_t::closed,
            right_bound.has()
              ? store_key_t(right_bound->print_primary())
              : store_key_t::max()),
      sorting(_sorting),
      parent(_parent)
{ }

batched_rget_stream_t::batched_rget_stream_t(
    const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
    const std::string &_sindex_id,
    counted_t<const ql::datum_t> _sindex_start_value, bool start_value_open,
    counted_t<const ql::datum_t> _sindex_end_value, bool end_value_open,
    const std::map<std::string, ql::wire_func_t> &_optargs, bool _use_outdated,
    sorting_t _sorting, ql::rcheckable_t *_parent)
    : ns_access(_ns_access),
      sindex_id(_sindex_id),
      finished(false),
      started(false),
      optargs(_optargs),
      use_outdated(_use_outdated),
      sindex_range(_sindex_start_value, start_value_open,
                   _sindex_end_value, end_value_open),
      range(rdb_protocol_t::sindex_key_range(
                _sindex_start_value.has()
                  ? _sindex_start_value->truncated_secondary()
                  : store_key_t::min(),
                _sindex_end_value.has()
                  ? _sindex_end_value->truncated_secondary()
                  : store_key_t::max())),
      sorting(_sorting),
      parent(_parent)
{ }

boost::optional<rget_item_t> batched_rget_stream_t::head(ql::env_t *env) {
    started = true;
    if (data.empty()) {
        if (finished) {
            return boost::optional<rget_item_t>();
        }
        read_more(env);
        if (data.empty()) {
            finished = true;
            return boost::optional<rget_item_t>();
        }
    }

    return data.front();
}

void batched_rget_stream_t::pop() {
    guarantee(!data.empty());
    data.pop_front();
}

bool rget_item_sindex_key_less(const rget_item_t &left, const rget_item_t &right) {
    r_sanity_check(left.sindex_key);
    r_sanity_check(right.sindex_key);
    return **left.sindex_key < **right.sindex_key;
}

bool rget_item_sindex_key_greater(const rget_item_t &left, const rget_item_t &right) {
    r_sanity_check(left.sindex_key);
    r_sanity_check(right.sindex_key);
    return **right.sindex_key < **left.sindex_key;
}

/* This function is a big monolithic mess right now. This is because a lot of
 * complexity got squeezed in to it. How we got in to this situation is that we
 * have this nasty edge case with sorting that comes from the fact that we
 * truncate secondary index keys when they're too long. Because we do this if a
 * users creates long sindex keys the btree sorting algorithm won't accurately
 * sort. Two adjacent keys which are long enough that their difference occur
 * after the truncation point will be sorted as equal but aren't actually
 * equal. To compensate for this on values with oversized sindex keys we send
 * back the full sindex value as json and do the remaining sorting on the
 * parser side in this function. */

/* On top of this there's added complexity due to the fact that we need to
 * return hints whether successive values are equal. This is for the case when
 * a user wants to sort by and index and use another field as the tiebreaker.
 * The sorting by the other field is done above but it needs to know which
 * elements had the same index value so it knows what to apply the sorting too.
 * */
hinted_datum_t batched_rget_stream_t::sorting_hint_next(ql::env_t *env) {
    /* The simple case. No sorting is happening. */
    if (sorting == UNORDERED) {
        boost::optional<rget_item_t> item = head(env);
        if (item) {
            pop();
            return hinted_datum_t(CONTINUE, item->data);
        } else {
            return hinted_datum_t(CONTINUE, counted_t<const ql::datum_t>());
        }
    }

    if (!sorting_buffer.empty()) {
        /* A non empty sorting buffer means we already got a batch of
         * ambigiously sorted values and sorted them. So now we can just pop
         * one off the front and return it. */
        counted_t<const ql::datum_t> datum = sorting_buffer.front().data;
        r_sanity_check(sorting_buffer.front().sindex_key);
        bool is_new_key = check_and_set_last_key(*sorting_buffer.front().sindex_key);
        sorting_buffer.pop_front();
        return hinted_datum_t((is_new_key ? START : CONTINUE), datum);
    } else {
        for (;;) {
            /* In this loop we load data in to the sorting buffer until we can
             * be sure that the next value to be returned is in it. (Or we hit
             * the limit at which point we error.) */
            boost::optional<rget_item_t> item = head(env);
            if (!item) {
                break;
            }

            if (!sindex_id) {
                pop();
                return hinted_datum_t(START, item->data);
            }

            std::string key = key_to_unescaped_str(item->key);
            std::string skey = ql::datum_t::extract_secondary(key);

            if (!ql::datum_t::key_is_truncated(item->key)) {
                if (sorting_buffer.empty()) {
                    /* We have an untruncated key and the sorting buffer is
                     * empty. This means we have the next value. */
                    pop();
                    bool is_new_key = check_and_set_last_key(skey);
                    return hinted_datum_t((is_new_key ? START : CONTINUE), item->data);
                } else {
                    /* We have an untruncated key but there's data in the
                     * sorting buffer. That means this value definitely isn't
                     * the next value but we know that something in the sorting
                     * buffer is. Time to sort the sorting buffer. */
                    break;
                }
            } else if (check_and_set_key_in_sorting_buffer(skey)) {
                /* We have a truncated key and the sorting buffer is either
                 * empty or contains only keys which could be greater than this
                 * one. Put the data in to the sorting buffer. */
                pop();
                sorting_buffer.push_back(*item);
                rcheck_target(parent, ql::base_exc_t::GENERIC,
                        sorting_buffer.size() < ql::sort_el_limit,
                        strprintf("In memory sort limit exceeded. This is due to having over "
                        "%zd index values with a common, long prefix.",  ql::sort_el_limit));
            } else {
                /* We have a truncated key and things in the sorting_buffer but
                 * the things in the sorting buffer don't have the same
                 * truncated key as the item. This means the next key to be
                 * returned is in the sorting buffer. We're done loading data.
                 * */
                break;
            }
        }
    }

    if (sorting_buffer.empty()) {
        /* Nothing in the sorting buffer, this means we don't have any data to
         * return so we return nothing. */
        return hinted_datum_t(CONTINUE, counted_t<const ql::datum_t>());
    } else {
        /* There's data in the sorting_buffer time to sort it. */
        if (sorting == ASCENDING) {
            std::sort(sorting_buffer.begin(), sorting_buffer.end(),
                      &rget_item_sindex_key_less);
        } else if (sorting == DESCENDING) {
            std::sort(sorting_buffer.begin(), sorting_buffer.end(),
                      &rget_item_sindex_key_greater);
        } else {
            r_sanity_check(false);
        }

        /* Now we can finally return a value from the sorting buffer. */
        counted_t<const ql::datum_t> datum = sorting_buffer.front().data;
        bool is_new_key = check_and_set_last_key(*sorting_buffer.front().sindex_key);
        sorting_buffer.pop_front();
        return hinted_datum_t((is_new_key ? START : CONTINUE), datum);
    }
}

counted_t<const ql::datum_t> batched_rget_stream_t::next(ql::env_t *env) {
    return sorting_hint_next(env).second;
}

boost::shared_ptr<json_stream_t> batched_rget_stream_t::add_transformation(
    const rdb_protocol_details::transform_variant_t &t) {
    guarantee(!started);
    transform.push_back(t);
    return shared_from_this();
}

rdb_protocol_t::rget_read_response_t::result_t batched_rget_stream_t::apply_terminal(
    const rdb_protocol_details::terminal_variant_t &t, ql::env_t *env) {
    rdb_protocol_t::rget_read_t rget_read = get_rget();
    rget_read.terminal = t;
    rdb_protocol_t::read_t read(rget_read);
    try {
        rdb_protocol_t::read_response_t res;
        if (use_outdated) {
            ns_access.get_namespace_if()->read_outdated(read, &res, env->interruptor);
        } else {
            ns_access.get_namespace_if()->read(
                read, &res, order_token_t::ignore, env->interruptor);
        }
        rdb_protocol_t::rget_read_response_t *p_res
            = boost::get<rdb_protocol_t::rget_read_response_t>(&res.response);
        guarantee(p_res);

        /* Re throw an exception if we got one. */
        if (ql::exc_t *e2 = boost::get<ql::exc_t>(&p_res->result)) {
            throw *e2;
        } else if (ql::datum_exc_t *e3 = boost::get<ql::datum_exc_t>(&p_res->result)) {
            throw *e3;
        }

        return p_res->result;
    } catch (const cannot_perform_query_exc_t &e) {
        rfail_datum(ql::base_exc_t::GENERIC, "cannot perform read: %s", e.what());
    }
}

rdb_protocol_t::rget_read_t batched_rget_stream_t::get_rget() {
    if (!sindex_id) {
        return rdb_protocol_t::rget_read_t(rdb_protocol_t::region_t(range),
                                           transform,
                                           optargs,
                                           sorting);
    } else {
        return rdb_protocol_t::rget_read_t(rdb_protocol_t::region_t(range),
                                           *sindex_id,
                                           sindex_range,
                                           transform,
                                           optargs,
                                           sorting);
    }
}

void batched_rget_stream_t::read_more(ql::env_t *env) {
    rdb_protocol_t::read_t read(get_rget());
    try {
        guarantee(ns_access.get_namespace_if());
        rdb_protocol_t::read_response_t res;
        if (use_outdated) {
            ns_access.get_namespace_if()->read_outdated(read, &res, env->interruptor);
        } else {
            ns_access.get_namespace_if()->read(
                read, &res, order_token_t::ignore, env->interruptor);
        }
        rdb_protocol_t::rget_read_response_t *p_res
            = boost::get<rdb_protocol_t::rget_read_response_t>(&res.response);
        guarantee(p_res);

        /* Re throw an exception if we got one. */
        if (auto e = boost::get<ql::exc_t>(&p_res->result)) {
            throw *e;
        } else if (auto e2 = boost::get<ql::datum_exc_t>(&p_res->result)) {
            throw *e2;
        }

        // todo: just do a straight copy?
        typedef rdb_protocol_t::rget_read_response_t::stream_t stream_t;
        stream_t *stream = boost::get<stream_t>(&p_res->result);
        guarantee(stream);

        for (stream_t::iterator i = stream->begin(); i != stream->end(); ++i) {
            guarantee(i->data);
            data.push_back(*i);
        }

        if (forward(sorting)) {
            range.left = p_res->last_considered_key;
        } else {
            range.right = key_range_t::right_bound_t(p_res->last_considered_key);
        }

        if (forward(sorting) &&
            (!range.left.increment() ||
            (!range.right.unbounded && (range.right.key < range.left)))) {
            finished = true;
        } else if (backward(sorting)) {
            guarantee(!range.right.unbounded);
            if (!range.right.key.decrement() ||
                range.right.key < range.left) {
                finished = true;
            }
        }
    } catch (const cannot_perform_query_exc_t &e) {
        rfail_datum(ql::base_exc_t::GENERIC, "cannot perform read: %s", e.what());
    }
}

bool batched_rget_stream_t::check_and_set_key_in_sorting_buffer(const std::string &key) {
    if (sorting_buffer.empty()) {
        key_in_sorting_buffer = key;
        return true;
    } else if (strncmp(key_in_sorting_buffer.data(), key.data(),
               std::min(key_in_sorting_buffer.size(), key.size()))
                == 0) {
        if (key.size() > key_in_sorting_buffer.size()) {
            key_in_sorting_buffer = key;
        }
        return true;
    } else {
        return false;
    }
}

bool batched_rget_stream_t::check_and_set_last_key(const std::string &key) {
    std::string *last_key_str;
    if (!(last_key_str = boost::get<std::string>(&last_key))) {
        last_key = key;
        return true;
    } else {
        if (key == *last_key_str) {
            return false;
        } else {
            last_key = key;
            return true;
        }
    }
}
bool batched_rget_stream_t::check_and_set_last_key(counted_t<const ql::datum_t> key) {
    counted_t<const ql::datum_t> *last_key_json;
    /* Notice we check both for a last_key value which is a std::string and a
     * last_key value which is an empty shared_ptr the latter is only present
     * immediately after construction. */
    if (!(last_key_json = boost::get<counted_t<const ql::datum_t> >(&last_key)) ||
        !(*last_key_json)) {
        last_key = key;
        return true;
    } else {
        if (*key == **last_key_json) {
            return false;
        } else {
            last_key = key;
            return true;
        }
    }
}

} // namespace query_language

