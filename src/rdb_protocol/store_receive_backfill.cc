#include "rdb_protocol/store.hpp"

#include "btree/backfill.hpp"
#include "btree/reql_specific.hpp"
#include "rdb_protocol/btree.hpp"

/* `MAX_CONCURRENT_BACKFILL_ITEMS` is the maximum number of coroutines we'll spawn in
parallel to apply backfill items to the B-tree. */
static const int MAX_CONCURRENT_BACKFILL_ITEMS = 16;

/* `MAX_CHANGES_PER_TXN` is the maximum number of keys we'll modify or delete in a single
transaction when applying a multi-key backfill item. Increasing this number will make the
backfill faster, but it will also cause the backfill-related transactions to hold the
superblock for a longer time. */
static const int MAX_CHANGES_PER_TXN = 16;

/* `MAX_UNSAVED_CHANGES` is the maximum number of keys we'll modify or delete before
flushing our changes out to disk. This prevents the backfill from using too much of the
cache's unsaved data limit, which would slow down queries on other shards. */
static const int MAX_UNSAVED_CHANGES = 1000;

void flush_cache(cache_conn_t *cache, UNUSED signal_t *interruptor) {
    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    get_btree_superblock_and_txn_for_writing(cache, nullptr,
        write_access_t::write, 1, write_durability_t::HARD, &superblock, &txn);
    buf_write_t write(superblock->get());
    /* `txn`'s destructor will block until all of the transactions that acquired the
    superblock before we did and modified the metainfo have been flushed to disk. It's a
    shame we can't wait on `interruptor` using this API. */
}

class unsaved_data_limiter_t {
public:
    explicit unsaved_data_limiter_t(cache_conn_t *_cache) :
        cache(_cache), semaphore(MAX_UNSAVED_CHANGES)
        { }

    /* `prepare_for_changes()` indicates an intention to change `num_changes` keys. If
    there's too much unsaved data already, then it will block until some of the data is
    flushed. */
    void prepare_for_changes(int num_changes, signal_t *interruptor) {
        scoped_ptr_t<new_semaphore_acq_t> sem_acq =
            make_scoped<new_semaphore_acq_t>(&semaphore, num_changes);
        wait_interruptible(sem_acq->acquisition_signal(), interruptor);
        if (unflushed_sem_acq.has()) {
            unflushed_sem_acq->transfer_in(std::move(*sem_acq));
        } else {
            unflushed_sem_acq = std::move(sem_acq);
        }
        if (unflushed_sem_acq->count() > MAX_UNSAVED_CHANGES / 4) {
            scoped_ptr_t<new_semaphore_acq_t> temp;
            std::swap(temp, unflushed_sem_acq);
            coro_t::spawn_sometime(std::bind(&unsaved_data_limiter_t::flush, this,
                std::move(temp), drainer.lock()));
        }
    }

private:
    void flush(
            const scoped_ptr_t<new_semaphore_acq_t> &,
            auto_drainer_t::lock_t keepalive) {
        try {
            flush_cache(cache, keepalive.get_drain_signal());
        } catch (const interrupted_exc_t &) {
            /* ignore */
        }
        /* Once `flush()` returns, then the `std::bind` will be destroyed, releasing the
        `new_semaphore_acq_t`. */
    }

    cache_conn_t *cache;

    /* Destructor order matters here. We have to destroy `drainer` first, because that
    will stop the `flush()` coroutines. Then we have to destroy `unflushed_sem_acq`
    before `semaphore` because `unflushed_sem_acq` references `semaphore`. */
    new_semaphore_t semaphore;
    scoped_ptr_t<new_semaphore_acq_t> unflushed_sem_acq;
    auto_drainer_t drainer;
};

/* `receive_backfill()` spawns a series of coroutines running `apply_empty_range()`,
`apply_single_key_item()`, and `apply_multi_key_item()`. Each coroutine gets a
`receive_backfill_tokens_t` that keeps track of order, takes care of updating the
sindexes, etc. `receive_backfill_info_t` stores some information that's common to the
entire backfill task, that the tokens need access to. */

class receive_backfill_info_t {
public:
    receive_backfill_info_t(
            cache_conn_t *c, btree_slice_t *s, unsaved_data_limiter_t *l) :
        cache_conn(c), slice(s), limiter(l),
        semaphore(MAX_CONCURRENT_BACKFILL_ITEMS) { }

    /* `cache_conn` and `slice` are just copied from the corresponding fields of the
    `store_t` object */
    cache_conn_t *cache_conn;
    btree_slice_t *slice;

