// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/query_cache.hpp"

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/pseudo_time.hpp"
#include "rdb_protocol/response.hpp"
#include "rdb_protocol/term_walker.hpp"

namespace ql {

query_cache_t::query_cache_t(
            rdb_context_t *_rdb_ctx,
            ip_and_port_t _client_addr_port,
            return_empty_normal_batches_t _return_empty_normal_batches) :
        rdb_ctx(_rdb_ctx),
        client_addr_port(_client_addr_port),
        return_empty_normal_batches(_return_empty_normal_batches),
        next_query_id(0),
        oldest_outstanding_query_id(0) {
    auto res = rdb_ctx->get_query_caches_for_this_thread()->insert(this);
    guarantee(res.second);
}

query_cache_t::~query_cache_t() {
    size_t res = rdb_ctx->get_query_caches_for_this_thread()->erase(this);
    guarantee(res == 1);
}

query_cache_t::const_iterator query_cache_t::begin() const {
    return queries.begin();
}

query_cache_t::const_iterator query_cache_t::end() const {
    return queries.end();
}

scoped_ptr_t<query_cache_t::ref_t> query_cache_t::create(query_params_t *query_params,
                                                         signal_t *interruptor) {
    guarantee(this == query_params->query_cache);
    query_params->maybe_release_query_id();
    if (queries.find(query_params->token) != queries.end()) {
        throw bt_exc_t(Response::CLIENT_ERROR, Response::QUERY_LOGIC,
            strprintf("ERROR: duplicate token %" PRIi64, query_params->token),
            backtrace_registry_t::EMPTY_BACKTRACE);
    }

    global_optargs_t global_optargs;
    counted_t<const term_t> term_tree;
    try {
        query_params->term_storage->preprocess();
        global_optargs = query_params->term_storage->global_optargs();

        compile_env_t compile_env((var_visibility_t()));
        term_tree = compile_term(&compile_env, query_params->term_storage->root_term());

    } catch (const exc_t &e) {
        throw bt_exc_t(Response::COMPILE_ERROR,
            e.get_error_type(),
            e.what(),
            query_params->term_storage->backtrace_registry().datum_backtrace(e));
    } catch (const datum_exc_t &e) {
        throw bt_exc_t(Response::COMPILE_ERROR,
                       e.get_error_type(),
                       e.what(),
                       backtrace_registry_t::EMPTY_BACKTRACE);
    }
    scoped_ptr_t<entry_t> entry(new entry_t(query_params,
                                            std::move(global_optargs),
                                            std::move(term_tree)));

    scoped_ptr_t<ref_t> ref(new ref_t(this,
                                      query_params->token,
                                      std::move(query_params->throttler),
                                      entry.get(),
                                      interruptor));
    auto insert_res = queries.insert(std::make_pair(query_params->token,
                                                    std::move(entry)));
    guarantee(insert_res.second);
    return ref;
}

scoped_ptr_t<query_cache_t::ref_t> query_cache_t::get(query_params_t *query_params,
                                                      signal_t *interruptor) {
    guarantee(this == query_params->query_cache);
    query_params->maybe_release_query_id();
    auto it = queries.find(query_params->token);
    if (it == queries.end()) {
        throw bt_exc_t(Response::CLIENT_ERROR, Response::QUERY_LOGIC,
            strprintf("Token %" PRIi64 " not in stream cache.", query_params->token),
            backtrace_registry_t::EMPTY_BACKTRACE);
    }

    return scoped_ptr_t<ref_t>(new ref_t(this,
                                         query_params->token,
                                         std::move(query_params->throttler),
                                         it->second.get(),
                                         interruptor));
}

void query_cache_t::noreply_wait(const query_params_t &query_params,
                                 signal_t *interruptor) {
    guarantee(this == query_params.query_cache);
    auto it = queries.find(query_params.token);
    if (it != queries.end()) {
        throw bt_exc_t(Response::CLIENT_ERROR, Response::QUERY_LOGIC,
            strprintf("ERROR: duplicate token %" PRIi64, query_params.token),
            backtrace_registry_t::EMPTY_BACKTRACE);
    }

    oldest_outstanding_query_id.get_watchable()->run_until_satisfied(
        [&query_params](uint64_t oldest_id_value) -> bool {
            return oldest_id_value == query_params.id.value();
        }, interruptor);
}

void query_cache_t::stop_query(query_params_t *query_params, signal_t *interruptor) {
    r_sanity_check(query_params->type == Query::STOP);
    guarantee(this == query_params->query_cache);
    assert_thread();
    auto entry_it = queries.find(query_params->token);
    if (entry_it != queries.end()) {
        terminate_internal(entry_it->second.get());
        entry_it->second->interrupt_reason = interrupt_reason_t::STOP;
    }

    // Acquire a temporary reference to the query, so we don't respond before the existing
    // coroutines waiting on the lock finish.
    get(query_params, interruptor);
}

void query_cache_t::terminate_internal(query_cache_t::entry_t *entry) {
    if (entry->state == entry_t::state_t::START ||
        entry->state == entry_t::state_t::STREAM) {
        entry->state = entry_t::state_t::DONE;
    }
    entry->persistent_interruptor.pulse_if_not_already_pulsed();
}

query_cache_t::ref_t::ref_t(query_cache_t *_query_cache,
                            int64_t _token,
                            new_semaphore_acq_t _throttler,
                            query_cache_t::entry_t *_entry,
                            signal_t *interruptor) :
        entry(_entry),
        token(_token),
        trace(maybe_make_profile_trace(entry->profile)),
        query_cache(_query_cache),
        throttler(std::move(_throttler)),
        drainer_lock(&entry->drainer),
        combined_interruptor(interruptor, &entry->persistent_interruptor),
        mutex_lock(&entry->mutex) {
    wait_interruptible(mutex_lock.acq_signal(), interruptor);
}

void query_cache_t::async_destroy_entry(query_cache_t::entry_t *entry) {
    delete entry;
}

query_cache_t::ref_t::~ref_t() {
    query_cache->assert_thread();
    guarantee(entry->state != entry_t::state_t::START);

    if (entry->state == entry_t::state_t::DONE) {
        // We do not delete the entry in this context for reasons:
        //  1. If there is an active exception, we aren't allowed to switch coroutines
        //  2. This will block until all auto-drainer locks on the entry have been
        //     removed, including the one in this reference
        // We remove the entry from the cache so no new queries can acquire it
        entry->state = entry_t::state_t::DELETING;

        auto it = query_cache->queries.find(token);
        guarantee(it != query_cache->queries.end());
        coro_t::spawn_sometime(std::bind(&query_cache_t::async_destroy_entry,
                                         it->second.release()));
        query_cache->queries.erase(it);
    }
}

void query_cache_t::ref_t::fill_response(response_t *res) {
    query_cache->assert_thread();
    if (entry->state != entry_t::state_t::START &&
        entry->state != entry_t::state_t::STREAM) {
        // This should only happen if the client recycled a token before
        // getting the response for the last use of the token.
        // In this case, just pretend it's a duplicate token issue
        throw bt_exc_t(
            Response::CLIENT_ERROR,
            Response::QUERY_LOGIC,
            strprintf("ERROR: duplicate token %" PRIi64, token),
            backtrace_registry_t::EMPTY_BACKTRACE);
    }

    try {
        env_t env(query_cache->rdb_ctx,
                  query_cache->return_empty_normal_batches,
                  &combined_interruptor,
                  entry->global_optargs,
                  trace.get_or_null());

        if (entry->state == entry_t::state_t::START) {
            run(&env, res);
            entry->term_tree.reset();
        }

        if (entry->state == entry_t::state_t::STREAM) {
            serve(&env, res);
        }

        if (trace.has()) {
            res->set_profile(trace->as_datum());
        }
    } catch (const interrupted_exc_t &ex) {
        query_cache->terminate_internal(entry);
        if (entry->persistent_interruptor.is_pulsed()) {
            std::string message;
            switch (entry->interrupt_reason) {
            case interrupt_reason_t::DELETE:
                message.assign("Query terminated by the `rethinkdb.jobs` table.");
                break;
            case interrupt_reason_t::STOP:
                // A STOP'ed stream should always return an empty SUCCESS
                // for compatibility purposes
                res->clear();
                res->set_type(Response::SUCCESS_SEQUENCE);
                return;
            case interrupt_reason_t::UNKNOWN:
                message.assign("Query terminated by an unknown cause.");
                break;
            default: unreachable();
            }

            throw bt_exc_t(
                Response::RUNTIME_ERROR,
                Response::OP_INDETERMINATE,
                message,
                backtrace_registry_t::EMPTY_BACKTRACE);
        }
        throw;
    } catch (const exc_t &ex) {
        query_cache->terminate_internal(entry);
        throw bt_exc_t(Response::RUNTIME_ERROR,
                       ex.get_error_type(),
                       ex.what(),
                       entry->term_storage->backtrace_registry().datum_backtrace(ex));
    } catch (const datum_exc_t &ex) {
        query_cache->terminate_internal(entry);
        throw bt_exc_t(Response::RUNTIME_ERROR,
                       ex.get_error_type(),
                       ex.what(),
                       entry->term_storage->backtrace_registry().datum_backtrace(
                            backtrace_id_t::empty(), 0));
    } catch (const std::exception &ex) {
        query_cache->terminate_internal(entry);
        throw bt_exc_t(Response::RUNTIME_ERROR,
                       Response::INTERNAL,
                       strprintf("Unexpected exception: %s", ex.what()).c_str(),
                       backtrace_registry_t::EMPTY_BACKTRACE);
    }
}

void query_cache_t::ref_t::run(env_t *env, response_t *res) {
    scope_env_t scope_env(env, var_scope_t());
    scoped_ptr_t<val_t> val = entry->term_tree->eval(&scope_env);

    if (val->get_type().is_convertible(val_t::type_t::DATUM)) {
        res->set_type(Response::SUCCESS_ATOM);
        res->set_data(val->as_datum());
        entry->state = entry_t::state_t::DONE;
    } else if (counted_t<grouped_data_t> gd =
            val->maybe_as_promiscuous_grouped_data(scope_env.env)) {
        datum_t d = to_datum_for_client_serialization(std::move(*gd), env->limits());
        res->set_type(Response::SUCCESS_ATOM);
        res->set_data(d);
        entry->state = entry_t::state_t::DONE;
    } else if (val->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
        counted_t<datum_stream_t> seq = val->as_seq(env);
        const datum_t arr = seq->as_array(env);
        if (arr.has()) {
            res->set_type(Response::SUCCESS_ATOM);
            res->set_data(arr);
            entry->state = entry_t::state_t::DONE;
        } else {
            entry->stream = seq;
            entry->has_sent_batch = false;
            entry->state = entry_t::state_t::STREAM;
        }
    } else {
        rfail_toplevel(base_exc_t::LOGIC,
                       "Query result must be of type "
                       "DATUM, GROUPED_DATA, or STREAM (got %s).",
                       val->get_type().name());
    }
}

void query_cache_t::ref_t::serve(env_t *env, response_t *res) {
    guarantee(entry->stream.has());

    feed_type_t cfeed_type = entry->stream->cfeed_type();
    if (cfeed_type != feed_type_t::not_feed) {
        // We don't throttle changefeed queries because they can block forever.
        throttler.reset();
    }

    batch_type_t batch_type = entry->has_sent_batch
                                  ? batch_type_t::NORMAL
                                  : batch_type_t::NORMAL_FIRST;
    std::vector<datum_t> ds = entry->stream->next_batch(
            env, batchspec_t::user(batch_type, env));
    entry->has_sent_batch = true;
    res->set_data(std::move(ds));

    // Note that `SUCCESS_SEQUENCE` is possible for feeds if you call `.limit`
    // after the feed.
    if (entry->stream->is_exhausted() || entry->noreply) {
        guarantee(entry->state == entry_t::state_t::STREAM);
        entry->state = entry_t::state_t::DONE;
        res->set_type(Response::SUCCESS_SEQUENCE);
    } else {
        res->set_type(Response::SUCCESS_PARTIAL);
    }

    switch (cfeed_type) {
    case feed_type_t::not_feed:
        // If we don't have a feed, then a 0-size response means there's no more
        // data.  The reason this `if` statement is only in this branch of the
        // `case` statement is that feeds can sometimes have 0-size responses
        // for other reasons (e.g. in their first batch, or just whenever with a
        // V0_3 protocol).
        if (res->data().size() == 0) res->set_type(Response::SUCCESS_SEQUENCE);
        break;
    case feed_type_t::stream:
        res->add_note(Response::SEQUENCE_FEED);
        break;
    case feed_type_t::point:
        res->add_note(Response::ATOM_FEED);
        break;
    case feed_type_t::orderby_limit:
        res->add_note(Response::ORDER_BY_LIMIT_FEED);
        break;
    case feed_type_t::unioned:
        res->add_note(Response::UNIONED_FEED);
        break;
    default: unreachable();
    }
    entry->stream->set_notes(res);
}

query_cache_t::entry_t::entry_t(query_params_t *query_params,
                                global_optargs_t &&_global_optargs,
                                counted_t<const term_t> &&_term_tree) :
        state(state_t::START),
        interrupt_reason(interrupt_reason_t::UNKNOWN),
        job_id(generate_uuid()),
        noreply(query_params->noreply),
        profile(query_params->profile ? profile_bool_t::PROFILE :
                                        profile_bool_t::DONT_PROFILE),
        term_storage(std::move(query_params->term_storage)),
        global_optargs(std::move(_global_optargs)),
        start_time(current_microtime()),
        term_tree(std::move(_term_tree)),
        has_sent_batch(false) { }

query_cache_t::entry_t::~entry_t() { }

} // namespace ql
