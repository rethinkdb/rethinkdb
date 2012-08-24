#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>

#include "btree/erase_range.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "btree/superblock.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "protob/protob.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/query_language.hpp"
#include "serializer/config.hpp"

typedef rdb_protocol_details::backfill_atom_t rdb_backfill_atom_t;

typedef rdb_protocol_t::store_t store_t;
typedef rdb_protocol_t::region_t region_t;

typedef rdb_protocol_t::read_t read_t;
typedef rdb_protocol_t::read_response_t read_response_t;

typedef rdb_protocol_t::point_read_t point_read_t;
typedef rdb_protocol_t::point_read_response_t point_read_response_t;

typedef rdb_protocol_t::rget_read_t rget_read_t;
typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

typedef rdb_protocol_t::write_t write_t;
typedef rdb_protocol_t::write_response_t write_response_t;

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;

typedef rdb_protocol_t::point_delete_t point_delete_t;
typedef rdb_protocol_t::point_delete_response_t point_delete_response_t;

typedef rdb_protocol_t::backfill_chunk_t backfill_chunk_t;

typedef rdb_protocol_t::backfill_progress_t backfill_progress_t;

typedef rdb_protocol_t::rget_read_response_t::stream_t stream_t;
typedef rdb_protocol_t::rget_read_response_t::groups_t groups_t;
typedef rdb_protocol_t::rget_read_response_t::atom_t atom_t;
typedef rdb_protocol_t::rget_read_response_t::length_t length_t;
typedef rdb_protocol_t::rget_read_response_t::inserted_t inserted_t;

const std::string rdb_protocol_t::protocol_name("rdb");

RDB_IMPL_PROTOB_SERIALIZABLE(Builtin_Range);
RDB_IMPL_PROTOB_SERIALIZABLE(Builtin_Filter);
RDB_IMPL_PROTOB_SERIALIZABLE(Builtin_Map);
RDB_IMPL_PROTOB_SERIALIZABLE(Builtin_ConcatMap);
RDB_IMPL_PROTOB_SERIALIZABLE(Builtin_GroupedMapReduce);
RDB_IMPL_PROTOB_SERIALIZABLE(Reduction);
RDB_IMPL_PROTOB_SERIALIZABLE(WriteQuery_ForEach);

// Construct a region containing only the specified key
region_t rdb_protocol_t::monokey_region(const store_key_t &k) {
    uint64_t h = hash_region_hasher(k.contents(), k.size());
    return region_t(h, h + 1, key_range_t(key_range_t::closed, k, key_range_t::closed, k));
}

/* read_t::get_region implementation */
struct r_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const point_read_t &pr) const {
        return rdb_protocol_t::monokey_region(pr.key);
    }

    region_t operator()(const rget_read_t &rg) const {
        // TODO: Sam bets this causes problems
        return region_t(rg.key_range);
    }
};

region_t read_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(r_get_region_visitor(), read);
}

/* read_t::shard implementation */

struct r_shard_visitor : public boost::static_visitor<read_t> {
    explicit r_shard_visitor(const region_t &_region)
        : region(_region)
    { }

    read_t operator()(const point_read_t &pr) const {
        rassert(rdb_protocol_t::monokey_region(pr.key) == region);
        return read_t(pr);
    }

    read_t operator()(const rget_read_t &rg) const {
        rassert(region_is_superset(region_t(rg.key_range), region));
        // TODO: Reevaluate this code.  Should rget_query_t really have a region_t range?
        rget_read_t _rg(rg);
        _rg.key_range = region.inner;
        return read_t(_rg);
    }
    const region_t &region;
};

read_t read_t::shard(const region_t &region) const THROWS_NOTHING {
    return boost::apply_visitor(r_shard_visitor(region), read);
}

/* read_t::unshard implementation */
bool read_response_cmp(const read_response_t &l, const read_response_t &r) {
    const rget_read_response_t *lr = boost::get<rget_read_response_t>(&l.response);
    rassert(lr);
    const rget_read_response_t *rr = boost::get<rget_read_response_t>(&r.response);
    rassert(rr);
    return lr->key_range < rr->key_range;
}