    /* `limiter` lives on the stack in `receive_backfill()` */
    unsaved_data_limiter_t *limiter;

    /* `semaphore` limits how many coroutines can be running at once. */
    new_semaphore_t semaphore;

    /* Every coroutine gets a write token from `fifo_source`. It must acquire
    `btree_fifo_sink` before acquiring the B-tree superblock to modify the B-tree; it
    must acquire `commit_fifo_sink` before calling the `commit_cb` on its tokens. */
    fifo_enforcer_source_t fifo_source;
    fifo_enforcer_sink_t btree_fifo_sink, commit_fifo_sink;

    /* `drainer` ensures that all the coroutines stop if the call to `receive_backfill()`
    is interrupted. */
    auto_drainer_t drainer;
};

class receive_backfill_tokens_t {
public:
    receive_backfill_tokens_t(receive_backfill_info_t *i, signal_t *interruptor) :
        info(i), sem_acq(&info->semaphore, 1),
        write_token(info->fifo_source.enter_write()), keepalive(info->drainer.lock()) {
        wait_interruptible(sem_acq.acquisition_signal(), interruptor);
    }

    receive_backfill_info_t *info;
    new_semaphore_acq_t sem_acq;
    fifo_enforcer_write_token_t write_token;
    auto_drainer_t::lock_t keepalive;

    /* The `apply_*` coroutines call `update_metainfo_cb()` to update the superblock's
    metainfo up to a certain point, and then they call `commit_cb()` to apply sindex
    changes and notify the callback that was passed to `receive_backfill()`. Note that an
    `apply_*` coroutine may call each of these callbacks multiple times, but the calls
    must be in order. */
    std::function<void(
        const key_range_t::right_bound_t &progress,
        real_superblock_t *superblock
        )> update_metainfo_cb;
    std::function<void(
        const key_range_t::right_bound_t &progress,
        scoped_ptr_t<txn_t> &&txn,
        buf_lock_t &&sindex_block,
        std::vector<rdb_modification_report_t> &&mod_reports
        )> commit_cb;
};

/* `apply_empty_range()` is spawned when the `backfill_item_producer_t` generates an
"empty range", indicating that no more backfill items are present before a certain point.
*/
void apply_empty_range(
        const receive_backfill_tokens_t &tokens,
        const key_range_t::right_bound_t &empty_range) {
    try {
        /* Acquire the superblock */
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        {
            fifo_enforcer_sink_t::exit_write_t exiter(
                &tokens.info->btree_fifo_sink, tokens.write_token);
            wait_interruptible(&exiter, tokens.keepalive.get_drain_signal());
            get_btree_superblock_and_txn_for_writing(tokens.info->cache_conn, nullptr,
                write_access_t::write, 1, write_durability_t::SOFT, &superblock, &txn);

            /* Update the metainfo and release the superblock. */
            tokens.update_metainfo_cb(empty_range, superblock.get());
            superblock.reset();
        }

        /* Notify that we're done */
        fifo_enforcer_sink_t::exit_write_t exiter(
            &tokens.info->commit_fifo_sink, tokens.write_token);
        exiter.wait_lazily_unordered();
        tokens.commit_cb(empty_range, std::move(txn), buf_lock_t(),
            std::vector<rdb_modification_report_t>());

    } catch (const interrupted_exc_t &exc) {
        /* The call to `receive_backfill()` was interrupted. Ignore. */
    }
}

/* `apply_item_pair()` is a helper function for `apply_single_key_item()` and
`apply_multi_key_item()`. It applies a single `backfill_item_t::pair_t` to the B-tree.
It doesn't call `on_commit()` or modify the metainfo. */
void apply_item_pair(
        btree_slice_t *slice,
        real_superblock_t *superblock,
        backfill_item_t::pair_t &&pair,
        std::vector<rdb_modification_report_t> *mod_reports_out,
        promise_t<superblock_t *> *pass_back_superblock) {
    rdb_live_deletion_context_t deletion_context;
    mod_reports_out->resize(mod_reports_out->size() + 1);
    mod_reports_out->back().primary_key = pair.key;
    if (static_cast<bool>(pair.value)) {
        vector_read_stream_t read_stream(std::move(*pair.value));
        ql::datum_t datum;
        archive_result_t res = datum_deserialize(&read_stream, &datum);
        guarantee(res == archive_result_t::SUCCESS);

        point_write_response_t dummy_response;
        rdb_set(pair.key, datum, true, slice, pair.recency, superblock,
            &deletion_context, &dummy_response, &mod_reports_out->back().info, nullptr,
            pass_back_superblock);
    } else {
        point_delete_response_t dummy_response;
        rdb_delete(pair.key, slice, pair.recency, superblock,
            &deletion_context, delete_or_erase_t::DELETE, &dummy_response,
            &mod_reports_out->back().info, nullptr, pass_back_superblock);
    }
}

