// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/distribution_progress.hpp"

#include "rdb_protocol/protocol.hpp"
#include "store_view.hpp"

distribution_progress_estimator_t::distribution_progress_estimator_t(
        store_view_t *store,
        signal_t *interruptor) {
    static const int max_depth = 2;
    static const size_t result_limit = 128;

#ifndef NDEBUG
    metainfo_checker_t metainfo_checker(
        store->get_region(),
        [](const region_t &, const binary_blob_t &) { });
#endif

    distribution_read_t distribution_read(max_depth, result_limit);
    read_t read(
        distribution_read, profile_bool_t::DONT_PROFILE, read_mode_t::OUTDATED);
    read_response_t read_response;
    read_token_t read_token;
    store->read(
        DEBUG_ONLY(metainfo_checker, )
        read, &read_response, &read_token, interruptor);
    distribution_counts = std::move(
        boost::get<distribution_read_response_t>(read_response.response).key_counts);

    /* For the progress calculation we need partial sums for each key thus we
    calculate those from the results that the distribution query returns. */
    distribution_counts_sum = 0;
    for (auto &&distribution_count : distribution_counts) {
        distribution_count.second =
            (distribution_counts_sum += distribution_count.second);
    }
}

double distribution_progress_estimator_t::estimate_progress(
        const store_key_t &bound) const {
    if (distribution_counts_sum == 0) {
        return 0.0;
    }
    auto lower_bound = distribution_counts.lower_bound(bound);
    if (lower_bound != distribution_counts.end()) {
        return static_cast<double>(lower_bound->second) /
            static_cast<double>(distribution_counts_sum);
    } else {
        return 1.0;
    }
}

RDB_IMPL_SERIALIZABLE_2(distribution_progress_estimator_t,
    distribution_counts, distribution_counts_sum);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(distribution_progress_estimator_t);