void read_t::unshard(std::vector<read_response_t> responses, read_response_t *response, context_t *ctx) const THROWS_NOTHING {
    boost::shared_ptr<js::runner_t> js_runner = boost::make_shared<js::runner_t>();
    query_language::runtime_environment_t env(ctx->pool_group, ctx->ns_repo, ctx->semilattice_metadata, js_runner, &ctx->interruptor, ctx->machine_id);

    const point_read_t *pr = boost::get<point_read_t>(&read);
    const rget_read_t *rg = boost::get<rget_read_t>(&read);
    if (pr) {
        rassert(responses.size() == 1);
        rassert(boost::get<point_read_response_t>(&responses[0].response));
        *response = responses[0];
    } else if (rg) {
        response->response = rget_read_response_t();
        rget_read_response_t &rg_response = boost::get<rget_read_response_t>(response->response);
        rg_response.truncated = false;
        rg_response.key_range = get_region().inner;
        rg_response.last_considered_key = get_region().inner.left;
        typedef std::vector<read_response_t>::iterator rri_t;

        if (!rg->terminal) {
            //A vanilla range get
            rg_response.result = stream_t();
            stream_t *res_stream = boost::get<stream_t>(&rg_response.result);
            for(rri_t i = responses.begin(); i != responses.end(); ++i) {
                // TODO: we're ignoring the limit when recombining.
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                rassert(_rr);

                const stream_t *stream = boost::get<stream_t>(&(_rr->result));

                res_stream->insert(res_stream->end(), stream->begin(), stream->end());
                rg_response.truncated = rg_response.truncated || _rr->truncated;
                if (rg_response.last_considered_key < _rr->last_considered_key) {
                    rg_response.last_considered_key = _rr->last_considered_key;
                }
            }
        } else if (const Builtin_GroupedMapReduce *gmr = boost::get<Builtin_GroupedMapReduce>(&*rg->terminal)) {
            //GroupedMapreduce
            rg_response.result = groups_t();
            groups_t *res_groups = boost::get<groups_t>(&rg_response.result);
            for(rri_t i = responses.begin(); i != responses.end(); ++i) {
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                guarantee(_rr);

                const groups_t *groups = boost::get<groups_t>(&(_rr->result));
                query_language::backtrace_t backtrace;

                for (groups_t::const_iterator j = groups->begin(); j != groups->end(); ++j) {
                    query_language::new_val_scope_t scope(&env.scope);
                    Term base = gmr->reduction().base(),
                         body = gmr->reduction().body();

                    env.scope.put_in_scope(gmr->reduction().var1(), get_with_default(*res_groups, j->first, eval(&base, &env, backtrace)));
                    env.scope.put_in_scope(gmr->reduction().var2(), j->second);

                    (*res_groups)[j->first] = eval(&body, &env, backtrace);
                }
            }
        } else if (const Reduction *r = boost::get<Reduction>(&*rg->terminal)) {
            //Normal Mapreduce
            rg_response.result = atom_t();
            atom_t *res_atom = boost::get<atom_t>(&rg_response.result);

            query_language::backtrace_t backtrace;

            Term base = r->base();
            *res_atom = eval(&base, &env, backtrace);

            for(rri_t i = responses.begin(); i != responses.end(); ++i) {
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                guarantee(_rr);

                const atom_t *atom = boost::get<atom_t>(&(_rr->result));

                query_language::new_val_scope_t scope(&env.scope);
                env.scope.put_in_scope(r->var1(), *res_atom);
                env.scope.put_in_scope(r->var2(), *atom);
                Term body = r->body();
                *res_atom = eval(&body, &env, backtrace);
            }
        } else if (boost::get<rdb_protocol_details::Length>(&*rg->terminal)) {
            rg_response.result = atom_t();
            length_t *res_length = boost::get<length_t>(&rg_response.result);
            res_length->length = 0;

            for(rri_t i = responses.begin(); i != responses.end(); ++i) {
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                guarantee(_rr);

                const length_t *length = boost::get<length_t>(&(_rr->result));

                res_length->length += length->length;
            }
        } else if (boost::get<WriteQuery_ForEach>(&*rg->terminal)) {
            rg_response.result = atom_t();
            inserted_t *res_inserted = boost::get<inserted_t>(&rg_response.result);
            res_inserted->inserted = 0;

            for(rri_t i = responses.begin(); i != responses.end(); ++i) {
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                guarantee(_rr);

                const inserted_t *inserted = boost::get<inserted_t>(&(_rr->result));

                res_inserted->inserted += inserted->inserted;
            }
        } else {
            unreachable();
        }
    } else {
        unreachable("Unknown read response.");
    }
}