/* `apply_single_key_item()` applies a `backfill_item_t` whose range is a single key wide
and which has a `backfill_item_t::pair_t` for that key. This eliminates the need to erase
the previous contents of the range.

Note that multiple calls to `apply_single_key_item()` can be pipelined efficiently,
because it releases the superblock as soon as it acquires the next node in the B-tree. */
void apply_single_key_item(
        const receive_backfill_tokens_t &tokens,
        /* `item` is conceptually passed by move, but `std::bind()` isn't smart enough to
        handle that. */
        backfill_item_t &item   // NOLINT runtime/references
        ) {
    try {
        /* Acquire the superblock */
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        buf_lock_t sindex_block;
        {
            fifo_enforcer_sink_t::exit_write_t exiter(
                &tokens.info->btree_fifo_sink, tokens.write_token);
            wait_interruptible(&exiter, tokens.keepalive.get_drain_signal());

            /* Block until there's not too much unsaved data. We have to be careful about
            deadlocks; acquiring `limiter` only when we also hold `btree_fifo_sink` is
            sufficient to prevent deadlocks. */
            tokens.info->limiter->prepare_for_changes(
                1, tokens.keepalive.get_drain_signal());

            get_btree_superblock_and_txn_for_writing(tokens.info->cache_conn, nullptr,
                write_access_t::write, 1, write_durability_t::SOFT, &superblock, &txn);

            /* Acquire the sindex block and update the metainfo now, because we'll
            release the superblock soon */
            sindex_block = buf_lock_t(superblock->expose_buf(),
                superblock->get_sindex_block_id(), access_t::write);
            tokens.update_metainfo_cb(item.range.right, superblock.get());
        }

        /* Actually apply the change, releasing the superblock in the process. */
        std::vector<rdb_modification_report_t> mod_reports;
        apply_item_pair(tokens.info->slice, superblock.get(),
            std::move(item.pairs[0]), &mod_reports, nullptr);

        /* Notify that we're done and update the sindexes */
        fifo_enforcer_sink_t::exit_write_t exiter(
            &tokens.info->commit_fifo_sink, tokens.write_token);
        /* Note: This must not be interruptible, or we might miss updating secondary
        indexes. */
        exiter.wait_lazily_unordered();
        tokens.commit_cb(item.range.right, std::move(txn), std::move(sindex_block),
            std::move(mod_reports));

    } catch (const interrupted_exc_t &exc) {
        /* The call to `receive_backfill()` was interrupted. Ignore. */
    }
}

