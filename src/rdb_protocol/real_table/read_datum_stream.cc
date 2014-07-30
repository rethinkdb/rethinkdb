#include "rdb_protocol/real_table/read_datum_stream.hpp"

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/real_table/convert_key.hpp"

namespace ql {

// RANGE/READGEN STUFF
reader_t::reader_t(
    const real_table_t &_table,
    bool _use_outdated,
    scoped_ptr_t<readgen_t> &&_readgen)
    : table(_table),
      use_outdated(_use_outdated),
      started(false), shards_exhausted(false),
      readgen(std::move(_readgen)),
      active_range(readgen->original_keyrange()),
      items_index(0) { }

void reader_t::add_transformation(ql::transform_variant_t &&tv) {
    r_sanity_check(!started);
    transforms.push_back(std::move(tv));
}

void reader_t::accumulate(ql::env_t *env, ql::eager_acc_t *acc,
        const ql::terminal_variant_t &tv) {
    r_sanity_check(!started);
    started = shards_exhausted = true;
    ql::batchspec_t batchspec = ql::batchspec_t::user(ql::batch_type_t::TERMINAL, env);
    read_t read = readgen->terminal_read(transforms, tv, batchspec);
    ql::result_t res = do_read(env, std::move(read)).result;
    acc->add_res(env, &res);
}

void reader_t::accumulate_all(ql::env_t *env, ql::eager_acc_t *acc) {
    r_sanity_check(!started);
    started = true;
    ql::batchspec_t batchspec = ql::batchspec_t::all();
    read_t read = readgen->next_read(active_range, transforms, batchspec);
    rget_read_response_t resp = do_read(env, std::move(read));

    auto rr = boost::get<rget_read_t>(&read.read);
    auto final_key = !reversed(rr->sorting) ? store_key_t::max() : store_key_t::min();
    r_sanity_check(resp.last_key == final_key);
    r_sanity_check(!resp.truncated);
    shards_exhausted = true;

    acc->add_res(env, &resp.result);
}

rget_read_response_t reader_t::do_read(ql::env_t *env, const read_t &read) {
    read_response_t res;
    table.read_with_profile(env, read, &res, use_outdated);
    auto rget_res = boost::get<rget_read_response_t>(&res.response);
    r_sanity_check(rget_res != NULL);
    if (auto e = boost::get<ql::exc_t>(&rget_res->result)) {
        throw *e;
    }
    return std::move(*rget_res);
}

std::vector<ql::rget_item_t> reader_t::do_range_read(ql::env_t *env,
                                                     const read_t &read) {
    rget_read_response_t res = do_read(env, read);

    // It's called `do_range_read`.  If we have more than one type of range
    // read (which we might; rget_read_t should arguably be two types), this
    // will have to be a visitor.
    auto rr = boost::get<rget_read_t>(&read.read);
    r_sanity_check(rr);
    const key_range_t &rng = rr->sindex? rr->sindex->region.inner : rr->region.inner;

    // We need to do some adjustments to the last considered key so that we
    // update the range correctly in the case where we're reading a subportion
    // of the total range.
    store_key_t *key = &res.last_key;
    if (*key == store_key_t::max() && !reversed(rr->sorting)) {
        if (!rng.right.unbounded) {
            *key = rng.right.key;
            bool b = key->decrement();
            r_sanity_check(b);
        }
    } else if (*key == store_key_t::min() && reversed(rr->sorting)) {
        *key = rng.left;
    }

    shards_exhausted = readgen->update_range(&active_range, res.last_key);
    ql::grouped_t<ql::stream_t> *gs =
        boost::get<ql::grouped_t<ql::stream_t> >(&res.result);
    return ql::groups_to_batch(gs->get_underlying_map());
}

bool reader_t::load_items(ql::env_t *env, const ql::batchspec_t &batchspec) {
    started = true;
    if (items_index >= items.size() && !shards_exhausted) { // read some more
        items_index = 0;
        items = do_range_read(
            env, readgen->next_read(active_range, transforms, batchspec));
        // Everything below this point can handle `items` being empty (this is
        // good hygiene anyway).
        while (boost::optional<read_t> read = readgen->sindex_sort_read(
                   active_range, items, transforms, batchspec)) {
            std::vector<ql::rget_item_t> new_items = do_range_read(env, *read);
            if (new_items.size() == 0) {
                break;
            }

            rcheck_datum(
                (items.size() + new_items.size()) <= env->limits.array_size_limit(),
                ql::base_exc_t::GENERIC,
                strprintf("Too many rows (> %zu) with the same "
                          "truncated key for index `%s`.  "
                          "Example value:\n%s\n"
                          "Truncated key:\n%s",
                          env->limits.array_size_limit(),
                          readgen->sindex_name().c_str(),
                          items[items.size() - 1].sindex_key->trunc_print().c_str(),
                          key_to_debug_str(items[items.size() - 1].key).c_str()));

            items.reserve(items.size() + new_items.size());
            std::move(new_items.begin(), new_items.end(), std::back_inserter(items));
        }
        readgen->sindex_sort(&items);
    }
    if (items_index >= items.size()) {
        shards_exhausted = true;
    }
    return items_index < items.size();
}

std::vector<counted_t<const ql::datum_t> >
reader_t::next_batch(ql::env_t *env, const ql::batchspec_t &batchspec) {
    started = true;
    if (!load_items(env, batchspec)) {
        return std::vector<counted_t<const ql::datum_t> >();
    }
    r_sanity_check(items_index < items.size());

    std::vector<counted_t<const ql::datum_t> > res;
    switch (batchspec.get_batch_type()) {
    case ql::batch_type_t::NORMAL: // fallthru
    case ql::batch_type_t::NORMAL_FIRST: // fallthru
    case ql::batch_type_t::TERMINAL: {
        res.reserve(items.size() - items_index);
        for (; items_index < items.size(); ++items_index) {
            res.push_back(std::move(items[items_index].data));
        }
    } break;
    case ql::batch_type_t::SINDEX_CONSTANT: {
        counted_t<const ql::datum_t> sindex = std::move(items[items_index].sindex_key);
        store_key_t key = std::move(items[items_index].key);
        res.push_back(std::move(items[items_index].data));
        items_index += 1;

        bool maybe_more_with_sindex = true;
        while (maybe_more_with_sindex) {
            for (; items_index < items.size(); ++items_index) {
                if (sindex.has()) {
                    r_sanity_check(items[items_index].sindex_key.has());
                    if (*items[items_index].sindex_key != *sindex) {
                        break; // batch is done
                    }
                } else {
                    r_sanity_check(!items[items_index].sindex_key.has());
                    if (items[items_index].key != key) {
                        break;
                    }
                }
                res.push_back(std::move(items[items_index].data));

                rcheck_datum(
                    res.size() <= env->limits.array_size_limit(),
                    ql::base_exc_t::GENERIC,
                    strprintf("Too many rows (> %zu) with the same value "
                              "for index `%s`:\n%s",
                              env->limits.array_size_limit(),
                              readgen->sindex_name().c_str(),
                              // This is safe because you can't have duplicate
                              // primary keys, so they will never exceed the
                              // array limit.
                              sindex->trunc_print().c_str()));
            }
            if (items_index >= items.size()) {
                // If we consumed the whole batch without finding a new sindex,
                // we might have more rows with the same sindex in the next
                // batch, which we promptly load.
                maybe_more_with_sindex = load_items(env, batchspec);
            } else {
                maybe_more_with_sindex = false;
            }
        }
    } break;
    default: unreachable();
    }

    if (items_index >= items.size()) { // free memory immediately
        items_index = 0;
        std::vector<ql::rget_item_t> tmp;
        tmp.swap(items);
    }

    shards_exhausted = (res.size() == 0) ? true : shards_exhausted;
    return res;
}

bool reader_t::is_finished() const {
    return shards_exhausted && items_index >= items.size();
}

readgen_t::readgen_t(
    const std::map<std::string, ql::wire_func_t> &_global_optargs,
    std::string _table_name,
    const ql::datum_range_t &_original_datum_range,
    profile_bool_t _profile,
    sorting_t _sorting)
    : global_optargs(_global_optargs),
      table_name(std::move(_table_name)),
      original_datum_range(_original_datum_range),
      profile(_profile),
      sorting(_sorting) { }

bool readgen_t::update_range(key_range_t *active_range,
                             const store_key_t &last_key) const {
    if (!reversed(sorting)) {
        active_range->left = last_key;
    } else {
        active_range->right = key_range_t::right_bound_t(last_key);
    }

    // TODO: mixing these non-const operations INTO THE CONDITIONAL is bad, and
    // confused me (mlucy) for a while when I tried moving some stuff around.
    if (!reversed(sorting)) {
        if (!active_range->left.increment()
            || (!active_range->right.unbounded
                && (active_range->right.key < active_range->left))) {
            return true;
        }
    } else {
        r_sanity_check(!active_range->right.unbounded);
        if (!active_range->right.key.decrement()
            || active_range->right.key < active_range->left) {
            return true;
        }
    }
    return active_range->is_empty();
}

read_t readgen_t::next_read(
    const key_range_t &active_range,
    const std::vector<ql::transform_variant_t> &transforms,
    const ql::batchspec_t &batchspec) const {
    return read_t(next_read_impl(active_range, transforms, batchspec), profile);
}

// TODO: this is how we did it before, but it sucks.
read_t readgen_t::terminal_read(
    const std::vector<ql::transform_variant_t> &transforms,
    const ql::terminal_variant_t &_terminal,
    const ql::batchspec_t &batchspec) const {
    rget_read_t read = next_read_impl(original_keyrange(), transforms, batchspec);
    read.terminal = _terminal;
    return read_t(read, profile);
}

primary_readgen_t::primary_readgen_t(
    const std::map<std::string, ql::wire_func_t> &global_optargs,
    std::string table_name,
    ql::datum_range_t range,
    profile_bool_t profile,
    sorting_t sorting)
    : readgen_t(global_optargs, std::move(table_name), range, profile, sorting) { }

scoped_ptr_t<readgen_t> primary_readgen_t::make(
    ql::env_t *env,
    std::string table_name,
    ql::datum_range_t range,
    sorting_t sorting) {
    return scoped_ptr_t<readgen_t>(
        new primary_readgen_t(
            env->global_optargs.get_all_optargs(),
            std::move(table_name),
            range,
            env->profile(),
            sorting));
}

rget_read_t primary_readgen_t::next_read_impl(
    const key_range_t &active_range,
    const std::vector<ql::transform_variant_t> &transforms,
    const ql::batchspec_t &batchspec) const {
    return rget_read_t(
        region_t(active_range),
        global_optargs,
        table_name,
        batchspec,
        transforms,
        boost::optional<ql::terminal_variant_t>(),
        boost::optional<sindex_rangespec_t>(),
        sorting);
}

// We never need to do an sindex sort when indexing by a primary key.
boost::optional<read_t> primary_readgen_t::sindex_sort_read(
    UNUSED const key_range_t &active_range,
    UNUSED const std::vector<ql::rget_item_t> &items,
    UNUSED const std::vector<ql::transform_variant_t> &transforms,
    UNUSED const ql::batchspec_t &batchspec) const {
    return boost::optional<read_t>();
}
void primary_readgen_t::sindex_sort(UNUSED std::vector<ql::rget_item_t> *vec) const {
    return;
}

key_range_t primary_readgen_t::original_keyrange() const {
    return datum_range_to_primary_keyrange(original_datum_range);
}

std::string primary_readgen_t::sindex_name() const {
    return "";
}

sindex_readgen_t::sindex_readgen_t(
    const std::map<std::string, ql::wire_func_t> &global_optargs,
    std::string table_name,
    const std::string &_sindex,
    ql::datum_range_t range,
    profile_bool_t profile,
    sorting_t sorting)
    : readgen_t(global_optargs, std::move(table_name), range, profile, sorting),
      sindex(_sindex) { }

scoped_ptr_t<readgen_t> sindex_readgen_t::make(
    ql::env_t *env,
    std::string table_name,
    const std::string &sindex,
    ql::datum_range_t range,
    sorting_t sorting) {
    return scoped_ptr_t<readgen_t>(
        new sindex_readgen_t(
            env->global_optargs.get_all_optargs(),
            std::move(table_name),
            sindex,
            range,
            env->profile(),
            sorting));
}

class sindex_compare_t {
public:
    explicit sindex_compare_t(sorting_t _sorting) : sorting(_sorting) { }
    bool operator()(const ql::rget_item_t &l, const ql::rget_item_t &r) {
        r_sanity_check(l.sindex_key.has() && r.sindex_key.has());
        return reversed(sorting)
            ? (*l.sindex_key > *r.sindex_key)
            : (*l.sindex_key < *r.sindex_key);
    }
private:
    sorting_t sorting;
};

void sindex_readgen_t::sindex_sort(std::vector<ql::rget_item_t> *vec) const {
    if (vec->size() == 0) {
        return;
    }
    if (sorting != sorting_t::UNORDERED) {
        std::stable_sort(vec->begin(), vec->end(), sindex_compare_t(sorting));
    }
}

rget_read_t sindex_readgen_t::next_read_impl(
    const key_range_t &active_range,
    const std::vector<ql::transform_variant_t> &transforms,
    const ql::batchspec_t &batchspec) const {
    return rget_read_t(
        region_t::universe(),
        global_optargs,
        table_name,
        batchspec,
        transforms,
        boost::optional<ql::terminal_variant_t>(),
        sindex_rangespec_t(sindex, region_t(active_range), original_datum_range),
        sorting);
}

boost::optional<read_t> sindex_readgen_t::sindex_sort_read(
    const key_range_t &active_range,
    const std::vector<ql::rget_item_t> &items,
    const std::vector<ql::transform_variant_t> &transforms,
    const ql::batchspec_t &batchspec) const {
    if (sorting != sorting_t::UNORDERED && items.size() > 0) {
        const store_key_t &key = items[items.size() - 1].key;
        if (ql::datum_t::key_is_truncated(key)) {
            std::string skey = ql::datum_t::extract_secondary(key_to_unescaped_str(key));
            key_range_t rng = active_range;
            if (!reversed(sorting)) {
                // We construct a right bound that's larger than the maximum
                // possible row with this truncated sindex but smaller than the
                // minimum possible row with a larger sindex.
                rng.right = key_range_t::right_bound_t(
                    store_key_t(skey + std::string(MAX_KEY_SIZE - skey.size(), 0xFF)));
            } else {
                // We construct a left bound that's smaller than the minimum
                // possible row with this truncated sindex but larger than the
                // maximum possible row with a smaller sindex.
                rng.left = store_key_t(skey);
            }
            if (rng.right.unbounded || rng.left < rng.right.key) {
                return read_t(
                    rget_read_t(
                        region_t::universe(),
                        global_optargs,
                        table_name,
                        batchspec.with_new_batch_type(ql::batch_type_t::SINDEX_CONSTANT),
                        transforms,
                        boost::optional<ql::terminal_variant_t>(),
                        sindex_rangespec_t(
                            sindex,
                            region_t(key_range_t(rng)),
                            original_datum_range),
                        sorting),
                    profile);
            }
        }
    }
    return boost::optional<read_t>();
}

key_range_t sindex_readgen_t::original_keyrange() const {
    return datum_range_to_sindex_keyrange(original_datum_range);
}

std::string sindex_readgen_t::sindex_name() const {
    return sindex;
}

// LAZY_DATUM_STREAM_T
lazy_datum_stream_t::lazy_datum_stream_t(
    const real_table_t &_table,
    bool use_outdated,
    scoped_ptr_t<readgen_t> &&readgen,
    const ql::protob_t<const Backtrace> &bt_src)
    : datum_stream_t(bt_src),
      current_batch_offset(0),
      reader(_table, use_outdated, std::move(readgen)) { }

void lazy_datum_stream_t::add_transformation(ql::transform_variant_t &&tv,
                                             const ql::protob_t<const Backtrace> &bt) {
    reader.add_transformation(std::move(tv));
    update_bt(bt);
}

void lazy_datum_stream_t::accumulate(
    ql::env_t *env, ql::eager_acc_t *acc, const ql::terminal_variant_t &tv) {
    reader.accumulate(env, acc, tv);
}

void lazy_datum_stream_t::accumulate_all(ql::env_t *env, ql::eager_acc_t *acc) {
    reader.accumulate_all(env, acc);
}

std::vector<counted_t<const ql::datum_t> >
lazy_datum_stream_t::next_batch_impl(ql::env_t *env, const ql::batchspec_t &batchspec) {
    // Should never mix `next` with `next_batch`.
    r_sanity_check(current_batch_offset == 0 && current_batch.size() == 0);
    return reader.next_batch(env, batchspec);
}

bool lazy_datum_stream_t::is_exhausted() const {
    return reader.is_finished() && batch_cache_exhausted();
}
bool lazy_datum_stream_t::is_cfeed() const {
    return false;
}

}  // namespace ql