bool rget_data_cmp(const std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> >& a,
                   const std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> >& b) {
    return a.first < b.first;
}

void read_t::multistore_unshard(std::vector<read_response_t> responses, read_response_t *response, context_t *ctx) const THROWS_NOTHING {
    boost::shared_ptr<js::runner_t> js_runner = boost::make_shared<js::runner_t>();
    query_language::runtime_environment_t env(ctx->pool_group, ctx->ns_repo, ctx->semilattice_metadata, js_runner, &ctx->interruptor, ctx->machine_id);

    const point_read_t *pr = boost::get<point_read_t>(&read);
    const rget_read_t *rg = boost::get<rget_read_t>(&read);
    if (pr) {
        rassert(responses.size() == 1);
        rassert(boost::get<point_read_response_t>(&responses[0].response));
        *response = responses[0];
    } else if (rg) {
        response->response = rget_read_response_t();
        rget_read_response_t &rg_response = boost::get<rget_read_response_t>(response->response);
        rg_response.truncated = false;
        rg_response.key_range = get_region().inner;
        rg_response.last_considered_key = get_region().inner.left;
        typedef std::vector<read_response_t>::iterator rri_t;

        if (!rg->terminal) {
            //A vanilla range get (or filter or map)
            rg_response.result = stream_t(); //Set the response to have the correct result type
            stream_t *res_stream = boost::get<stream_t>(&rg_response.result);

            /* An annoyance occurs. We have results from several different hash
             * shards. We must figure out what the last considered key is,
             * however that value must be the last considered key for all of
             * the hash shards, thus we have to take the minimum of all the
             * shards last considered keys. Observe the picture:
             *
             *              A - - - - - - - - - - - - - - - Z
             * hash shard 1     | -        - -  -  -  |
             * hash shard 2     |  --       -    -   -|
             * hash shard 3     |     --- -   -       |
             * hash shard 4     |-   -         -  - - |
             *
             * Here each shard has returned 5 keys. (Each - is a key). Now the
             * question is what is the last considered key?
             *
             *              A - - - - - - - - - - - - - - - Z
             * hash shard 1     | -        - -  -  -  |
             * hash shard 2     |  --       -    -   a|
             * hash shard 3     |     --- -   b       |
             * hash shard 4     |-   -         -  - - |
             *
             * Is it "a" or "b"? The answer is "b"? If we picked "a" then the
             * next request we got would have "a" as the left side of the
             * range. And we could miss keys in hash shard 3.
             */

            /* Figure out what the last considered key actually is. */
            rg_response.last_considered_key = get_region().inner.last_key_in_range();

            for (rri_t i = responses.begin(); i != responses.end(); ++i) {
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                guarantee(_rr);

                const stream_t *stream = boost::get<stream_t>(&(_rr->result));
                guarantee(stream);


                if (stream->size() == rg->maximum) {
                    if (_rr->last_considered_key < rg_response.last_considered_key) {
                        rg_response.last_considered_key = _rr->last_considered_key;
                    }
                }
            }

            for (rri_t i = responses.begin(); i != responses.end(); ++i) {
                // TODO: we're ignoring the limit when recombining.
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                rassert(_rr);

                const stream_t *stream = boost::get<stream_t>(&(_rr->result));

                for (stream_t::const_iterator jt  = stream->begin();
                                              jt != stream->end();
                                              ++jt) {
                    //Filter out the results that went past our last considered key
                    if (jt->first <= rg_response.last_considered_key) {
                        res_stream->push_back(*jt);
                    }
                }

                //res_stream->insert(res_stream->end(), stream->begin(), stream->end());
                rg_response.truncated = rg_response.truncated || _rr->truncated;
            }
        } else if (const Builtin_GroupedMapReduce *gmr = boost::get<Builtin_GroupedMapReduce>(&*rg->terminal)) {
            //GroupedMapreduce
            rg_response.result = groups_t();
            groups_t *res_groups = boost::get<groups_t>(&rg_response.result);
            for(rri_t i = responses.begin(); i != responses.end(); ++i) {
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                guarantee(_rr);

                const groups_t *groups = boost::get<groups_t>(&(_rr->result));
                query_language::backtrace_t backtrace;

                for (groups_t::const_iterator j = groups->begin(); j != groups->end(); ++j) {
                    query_language::new_val_scope_t scope(&env.scope);
                    Term base = gmr->reduction().base(),
                         body = gmr->reduction().body();

                    env.scope.put_in_scope(gmr->reduction().var1(), get_with_default(*res_groups, j->first, eval(&base, &env, backtrace)));
                    env.scope.put_in_scope(gmr->reduction().var2(), j->second);

                    (*res_groups)[j->first] = eval(&body, &env, backtrace);
                }
            }
        } else if (const Reduction *r = boost::get<Reduction>(&*rg->terminal)) {
            //Normal Mapreduce
            rg_response.result = atom_t();
            atom_t *res_atom = boost::get<atom_t>(&rg_response.result);

            query_language::backtrace_t backtrace;

            Term base = r->base();
            *res_atom = eval(&base, &env, backtrace);

            for(rri_t i = responses.begin(); i != responses.end(); ++i) {
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                guarantee(_rr);

                const atom_t *atom = boost::get<atom_t>(&(_rr->result));

                query_language::new_val_scope_t scope(&env.scope);
                env.scope.put_in_scope(r->var1(), *res_atom);
                env.scope.put_in_scope(r->var2(), *atom);
                Term body = r->body();
                *res_atom = eval(&body, &env, backtrace);
            }
        } else if (boost::get<rdb_protocol_details::Length>(&*rg->terminal)) {
            rg_response.result = atom_t();
            length_t *res_length = boost::get<length_t>(&rg_response.result);
            res_length->length = 0;

            for(rri_t i = responses.begin(); i != responses.end(); ++i) {
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                guarantee(_rr);

                const length_t *length = boost::get<length_t>(&(_rr->result));

                res_length->length += length->length;
            }
        } else if (boost::get<WriteQuery_ForEach>(&*rg->terminal)) {
            rg_response.result = atom_t();
            inserted_t *res_inserted = boost::get<inserted_t>(&rg_response.result);
            res_inserted->inserted = 0;

            for(rri_t i = responses.begin(); i != responses.end(); ++i) {
                const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
                guarantee(_rr);

                const inserted_t *inserted = boost::get<inserted_t>(&(_rr->result));

                res_inserted->inserted += inserted->inserted;
            }
        } else {
            unreachable();
        }
    } else {
        unreachable("Unknown read response.");
    }
}


