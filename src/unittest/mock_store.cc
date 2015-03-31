#include "unittest/mock_store.hpp"

#include "arch/timing.hpp"
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
    return read_t(pr, profile_bool_t::DONT_PROFILE);
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
    trivial_metainfo_checker_callback_t checker_cb;
    metainfo_checker_t checker(&checker_cb, store->get_region());
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

void mock_store_t::do_get_metainfo(order_token_t order_token,
                                   read_token_t *token,
                                   signal_t *interruptor,
                                   region_map_t<binary_blob_t> *out)
    THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(&token->main_read_token);

    wait_interruptible(token->main_read_token.get(), interruptor);

    order_sink_.check_out(order_token);

    if (rng_.randint(2) == 0) {
        nap(rng_.randint(10), interruptor);
    }
    region_map_t<binary_blob_t> res = metainfo_.mask(get_region());
    *out = res;
}

void mock_store_t::set_metainfo(const region_map_t<binary_blob_t> &new_metainfo,
                                order_token_t order_token,
                                write_token_t *token,
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
    rassert(region_is_superset(get_region(), metainfo_checker.get_domain()));
    rassert(region_is_superset(get_region(), read.get_region()));

    {
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t
            destroyer(&token->main_read_token);

        if (token->main_read_token.has()) {
            wait_interruptible(token->main_read_token.get(), interruptor);
        }

#ifndef NDEBUG
        metainfo_checker.check_metainfo(metainfo_.mask(metainfo_checker.get_domain()));
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
    rassert(region_is_superset(get_region(), metainfo_checker.get_domain()));
    rassert(region_is_superset(get_region(), new_metainfo.get_domain()));
    rassert(region_is_superset(get_region(), write.get_region()));

    {
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(&token->main_write_token);

        wait_interruptible(token->main_write_token.get(), interruptor);

        order_sink_.check_out(order_token);

        rassert(metainfo_checker.get_domain() == metainfo_.mask(metainfo_checker.get_domain()).get_domain());
#ifndef NDEBUG
        metainfo_checker.check_metainfo(metainfo_.mask(metainfo_checker.get_domain()));
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

void mock_store_t::send_backfill_pre(
        const region_map_t<state_timestamp_t> &start_point,
        backfill_pre_atom_consumer_t *pre_atom_consumer,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    (void)start_point;
    (void)pre_atom_consumer;
    (void)interruptor;
    crash("not implemented");
}

void mock_store_t::send_backfill(
        const region_map_t<state_timestamp_t> &start_point,
        backfill_pre_atom_producer_t *pre_atom_producer,
        backfill_atom_consumer_t *atom_consumer,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    (void)start_point;
    (void)pre_atom_producer;
    (void)atom_consumer;
    (void)interruptor;
    crash("not implemented");
}

void mock_store_t::receive_backfill(
        const region_t &region,
        backfill_atom_producer_t *atom_producer,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    (void)region;
    (void)atom_producer;
    (void)interruptor;
    crash("not implemented");
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

    metainfo_.set(subregion, zero_version);
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