/* `apply_multi_key_item()` is for items that apply to a range of keys. We must first
delete any existing values or deletion entries in that range, and then apply the contents
of `item.pairs`. */
void apply_multi_key_item(
        const receive_backfill_tokens_t &tokens,
        /* `item` is conceptually passed by move, but `std::bind()` isn't smart enough to
        handle that. */
        backfill_item_t &item   // NOLINT runtime/references
        ) {
    try {
        /* Acquire and hold both `fifo_enforcer_sink_t`s until we're completely finished;
        since we're going to be making multiple B-tree queries in separate B-tree
        transactions, we can't pipeline with other backfill items */
        fifo_enforcer_sink_t::exit_write_t exiter1(
            &tokens.info->btree_fifo_sink, tokens.write_token);
        wait_interruptible(&exiter1, tokens.keepalive.get_drain_signal());
        fifo_enforcer_sink_t::exit_write_t exiter2(
            &tokens.info->commit_fifo_sink, tokens.write_token);
        wait_interruptible(&exiter2, tokens.keepalive.get_drain_signal());

        /* It's possible that there are a lot of keys to be deleted, so we might do the
        backfill item in several chunks. */
        bool is_first = true;
        size_t next_pair = 0;
        key_range_t::right_bound_t threshold(item.range.left);
        while (threshold != item.range.right) {
            std::vector<rdb_modification_report_t> mod_reports;

            /* Block until there's not too much unsaved data. Note that
            `MAX_CHANGES_PER_TXN` might be an overestimate, but that's OK. */
            tokens.info->limiter->prepare_for_changes(
                MAX_CHANGES_PER_TXN, tokens.keepalive.get_drain_signal());

            /* Acquire the superblock. */
            scoped_ptr_t<txn_t> txn;
            scoped_ptr_t<real_superblock_t> superblock;
            get_btree_superblock_and_txn_for_writing(tokens.info->cache_conn, nullptr,
                write_access_t::write, 1, write_durability_t::SOFT, &superblock, &txn);

            /* If we haven't already done so, then update the min deletion timstamps.
            See `btree/backfill.hpp` for an explanation of what this is. Note that we do
            this all at once for the entire range; we don't need to break it up into
            chunks because it's a fairly inexpensive operation and because it's a
            "conservative" operation, so it's safe if we affect parts of the key-space
            that we don't actually call `commit_cb()` for. */
            if (is_first) {
                rdb_value_sizer_t sizer(superblock->cache()->max_block_size());
                btree_receive_backfill_item_update_deletion_timestamps(
                    superblock.get(), release_superblock_t::KEEP, &sizer, item,
                    tokens.keepalive.get_drain_signal());
                is_first = false;
            }

            /* Establish an upper limit on how much of the range we're willing to delete
            in this cycle. We choose the upper limit such that it contains no more than
            `MAX_CHANGES_PER_TXN / 2` of the pairs in the backfill item. */
            key_range_t range_to_delete;
            range_to_delete.left = threshold.key();
            if (next_pair + MAX_CHANGES_PER_TXN / 2 + 1 < item.pairs.size()) {
                range_to_delete.right = key_range_t::right_bound_t(
                    item.pairs[next_pair + MAX_CHANGES_PER_TXN / 2 + 1].key);
            } else {
                range_to_delete.right = item.range.right;
            }

            /* Delete a chunk of the range, making sure to do no more than
            `MAX_CHANGES_PER_TXN / 2` changes at once. */
            always_true_key_tester_t key_tester;
            key_range_t range_deleted;
            rdb_live_deletion_context_t deletion_context;
            continue_bool_t res = rdb_erase_small_range(tokens.info->slice, &key_tester,
                range_to_delete, superblock.get(), &deletion_context,
                tokens.keepalive.get_drain_signal(), MAX_CHANGES_PER_TXN / 2,
                &mod_reports, &range_deleted);
            guarantee(range_deleted.right == range_to_delete.right
                || res == continue_bool_t::CONTINUE);

            /* Apply any pairs from the item that fall within the deleted region */
            while (next_pair < item.pairs.size() &&
                    range_deleted.contains_key(item.pairs[next_pair].key)) {
                promise_t<superblock_t *> pass_back_superblock;
                apply_item_pair(tokens.info->slice, superblock.get(),
                    std::move(item.pairs[next_pair]), &mod_reports,
                    &pass_back_superblock);
                guarantee(superblock.get() == pass_back_superblock.assert_get_value());
                ++next_pair;
            }

            /* Update `threshold` to reflect the changes we've made */
            threshold = range_deleted.right;

            /* Acquire the sindex block and update the metainfo */
            buf_lock_t sindex_block(superblock->expose_buf(),
                superblock->get_sindex_block_id(), access_t::write);
            tokens.update_metainfo_cb(threshold, superblock.get());
            superblock->release();

            /* Notify the callback of our progress and update the sindexes */
            tokens.commit_cb(threshold, std::move(txn), std::move(sindex_block),
                std::move(mod_reports));
        }

    } catch (const interrupted_exc_t &exc) {
        /* The call to `receive_backfill()` was interrupted. Ignore. */
    }
}