/* write_t::get_region() implementation */

struct w_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const point_write_t &pw) const {
        return rdb_protocol_t::monokey_region(pw.key);
    }

    region_t operator()(const point_delete_t &pd) const {
        return rdb_protocol_t::monokey_region(pd.key);
    }
};

region_t write_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(w_get_region_visitor(), write);
}

/* write_t::shard implementation */

struct w_shard_visitor : public boost::static_visitor<write_t> {
    explicit w_shard_visitor(const region_t &_region)
        : region(_region)
    { }

    write_t operator()(const point_write_t &pw) const {
        rassert(rdb_protocol_t::monokey_region(pw.key) == region);
        return write_t(pw);
    }
    write_t operator()(const point_delete_t &pd) const {
        rassert(rdb_protocol_t::monokey_region(pd.key) == region);
        return write_t(pd);
    }
    const region_t &region;
};

write_t write_t::shard(const region_t &region) const THROWS_NOTHING {
    return boost::apply_visitor(w_shard_visitor(region), write);
}

void write_t::unshard(std::vector<write_response_t> responses, write_response_t *response, context_t *) const THROWS_NOTHING {
    rassert(responses.size() == 1);
    *response = responses[0];
}

void write_t::multistore_unshard(const std::vector<write_response_t>& responses, write_response_t *response, context_t *ctx) const THROWS_NOTHING {
    return unshard(responses, response, ctx);
}

