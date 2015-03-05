// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/query_cache.hpp"

#include "rdb_protocol/env.hpp"

#include "debug.hpp"

namespace ql {

query_cache_exc_t::query_cache_exc_t(Response_ResponseType _type,
                                     std::string _message,
                                     backtrace_t _bt) :
    type(_type), message(_message), bt(_bt) { }

query_cache_exc_t::~query_cache_exc_t() throw () { }

void query_cache_exc_t::fill_response(Response *res) const {
    fill_error(res, type, message.c_str(), bt);
}

const char *query_cache_exc_t::what() const throw () {
    return message.c_str();
}

query_id_t::query_id_t(query_id_t &&other) :
        intrusive_list_node_t(std::move(other)),
        parent(other.parent),
        value_(other.value_) {
    parent->assert_thread();
    other.parent = NULL;
}

query_id_t::query_id_t(query_cache_t *_parent) :
        parent(_parent),
        value_(parent->next_query_id++) {
    // Guarantee correct ordering.
    query_id_t *last_newest = parent->outstanding_query_ids.tail();
    guarantee(last_newest == nullptr || last_newest->value() < value_);
    guarantee(value_ >= parent->oldest_outstanding_query_id.get());

    parent->outstanding_query_ids.push_back(this);
}

query_id_t::~query_id_t() {
    if (parent != nullptr) {
        parent->assert_thread();
    } else {
        rassert(!in_a_list());
    }

    if (in_a_list()) {
        parent->outstanding_query_ids.remove(this);
        if (value_ == parent->oldest_outstanding_query_id.get()) {
            query_id_t *next_outstanding_id = parent->outstanding_query_ids.head();
            if (next_outstanding_id == nullptr) {
                parent->oldest_outstanding_query_id.set_value(parent->next_query_id);
            } else {
                guarantee(next_outstanding_id->value() > value_);
                parent->oldest_outstanding_query_id.set_value(next_outstanding_id->value());
            }
        }
    }
}

uint64_t query_id_t::value() const {
    guarantee(in_a_list());
    return value_;
}

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

scoped_ptr_t<query_cache_t::ref_t> query_cache_t::create(
        int64_t token,
        protob_t<Query> original_query,
        use_json_t use_json,
        signal_t *interruptor) {
    if (queries.find(token) != queries.end()) {
        throw query_cache_exc_t(Response::CLIENT_ERROR,
            strprintf("ERROR: duplicate token %" PRIi64, token), backtrace_t());
    }

    counted_t<const term_t> root_term;
    std::map<std::string, wire_func_t> global_optargs;
    try {
        // Parsing the global optargs also pre-processes the query
        global_optargs = parse_global_optargs(original_query);

        Term *t = original_query->mutable_query();
        compile_env_t compile_env((var_visibility_t()));
        root_term = compile_term(&compile_env, original_query.make_child(t));
    } catch (const exc_t &e) {
        throw query_cache_exc_t(Response::COMPILE_ERROR, e.what(), e.backtrace());
    } catch (const datum_exc_t &e) {
        throw query_cache_exc_t(Response::COMPILE_ERROR, e.what(), backtrace_t());
    }

    scoped_ptr_t<entry_t> entry(new entry_t(original_query,
                                            std::move(global_optargs),
                                            std::move(root_term)));
    scoped_ptr_t<ref_t> ref(new ref_t(this,
                                      token,
                                      entry.get(),
                                      use_json,
                                      interruptor));
    auto insert_res = queries.insert(std::make_pair(token, std::move(entry)));
    guarantee(insert_res.second);
    return ref;
}

scoped_ptr_t<query_cache_t::ref_t> query_cache_t::get(
        int64_t token,
        use_json_t use_json,
        signal_t *interruptor) {
    auto it = queries.find(token);
    if (it == queries.end()) {
        throw query_cache_exc_t(Response::CLIENT_ERROR,
            strprintf("Token %" PRIi64 " not in stream cache.", token), backtrace_t());
    }

    return scoped_ptr_t<ref_t>(new ref_t(this,
                                         token,
                                         it->second.get(),
                                         use_json,
                                         interruptor));
}

void query_cache_t::noreply_wait(const query_id_t &query_id,
                                 int64_t token,
                                 signal_t *interruptor) {
    auto it = queries.find(token);
    if (it != queries.end()) {
        throw query_cache_exc_t(Response::CLIENT_ERROR,
            strprintf("ERROR: duplicate token %" PRIi64, token), backtrace_t());
    }

    oldest_outstanding_query_id.get_watchable()->run_until_satisfied(
        [&query_id](uint64_t oldest_id_value) -> bool {
            return oldest_id_value == query_id.value();
        }, interruptor);
}

query_cache_t::ref_t::ref_t(query_cache_t *_query_cache,
                            int64_t _token,
                            query_cache_t::entry_t *_entry,
                            use_json_t _use_json,
                            signal_t *interruptor) :
        entry(_entry),
        token(_token),
        trace(maybe_make_profile_trace(entry->profile)),
        use_json(_use_json),
        query_cache(_query_cache),
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

void query_cache_t::ref_t::terminate() {
    query_cache->assert_thread();
    if (entry->state == entry_t::state_t::START ||
        entry->state == entry_t::state_t::STREAM) {
        entry->state = entry_t::state_t::DONE;
    }
    entry->persistent_interruptor.pulse_if_not_already_pulsed();
}

void query_cache_t::ref_t::fill_response(Response *res) {
    query_cache->assert_thread();
    if (entry->state != entry_t::state_t::START &&
        entry->state != entry_t::state_t::STREAM) {
        // This should only happen if the client recycled a token before
        // getting the response for the last use of the token.
        // In this case, just pretend it's a duplicate token issue
        throw query_cache_exc_t(Response::CLIENT_ERROR,
                                strprintf("ERROR: duplicate token %" PRIi64, token),
                                backtrace_t());
    }

    try {
        env_t env(query_cache->rdb_ctx,
                  query_cache->return_empty_normal_batches,
                  &combined_interruptor,
                  entry->global_optargs,
                  trace.get_or_null());

        if (entry->state == entry_t::state_t::START) {
            run(&env, res);
            entry->root_term.reset();
        }

        if (entry->state == entry_t::state_t::STREAM) {
            serve(&env, res);
        }

        if (trace.has()) {
            trace->as_datum().write_to_protobuf(res->mutable_profile(), use_json);
        }
    } catch (const interrupted_exc_t &ex) {
        if (entry->persistent_interruptor.is_pulsed()) {
            if (entry->state != entry_t::state_t::DONE) {
                throw query_cache_exc_t(Response::RUNTIME_ERROR,
                    "Query terminated by the `rethinkdb.jobs` table.", backtrace_t());
            }
            // For compatibility, we return a SUCCESS_SEQUENCE in this case
            res->Clear();
            res->set_type(Response::SUCCESS_SEQUENCE);
        } else {
            terminate();
            throw;
        }
    } catch (...) {
        terminate();
        throw;
    }
}

void query_cache_t::ref_t::run(env_t *env, Response *res) {
    // The state will be overwritten if we end up with a stream
    entry->state = entry_t::state_t::DONE;

    scope_env_t scope_env(env, var_scope_t());

    scoped_ptr_t<val_t> val = entry->root_term->eval(&scope_env);
    if (val->get_type().is_convertible(val_t::type_t::DATUM)) {
        res->set_type(Response::SUCCESS_ATOM);
        val->as_datum().write_to_protobuf(res->add_response(), use_json);
    } else if (counted_t<grouped_data_t> gd =
            val->maybe_as_promiscuous_grouped_data(scope_env.env)) {
        datum_t d = to_datum_for_client_serialization(std::move(*gd),
                                                      env->reql_version(),
                                                      env->limits());
        res->set_type(Response::SUCCESS_ATOM);
        d.write_to_protobuf(res->add_response(), use_json);
    } else if (val->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
        counted_t<datum_stream_t> seq = val->as_seq(env);
        const datum_t arr = seq->as_array(env);
        if (arr.has()) {
            res->set_type(Response::SUCCESS_ATOM);
            arr.write_to_protobuf(res->add_response(), use_json);
        } else {
            entry->stream = seq;
            entry->has_sent_batch = false;
            entry->state = entry_t::state_t::STREAM;
        }
    } else {
        rfail_toplevel(base_exc_t::GENERIC,
                       "Query result must be of type "
                       "DATUM, GROUPED_DATA, or STREAM (got %s).",
                       val->get_type().name());
    }
}

void query_cache_t::ref_t::serve(env_t *env, Response *res) {
    guarantee(entry->stream.has());

    batch_type_t batch_type = entry->has_sent_batch
                                  ? batch_type_t::NORMAL
                                  : batch_type_t::NORMAL_FIRST;
    std::vector<datum_t> ds = entry->stream->next_batch(
            env, batchspec_t::user(batch_type, env));
    entry->has_sent_batch = true;
    for (auto d = ds.begin(); d != ds.end(); ++d) {
        d->write_to_protobuf(res->add_response(), use_json);
    }

    res->set_type(Response::SUCCESS_PARTIAL);
    switch (entry->stream->cfeed_type()) {
    case feed_type_t::not_feed:
        if (entry->stream->is_exhausted() || res->response_size() ==0) {
            res->set_type(Response::SUCCESS_SEQUENCE);
        }
        break;
    case feed_type_t::stream:
        res->add_notes(Response::SEQUENCE_FEED);
        break;
    case feed_type_t::point:
        res->add_notes(Response::ATOM_FEED);
        break;
    case feed_type_t::orderby_limit:
        res->add_notes(Response::ORDER_BY_LIMIT_FEED);
        break;
    case feed_type_t::unioned:
        res->add_notes(Response::UNIONED_FEED);
        break;
    default: unreachable();
    }
}

query_cache_t::entry_t::entry_t(protob_t<Query> _original_query,
                                std::map<std::string, wire_func_t> &&_global_optargs,
                                counted_t<const term_t> _root_term) :
        state(state_t::START),
        job_id(generate_uuid()),
        original_query(_original_query),
        global_optargs(std::move(_global_optargs)),
        profile(profile_bool_optarg(original_query)),
        start_time(current_microtime()),
        root_term(_root_term),
        has_sent_batch(false) { }

query_cache_t::entry_t::~entry_t() { }

} // namespace ql