continue_bool_t store_t::receive_backfill(
        const region_t &region,
        backfill_item_producer_t *item_producer,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    guarantee(region.beg == get_region().beg && region.end == get_region().end);

    unsaved_data_limiter_t unsaved_data_limiter(general_cache_conn.get());
    receive_backfill_info_t info(
        general_cache_conn.get(), btree.get(), &unsaved_data_limiter);

    /* `spawn_threshold` is the point up to which we've spawned coroutines.
    `metainfo_threshold` is the point up to which we've applied the metainfo to the
    superblock. `commit_threshold` is the point up to which we've called
    `item_producer->on_commit()`. These all increase monotonically to the right. */
    key_range_t::right_bound_t
        spawn_threshold(region.inner.left),
        metainfo_threshold(region.inner.left),
        commit_threshold(region.inner.left);

    /* We'll set `result` to `false` to record if `item_producer` returns `ABORT`. */
    continue_bool_t result = continue_bool_t::CONTINUE;

    /* Repeatedly request items from `item_producer` and spawn coroutines to handle them,
    but limit the number of simultaneously active coroutines. */
    while (spawn_threshold != region.inner.right) {
        bool is_item;
        backfill_item_t item;
        key_range_t::right_bound_t empty_range;
        if (continue_bool_t::ABORT ==
                item_producer->next_item(&is_item, &item, &empty_range)) {
            /* By breaking out of the loop instead of returning immediately, we ensure
            that we commit every item that we got from the item producer, as we are
            required to. */
            result = continue_bool_t::ABORT;
            break;
        }

        receive_backfill_tokens_t tokens(&info, interruptor);

        if (is_item) {
            rassert(key_range_t::right_bound_t(item.get_range().left)
                >= spawn_threshold);
            spawn_threshold = item.get_range().right;
        } else {
            rassert(empty_range >= spawn_threshold);
            spawn_threshold = empty_range;
        }

        /* The `apply_*()` functions will call back to `update_metainfo_cb` when they
        want to apply the metainfo to the superblock. They may make multiple calls, but
        the last call will have `progress` equal to `item.get_range().right`. */
        tokens.update_metainfo_cb = [this, &region, &metainfo_threshold, &item_producer,
                    &spawn_threshold](
                const key_range_t::right_bound_t &progress,
                real_superblock_t *superblock) {
            /* Compute the section of metainfo we're applying */
            guarantee(progress >= metainfo_threshold);
            guarantee(progress <= spawn_threshold);
            if (progress == metainfo_threshold) {
                /* This is a no-op */
                return;
            }
            region_t mask = region;
            mask.inner.left = metainfo_threshold.key();
            mask.inner.right = progress;
            metainfo_threshold = progress;

            /* Actually apply the metainfo */
            metainfo->update(superblock, item_producer->get_metainfo()->mask(mask));
        };

        /* The `apply_*()` functions will call back to `commit_cb` when they're done
        applying the changes for a given sub-region. They may make multiple calls, but
        the last call will have `progress` equal to `item.get_range().right`. */
        tokens.commit_cb = [this, item_producer, &commit_threshold, &metainfo_threshold](
                const key_range_t::right_bound_t &progress,
                scoped_ptr_t<txn_t> &&txn,
                buf_lock_t &&sindex_block,
                std::vector<rdb_modification_report_t> &&mod_reports) {
            /* Apply the modifications */
            if (!mod_reports.empty()) {
                update_sindexes(txn.get(), &sindex_block, mod_reports, true);
            } else {
                sindex_block.reset_buf_lock();
            }

            /* End the transaction and notify that we've made progress */
            txn.reset();
            guarantee(progress >= commit_threshold);
            guarantee(progress <= metainfo_threshold);
            if (progress == commit_threshold) {
                /* This is a no-op */
                guarantee(mod_reports.empty());
                return;
            }
            commit_threshold = progress;
            item_producer->on_commit(progress);
        };

        if (!is_item) {
            coro_t::spawn_sometime(std::bind(
                &apply_empty_range, std::move(tokens), empty_range));
        } else if (item.is_single_key()) {
            coro_t::spawn_sometime(std::bind(
                &apply_single_key_item, std::move(tokens), std::move(item)));
        } else {
            coro_t::spawn_sometime(std::bind(
                &apply_multi_key_item, std::move(tokens), std::move(item)));
        }
    }

    /* Wait for any running coroutines to finish. We construct an `exit_write_t` instead
    of just destroying `info.drainer` because we don't want to interrupt the coroutines
    unless `interruptor` is pulsed. */
    fifo_enforcer_sink_t::exit_write_t exiter(
        &info.commit_fifo_sink, info.fifo_source.enter_write());
    wait_interruptible(&exiter, interruptor);

    if (result == continue_bool_t::CONTINUE) {
        guarantee(spawn_threshold == region.inner.right);
        guarantee(metainfo_threshold == region.inner.right);
        guarantee(commit_threshold == region.inner.right);
    }

    /* Flush remaining data to disk. This serves two purposes. The first reason is
    correctness; it's potentially dangerous to report that we finished the backfill when
    there is data that's not yet safely on disk. The other is to make sure that we don't
    use increasingly large amounts of the unsaved data limit if `receive_backfill()` is
    called repeatedly. */
    flush_cache(general_cache_conn.get(), interruptor);

    return result;
}

void store_t::wait_until_ok_to_receive_backfill(signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    // As long as our secondary index post construction is implemented using a
    // secondary index mod queue, it doesn't make sense from a performance point
    // of view to run secondary index post construction and backfilling at the same
    // time. We pause backfilling while any index post construction is going on on
    // this store by acquiring a read lock on `backfill_postcon_lock`.
    rwlock_in_line_t lock_acq(&backfill_postcon_lock, access_t::read);
    wait_interruptible(lock_acq.read_signal(), interruptor);
}