store_t::store_t(io_backender_t *io_backend,
                 const std::string& filename,
                 bool create,
                 perfmon_collection_t *parent_perfmon_collection,
                 context_t *_ctx) :
    btree_store_t<rdb_protocol_t>(io_backend, filename, create, parent_perfmon_collection, _ctx),
    ctx(_ctx)
{ }

store_t::~store_t() {
    assert_thread();
}

// TODO: get rid of this extra response_t copy on the stack
struct read_visitor_t : public boost::static_visitor<read_response_t> {
    read_response_t operator()(const point_read_t& get) {
        return read_response_t(rdb_get(get.key, btree, txn, superblock));
    }

    read_response_t operator()(const rget_read_t& rget) {
        return read_response_t(rdb_rget_slice(btree, rget.key_range, 1000, txn, superblock, &env, rget.transform, rget.terminal));
    }

    read_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_, rdb_protocol_t::context_t *ctx) :
        btree(btree_), txn(txn_), superblock(superblock_), 
        env(ctx->pool_group, ctx->ns_repo, ctx->semilattice_metadata, boost::make_shared<js::runner_t>(), &ctx->interruptor, ctx->machine_id)
    { }

private:
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    query_language::runtime_environment_t env;
};

void store_t::protocol_read(const read_t &read,
                            read_response_t *response,
                            btree_slice_t *btree,
                            transaction_t *txn,
                            superblock_t *superblock) {
    read_visitor_t v(btree, txn, superblock, ctx);
    *response = boost::apply_visitor(v, read.read);
}

// TODO: get rid of this extra response_t copy on the stack
struct write_visitor_t : public boost::static_visitor<write_response_t> {
    write_response_t operator()(const point_write_t &w) {
        return write_response_t(
            rdb_set(w.key, w.data, btree, timestamp, txn, superblock));
    }

    write_response_t operator()(const point_delete_t &d) {
        return write_response_t(
            rdb_delete(d.key, btree, timestamp, txn, superblock));
    }

    write_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_, repli_timestamp_t timestamp_) :
      btree(btree_), txn(txn_), superblock(superblock_), timestamp(timestamp_) { }

private:
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    repli_timestamp_t timestamp;
};

void store_t::protocol_write(const write_t &write,
                             write_response_t *response,
                             transition_timestamp_t timestamp,
                             btree_slice_t *btree,
                             transaction_t *txn,
                             superblock_t *superblock) {
    write_visitor_t v(btree, txn, superblock, timestamp.to_repli_timestamp());
    *response = boost::apply_visitor(v, write.write);
}

struct backfill_chunk_get_region_visitor_t : public boost::static_visitor<region_t> {
    region_t operator()(const backfill_chunk_t::delete_key_t &del) {
        return rdb_protocol_t::monokey_region(del.key);
    }

    region_t operator()(const backfill_chunk_t::delete_range_t &del) {
        return del.range;
    }

    region_t operator()(const backfill_chunk_t::key_value_pair_t &kv) {
        return rdb_protocol_t::monokey_region(kv.backfill_atom.key);
    }
};

region_t backfill_chunk_t::get_region() const {
    backfill_chunk_get_region_visitor_t v;
    return boost::apply_visitor(v, val);
}

struct backfill_chunk_get_btree_repli_timestamp_visitor_t : public boost::static_visitor<repli_timestamp_t> {
    repli_timestamp_t operator()(const backfill_chunk_t::delete_key_t &del) {
        return del.recency;
    }

