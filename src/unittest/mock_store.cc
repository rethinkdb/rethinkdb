#include "unittest/mock_store.hpp"

#include "arch/timing.hpp"
#include "btree/backfill.hpp"
#include "rdb_protocol/store.hpp"

namespace unittest {

write_t mock_overwrite(std::string key, std::string value) {
    std::map<datum_string_t, ql::datum_t> m;
    m[datum_string_t("id")] = ql::datum_t(datum_string_t(key));
    m[datum_string_t("value")] = ql::datum_t(datum_string_t(value));

    point_write_t pw(store_key_t(key),
                                     ql::datum_t(std::move(m)),
                                     true);
    return write_t(pw, DURABILITY_REQUIREMENT_SOFT, profile_bool_t::DONT_PROFILE,
                   ql::configured_limits_t());
}

read_t mock_read(std::string key) {
    point_read_t pr((store_key_t(key)));
    return read_t(pr, profile_bool_t::DONT_PROFILE, read_mode_t::SINGLE);
}

std::string mock_parse_read_response(const read_response_t &rr) {
    const point_read_response_t *prr
        = boost::get<point_read_response_t>(&rr.response);
    guarantee(prr != NULL);
    guarantee(prr->data.has());
    if (prr->data.get_type() == ql::datum_t::R_NULL) {
        // Behave like the old dummy_protocol_t.
        return "";
    }
    return prr->data.get_field("value").as_str().to_std();
}

std::string mock_lookup(store_view_t *store, std::string key) {
#ifndef NDEBUG
    metainfo_checker_t checker(store->get_region(),
        [](const region_t &, const binary_blob_t &) { });
#endif
    read_token_t token;
    store->new_read_token(&token);

    read_t r = mock_read(key);
    read_response_t rr;
    cond_t dummy_cond;
    store->read(DEBUG_ONLY(checker, )
                r,
                &rr,
                &token,
                &dummy_cond);
    return mock_parse_read_response(rr);
}


mock_store_t::mock_store_t(binary_blob_t universe_metainfo)
    : store_view_t(region_t::universe()),
      metainfo_(get_region(), universe_metainfo) { }
mock_store_t::~mock_store_t() { }

void mock_store_t::new_read_token(read_token_t *token_out) {
    assert_thread();
    fifo_enforcer_read_token_t token = token_source_.enter_read();
    token_out->main_read_token.create(&token_sink_, token);
}

void mock_store_t::new_write_token(write_token_t *token_out) {
    assert_thread();
    fifo_enforcer_write_token_t token = token_source_.enter_write();
    token_out->main_write_token.create(&token_sink_, token);
}

region_map_t<binary_blob_t> mock_store_t::get_metainfo(
        order_token_t order_token,
        read_token_t *token,
        const region_t &region,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(&token->main_read_token);

    wait_interruptible(token->main_read_token.get(), interruptor);

    order_sink_.check_out(order_token);

    if (rng_.randint(2) == 0) {
        nap(rng_.randint(10), interruptor);
    }
    return metainfo_.mask(region);
}

void mock_store_t::set_metainfo(const region_map_t<binary_blob_t> &new_metainfo,
                                order_token_t order_token,
                                write_token_t *token,
                                UNUSED write_durability_t durability,
                                signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    rassert(region_is_superset(get_region(), new_metainfo.get_domain()));

    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(&token->main_write_token);

    wait_interruptible(token->main_write_token.get(), interruptor);

    order_sink_.check_out(order_token);

    if (rng_.randint(2) == 0) {
        nap(rng_.randint(10), interruptor);
    }

    metainfo_.update(new_metainfo);
}



void mock_store_t::read(
        DEBUG_ONLY(const metainfo_checker_t &metainfo_checker, )
        const read_t &read,
        read_response_t *response,
        read_token_t *token,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    rassert(region_is_superset(get_region(), metainfo_checker.region));
    rassert(region_is_superset(get_region(), read.get_region()));

    {
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t
            destroyer(&token->main_read_token);

        if (token->main_read_token.has()) {
            wait_interruptible(token->main_read_token.get(), interruptor);
        }

#ifndef NDEBUG
        metainfo_.visit(metainfo_checker.region, metainfo_checker.callback);
#endif

        if (rng_.randint(2) == 0) {
            nap(rng_.randint(10), interruptor);
        }

        const point_read_t *point_read = boost::get<point_read_t>(&read.read);
        guarantee(point_read != NULL);

        response->n_shards = 1;
        response->response = point_read_response_t();
        point_read_response_t *res = boost::get<point_read_response_t>(&response->response);

        auto it = table_.find(point_read->key);
        if (it == table_.end()) {
            res->data = ql::datum_t::null();
        } else {
            res->data = it->second.second;
        }
    }
    if (rng_.randint(2) == 0) {
        nap(rng_.randint(10), interruptor);
    }
}

void mock_store_t::write(
        DEBUG_ONLY(const metainfo_checker_t &metainfo_checker, )
        const region_map_t<binary_blob_t> &new_metainfo,
        const write_t &write,
        write_response_t *response,
        UNUSED write_durability_t durability,
        state_timestamp_t timestamp,
        order_token_t order_token,
        write_token_t *token,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    rassert(region_is_superset(get_region(), metainfo_checker.region));
    rassert(region_is_superset(get_region(), new_metainfo.get_domain()));
    rassert(region_is_superset(get_region(), write.get_region()));

    {
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(&token->main_write_token);

        wait_interruptible(token->main_write_token.get(), interruptor);

        order_sink_.check_out(order_token);

#ifndef NDEBUG
        metainfo_.visit(metainfo_checker.region, metainfo_checker.callback);
#endif

        if (rng_.randint(2) == 0) {
            nap(rng_.randint(10), interruptor);
        }

        // Note that if we want to support point deletes, we'll need to store
        // deletion entries so that we can backfill them properly.  This code
        // originally was a port of the dummy protocol, so we didn't need to support
        // deletes at first.
        if (const point_write_t *point_write = boost::get<point_write_t>(&write.write)) {
            response->n_shards = 1;
            response->response = point_write_response_t();
            point_write_response_t *res =
                boost::get<point_write_response_t>(&response->response);

            guarantee(point_write->data.has());
            const bool had_value = table_.find(point_write->key) != table_.end();
            if (point_write->overwrite || !had_value) {
                table_[point_write->key]
                    = std::make_pair(timestamp.to_repli_timestamp(),
                                     point_write->data);
            }
            res->result = had_value
                ? point_write_result_t::DUPLICATE
                : point_write_result_t::STORED;
        } else if (boost::get<sync_t>(&write.write) != nullptr) {
            response->n_shards = 1;
            response->response = sync_response_t();
        } else {
            crash("mock_store_t only supports point writes and syncs");
        }

        metainfo_.update(new_metainfo);
    }
    if (rng_.randint(2) == 0) {
        nap(rng_.randint(10), interruptor);
    }
}

continue_bool_t mock_store_t::send_backfill_pre(
        const region_map_t<state_timestamp_t> &start_point,
        backfill_pre_item_consumer_t *pre_item_consumer,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    /* We iterate by remembering the last key we visited, rather than by using a
    `std::map` iterator, because this lets us block between iterations. */
    store_key_t cursor = start_point.get_domain().inner.left;
    key_range_t::right_bound_t right_bound = start_point.get_domain().inner.right;
    while (true) {
        /* Randomly block in order to more effectively test the code */
        if (rng_.randint(2) == 0) {
            nap(rng_.randint(100), interruptor);
        }
        auto it = table_.lower_bound(cursor);
        if (it == table_.end() || key_range_t::right_bound_t(it->first) >= right_bound) {
            /* There are no more keys in the range */
            if (continue_bool_t::ABORT ==
                    pre_item_consumer->on_empty_range(right_bound)) {
                return continue_bool_t::ABORT;
            }
            break;
        }
        if (it->second.first > start_point.lookup(it->first).to_repli_timestamp()) {
            /* We need to rewind this key */
            backfill_pre_item_t item;
            if (rng_.randint(10) != 0) {
                item.range = key_range_t(
                    key_range_t::closed, it->first, key_range_t::closed, it->first);
            } else {
                /* Occasionally we send a large range, just to test the code. However,
                the large range only ever contains one key (otherwise it's non-trivial to
                find large ranges) */
                item.range = key_range_t(
                    key_range_t::closed, cursor, key_range_t::closed, it->first);
            }
            if (continue_bool_t::ABORT ==
                    pre_item_consumer->on_pre_item(std::move(item))) {
                return continue_bool_t::ABORT;
            }
        }
        cursor = it->first;
        /* We `cursor.increment()` so we don't get the exact same key again. If it fails,
        that means that `cursor` was the maximum possible key, so there can't possibly be
        any more keys, so we can stop. */
        if (!cursor.increment()) {
            break;
        }
    }
    if (rng_.randint(2) == 0) {
        nap(rng_.randint(10), interruptor);
    }
    return continue_bool_t::CONTINUE;
}

std::vector<char> datum_to_vector(const ql::datum_t &datum) {
    write_message_t wm;
    serialize<cluster_version_t::CLUSTER>(&wm, datum);
    vector_stream_t vs;
    int res = send_write_message(&vs, &wm);
    guarantee(res == 0);
    std::vector<char> vector;
    vs.swap(&vector);
    return vector;
}

ql::datum_t vector_to_datum(std::vector<char> &&vector) {
    vector_read_stream_t vs(std::move(vector));
    ql::datum_t datum;
    archive_result_t res = deserialize<cluster_version_t::CLUSTER>(&vs, &datum);
    guarantee(res == archive_result_t::SUCCESS);
    return datum;
}

continue_bool_t mock_store_t::send_backfill(
        const region_map_t<state_timestamp_t> &start_point,
        backfill_pre_item_producer_t *pre_item_producer,
        backfill_item_consumer_t *item_consumer,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    key_range_t::right_bound_t table_cursor(start_point.get_domain().inner.left);
    key_range_t::right_bound_t right_bound = start_point.get_domain().inner.right;

    std::list<backfill_pre_item_t> pre_items;
    key_range_t::right_bound_t pre_items_limit = table_cursor;
    auto more_pre_items = [&]() -> continue_bool_t {
        return pre_item_producer->consume_range(
            &pre_items_limit, right_bound,
            [&](const backfill_pre_item_t &pi) {
                pre_items.push_back(pi);
            });
    };
    if (continue_bool_t::ABORT == more_pre_items()) {
        return continue_bool_t::ABORT;
    }

    while (table_cursor < right_bound) {
        /* Randomly block in order to more effectively test the code */
        if (rng_.randint(2) == 0) {
            nap(rng_.randint(100), interruptor);
        }

        /* On each iteration through the loop, we emit at most one item. There are two
        places this item can come from:
        1. Because of a key in the local copy with higher recency than `start_point`
        2. Because of a pre item we received
        So here's how we handle this: We find the next key in our database, then we check
        if the stored pre item comes before that key. If the stored pre item comes before
        that key, we handle that pre item. Otherwise, we handle that next key. */

        auto it = table_.lower_bound(table_cursor.key());
        if (it != table_.end() && key_range_t::right_bound_t(it->first) >= right_bound) {
            it = table_.end();
        }

        key_range_t::right_bound_t pre_item_edge = pre_items.empty()
            ? pre_items_limit : key_range_t::right_bound_t(pre_items.front().range.left);

        if (it == table_.end() ||
                key_range_t::right_bound_t(it->first) >= pre_item_edge) {
            if (!pre_items.empty()) {
                /* The next thing in lexicographical order is a pre item, so handle it */
                backfill_item_t item;
                item.range = pre_items.front().range;
                /* Iterate over all keys in our local copy within `pre_item->range` and
                copy them into `item`, whether or not they've changed since
                `start_point`. */
                auto jt = table_.lower_bound(item.range.left);
                auto end = item.range.right.unbounded
                    ? table_.end() : table_.upper_bound(item.range.right.key());
                for (; jt != end; ++jt) {
                    backfill_item_t::pair_t pair;
                    pair.key = jt->first;
                    pair.recency = jt->second.first;
                    pair.value =
                        boost::make_optional(datum_to_vector(jt->second.second));
                    item.pairs.push_back(std::move(pair));
                }
                if (continue_bool_t::ABORT ==
                        item_consumer->on_item(metainfo_, std::move(item))) {
                    return continue_bool_t::ABORT;
                }
                table_cursor = pre_items.front().range.right;
                pre_items.pop_front();
            } else {
                /* The next thing is a gap with no pre items in it. We call
                `on_empty_range()` so that if `more_pre_items()` returns `ABORT`, at
                least the callback will know that there were no items in this range. */
                if (continue_bool_t::ABORT ==
                        item_consumer->on_empty_range(metainfo_, pre_items_limit)) {
                    return continue_bool_t::ABORT;
                }
                table_cursor = pre_items_limit;
                if (continue_bool_t::ABORT == more_pre_items()) {
                    return continue_bool_t::ABORT;
                }
            }
        } else {
            /* The next thing in lexicographical order is a key in our local copy, not a
            pre item. */
            if (it->second.first >
                    start_point.lookup(it->first).to_repli_timestamp()) {
                /* The key has changed since `start_point`, so we'll transmit it. */
                backfill_item_t item;
                item.range = key_range_t(
                    key_range_t::closed, it->first, key_range_t::closed, it->first);
                backfill_item_t::pair_t pair;
                pair.key = it->first;
                pair.recency = it->second.first;
                pair.value = boost::make_optional(datum_to_vector(it->second.second));
                item.pairs.push_back(std::move(pair));
                if (continue_bool_t::ABORT ==
                        item_consumer->on_item(metainfo_, std::move(item))) {
                    return continue_bool_t::ABORT;
                }
            }
            table_cursor = key_range_t::right_bound_t(it->first);
            bool ok = table_cursor.increment();
            guarantee(ok);
        }
    }
    if (rng_.randint(2) == 0) {
        nap(rng_.randint(10), interruptor);
    }
    return continue_bool_t::CONTINUE;
}

continue_bool_t mock_store_t::receive_backfill(
        const region_t &region,
        backfill_item_producer_t *item_producer,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    key_range_t::right_bound_t cursor(region.inner.left);
    while (cursor < region.inner.right) {
        /* Introduce a random delay to test more code paths */
        if (rng_.randint(2) == 0) {
            nap(rng_.randint(100), interruptor);
        }

        /* Fetch the next item from the producer */
        bool is_item;
        backfill_item_t item;
        key_range_t::right_bound_t empty_range;
        if (continue_bool_t::ABORT ==
                item_producer->next_item(&is_item, &item, &empty_range)) {
            return continue_bool_t::ABORT;
        }

        region_t metainfo_mask = region;
        metainfo_mask.inner.left = cursor.key();

        if (is_item) {
            guarantee(key_range_t::right_bound_t(item.range.left) >= cursor);
            cursor = item.range.right;

            /* Delete any existing key-value pairs in the range */
            auto end = item.range.right.unbounded
                    ? table_.end() : table_.lower_bound(item.range.right.key());
            for (auto it = table_.lower_bound(item.range.left); it != end; ) {
                table_.erase(it++);
            }

            /* Insert key-value pairs that were stored in the item */
            for (const auto &pair : item.pairs) {
                guarantee(static_cast<bool>(pair.value));
                table_[pair.key] = std::make_pair(
                    pair.recency, vector_to_datum(std::vector<char>(*pair.value)));
            }
        } else {
           cursor = empty_range;
        }

        metainfo_mask.inner.right = cursor;
        metainfo_.update(item_producer->get_metainfo()->mask(metainfo_mask));

        item_producer->on_commit(cursor);
    }
    guarantee(cursor == region.inner.right);
    return continue_bool_t::CONTINUE;
}

void mock_store_t::wait_until_ok_to_receive_backfill(signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    if (rng_.randint(2) == 0) {
        nap(rng_.randint(100), interruptor);
    }
}

void mock_store_t::reset_data(
        const binary_blob_t &zero_version,
        const region_t &subregion,
        UNUSED write_durability_t durability,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    rassert(region_is_superset(get_region(), subregion));

    write_token_t token;
    new_write_token(&token);

    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t
        destroyer(&token.main_write_token);

    wait_interruptible(token.main_write_token.get(), interruptor);

    rassert(region_is_superset(get_region(), subregion));

    auto it = table_.lower_bound(subregion.inner.left);
    while (it != table_.end() && subregion.inner.contains_key(it->first)) {
        auto jt = it;
        ++it;
        if (region_contains_key(subregion, jt->first)) {
            table_.erase(jt);
        }
    }

    metainfo_.update(subregion, zero_version);
}

std::string mock_store_t::values(std::string key) {
    auto it = table_.find(store_key_t(key));
    if (it == table_.end()) {
        // Behave like the old dummy_protocol_t.
        return "";
    }
    return it->second.second.get_field("value").as_str().to_std();
}

repli_timestamp_t mock_store_t::timestamps(std::string key) {
    auto it = table_.find(store_key_t(key));
    if (it == table_.end()) {
        return repli_timestamp_t::distant_past;
    }
    return it->second.first;
}



}  // namespace unittest