    repli_timestamp_t operator()(const backfill_chunk_t::delete_range_t &) {
        return repli_timestamp_t::invalid;
    }

    repli_timestamp_t operator()(const backfill_chunk_t::key_value_pair_t &kv) {
        return kv.backfill_atom.recency;
    }
};

repli_timestamp_t backfill_chunk_t::get_btree_repli_timestamp() const THROWS_NOTHING {
    backfill_chunk_get_btree_repli_timestamp_visitor_t v;
    return boost::apply_visitor(v, val);
}

struct rdb_backfill_callback_impl_t : public rdb_backfill_callback_t {
public:
    typedef backfill_chunk_t chunk_t;

    explicit rdb_backfill_callback_impl_t(chunk_fun_callback_t<rdb_protocol_t> *_chunk_fun_cb)
        : chunk_fun_cb(_chunk_fun_cb) { }
    ~rdb_backfill_callback_impl_t() { }

    void on_delete_range(const key_range_t &range) {
        chunk_fun_cb->send_chunk(chunk_t::delete_range(region_t(range)));
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency) {
        chunk_fun_cb->send_chunk(chunk_t::delete_key(to_store_key(key), recency));
    }

    void on_keyvalue(const rdb_backfill_atom_t& atom) {
        chunk_fun_cb->send_chunk(chunk_t::set_key(atom));
    }

protected:
    store_key_t to_store_key(const btree_key_t *key) {
        return store_key_t(key->size, key->contents);
    }

private:
    chunk_fun_callback_t<rdb_protocol_t> *chunk_fun_cb;

    DISABLE_COPYING(rdb_backfill_callback_impl_t);
};

static void call_rdb_backfill(int i, btree_slice_t *btree, const std::vector<std::pair<region_t, state_timestamp_t> > &regions,
        rdb_backfill_callback_t *callback, transaction_t *txn, superblock_t *superblock, backfill_progress_t *progress,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    parallel_traversal_progress_t *p = new parallel_traversal_progress_t;
    scoped_ptr_t<traversal_progress_t> p_owned(p);
    progress->add_constituent(&p_owned);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    try {
        rdb_backfill(btree, regions[i].first.inner, timestamp, callback, txn, superblock, p, interruptor);
    } catch (interrupted_exc_t) {
        /* do nothing; `protocol_send_backfill()` will notice that interruptor
        has been pulsed */
    }
}

void store_t::protocol_send_backfill(const region_map_t<rdb_protocol_t, state_timestamp_t> &start_point,
                                     chunk_fun_callback_t<rdb_protocol_t> *chunk_fun_cb,
                                     superblock_t *superblock,
                                     btree_slice_t *btree,
                                     transaction_t *txn,
                                     backfill_progress_t *progress,
                                     signal_t *interruptor)
                                     THROWS_ONLY(interrupted_exc_t) {
    rdb_backfill_callback_impl_t callback(chunk_fun_cb);
    std::vector<std::pair<region_t, state_timestamp_t> > regions(start_point.begin(), start_point.end());
    refcount_superblock_t refcount_wrapper(superblock, regions.size());
    pmap(regions.size(), boost::bind(&call_rdb_backfill, _1,
        btree, regions, &callback, txn, &refcount_wrapper, progress, interruptor));

    /* If interruptor was pulsed, `call_rdb_backfill()` exited silently, so we
    have to check directly. */
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

struct receive_backfill_visitor_t : public boost::static_visitor<> {
    receive_backfill_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_, signal_t *interruptor_) :
      btree(btree_), txn(txn_), superblock(superblock_), interruptor(interruptor_) { }

    void operator()(const backfill_chunk_t::delete_key_t& delete_key) const {
        // FIXME: we ignored delete_key.recency here. Should we use it in place of repli_timestamp_t::invalid?
        rdb_delete(delete_key.key, btree, repli_timestamp_t::invalid, txn, superblock);
    }

    void operator()(const backfill_chunk_t::delete_range_t& delete_range) const {
        range_key_tester_t tester(delete_range.range);
        rdb_erase_range(btree, &tester, delete_range.range.inner, txn, superblock);
    }

    void operator()(const backfill_chunk_t::key_value_pair_t& kv) const {
        const rdb_backfill_atom_t& bf_atom = kv.backfill_atom;
        rdb_set(bf_atom.key, bf_atom.value,
                btree, repli_timestamp_t::invalid,
                txn, superblock);
    }

private:
    /* TODO: This might be redundant. I thought that `key_tester_t` was only
    originally necessary because in v1.1.x the hashing scheme might be different
    between the source and destination machines. */
    struct range_key_tester_t : public key_tester_t {
        explicit range_key_tester_t(const region_t& delete_range_) : delete_range(delete_range_) { }
        bool key_should_be_erased(const btree_key_t *key) {
            uint64_t h = hash_region_hasher(key->contents, key->size);
            return delete_range.beg <= h && h < delete_range.end
                && delete_range.inner.contains_key(key->contents, key->size);
        }

        const region_t& delete_range;
    };

    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    signal_t *interruptor;  // FIXME: interruptors are not used in btree code, so this one ignored.
};

void store_t::protocol_receive_backfill(btree_slice_t *btree,
                                        transaction_t *txn,
                                        superblock_t *superblock,
                                        signal_t *interruptor,
                                        const backfill_chunk_t &chunk) {
    boost::apply_visitor(receive_backfill_visitor_t(btree, txn, superblock, interruptor), chunk.val);
}

void store_t::protocol_reset_data(const region_t& subregion,
                                  btree_slice_t *btree,
                                  transaction_t *txn,
                                  superblock_t *superblock) {
    always_true_key_tester_t key_tester;
    rdb_erase_range(btree, &key_tester, subregion.inner, txn, superblock);
}

region_t rdb_protocol_t::cpu_sharding_subspace(int subregion_number, int num_cpu_shards) {
    rassert(subregion_number >= 0);
    rassert(subregion_number < num_cpu_shards);

    // We have to be careful with the math here, to avoid overflow.
    uint64_t width = HASH_REGION_HASH_SIZE / num_cpu_shards;

    uint64_t beg = width * subregion_number;
    uint64_t end = subregion_number + 1 == num_cpu_shards ? HASH_REGION_HASH_SIZE : beg + width;

    return region_t(beg, end, key_range_t::universe());
}


struct backfill_chunk_shard_visitor_t : public boost::static_visitor<rdb_protocol_t::backfill_chunk_t> {
public:
    explicit backfill_chunk_shard_visitor_t(const rdb_protocol_t::region_t &_region) : region(_region) { }
    rdb_protocol_t::backfill_chunk_t operator()(const rdb_protocol_t::backfill_chunk_t::delete_key_t &del) {
        rdb_protocol_t::backfill_chunk_t ret(del);
        rassert(region_is_superset(region, ret.get_region()));
        return ret;
    }
    rdb_protocol_t::backfill_chunk_t operator()(const rdb_protocol_t::backfill_chunk_t::delete_range_t &del) {
        rdb_protocol_t::region_t r = region_intersection(del.range, region);
        rassert(!region_is_empty(r));
        return rdb_protocol_t::backfill_chunk_t(rdb_protocol_t::backfill_chunk_t::delete_range_t(r));
    }
    rdb_protocol_t::backfill_chunk_t operator()(const rdb_protocol_t::backfill_chunk_t::key_value_pair_t &kv) {
        rdb_protocol_t::backfill_chunk_t ret(kv);
        rassert(region_is_superset(region, ret.get_region()));
        return ret;
    }
private:
    const rdb_protocol_t::region_t &region;

    DISABLE_COPYING(backfill_chunk_shard_visitor_t);
};

rdb_protocol_t::backfill_chunk_t rdb_protocol_t::backfill_chunk_t::shard(const rdb_protocol_t::region_t &region) const THROWS_NOTHING {
    backfill_chunk_shard_visitor_t v(region);
    return boost::apply_visitor(v, val);
}
