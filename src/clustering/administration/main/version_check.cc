// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/main/version_check.hpp"

#include <math.h>

#include "logger.hpp"

#include "clustering/administration/metadata.hpp"
#include "extproc/http_runner.hpp"
#include "rdb_protocol/env.hpp"

namespace ql {
void dispatch_http(ql::env_t *env,
                   const http_opts_t &opts,
                   http_runner_t *runner,
                   http_result_t *res_out,
                   const ql::pb_rcheckable_t *parent);
};

version_checker_t::version_checker_t(rdb_context_t *_rdb_ctx, signal_t *_interruptor,
                                     version_checker_t::metadata_ptr_t _metadata,
                                     const std::string &_uname) :
    rdb_ctx(_rdb_ctx),
    interruptor(_interruptor),
    seen_version(),
    metadata(_metadata),
    uname(_uname) {
    rassert(rdb_ctx != NULL);
}

void version_checker_t::initial_check() {
    logINF("Beginning initial checkin");

    ql::env_t env(rdb_ctx, interruptor, std::map<std::string, ql::wire_func_t>(), nullptr);
    http_opts_t opts;
    opts.limits = env.limits();
    opts.result_format = http_result_format_t::JSON;
    opts.url = strprintf("http://update.rethinkdb.com/update_for/%s",
                         RETHINKDB_VERSION);

    http_runner_t runner(env.get_extproc_pool());
    http_result_t result;

    try {
        dispatch_http(&env, opts, &runner, &result, nullptr);
    } catch (const ql::base_exc_t &ex) {
        logWRN("Saw exception `%s` in initial checkin", ex.what());
    }

    process_result(result);
}

void version_checker_t::periodic_checkin(auto_drainer_t::lock_t) {
    logINF("Beginning periodic checkin");

    const cluster_semilattice_metadata_t snapshot = metadata->get();
    ql::env_t env(rdb_ctx, interruptor, std::map<std::string, ql::wire_func_t>(), nullptr);
    http_opts_t opts;
    opts.method = http_method_t::POST;
    opts.limits = env.limits();
    opts.result_format = http_result_format_t::JSON;
    opts.url = "http://update.rethinkdb.com/checkin";
    opts.header.push_back("Content-Type: application/x-www-form-urlencoded");
    opts.form_data["Version"] = RETHINKDB_VERSION;
    opts.form_data["Number-Of-Servers"] = snapshot.servers.servers.size();
    opts.form_data["Uname"] = uname;
    opts.form_data["Cooked-Number-Of-Tables"]
        = strprintf("%" PR_RECONSTRUCTABLE_DOUBLE,
                    cook(snapshot.rdb_namespaces->namespaces.size()));
    //opts.form_data["Cooked-Size-Of-Shards"]
    //    = strprintf("%" PR_RECONSTRUCTABLE_DOUBLE, cook(0.0)); // XXX
    logINF("server # %lu", snapshot.servers.servers.size());
    logINF("uname %s", uname.c_str());
    logINF("tables %lu", snapshot.rdb_namespaces->namespaces.size());
    logINF("databases %lu", snapshot.databases.databases.size());

    http_runner_t runner(env.get_extproc_pool());
    http_result_t result;

    try {
        dispatch_http(&env, opts, &runner, &result, nullptr);
    } catch (const ql::base_exc_t &ex) {
        logWRN("Saw exception `%s` in periodic checkin", ex.what());
    }

    process_result(result);
}

// sort of anonymize the input; specifically we want $2^(round(log_2(n)))$
double version_checker_t::cook(double n) {
    return exp2(round(log2(n)));
}

void version_checker_t::process_result(const http_result_t &result) {
    if (!result.error.empty()) {
        logWRN("Experienced protocol error attempting to check for updates: saw %s",
               result.error.c_str());
        return;
    }

    if (!result.body.has()) {
        logWRN("No body returned; aborting.");
        return;
    }

    ql::datum_t status = result.body.get_field("status", ql::NOTHROW);
    if (!status.has()) {
        logWRN("Got bizarre result from checking for updates; ignoring.");
        logDBG("Saw invalid datum %s; headers are %s",
               result.body.trunc_print().c_str(), result.header.trunc_print().c_str());
        return;
    } else if (status.get_type() != ql::datum_t::R_STR) {
        logWRN("Got malformed result from checking for updates; ignoring.");
        logDBG("Saw invalid status field %s; datum %s; headers are %s",
               status.trunc_print().c_str(), result.body.trunc_print().c_str(),
               result.header.trunc_print().c_str());
        return;
    }
    const datum_string_t &str = status.as_str();
    if (str == "ok") {
        logDBG("Server up-to-date");
    } else if (str == "error") {
        ql::datum_t reason = result.body.get_field("error", ql::NOTHROW);
        if (!reason.has() || reason.get_type() != ql::datum_t::R_STR) {
            logWRN("Remote server had some kind of problem: %s",
                   result.body.trunc_print().c_str());
        } else {
            logWRN("Remote server returned error message %s when checking for updates.",
                   reason.trunc_print().c_str());
        }
    } else if (str == "need_update") {
        ql::datum_t new_version = result.body.get_field("last_version", ql::NOTHROW);
        if (!new_version.has() || new_version.get_type() != ql::datum_t::R_STR) {
            logWRN("Remote server had some kind of problem: %s",
                   result.body.trunc_print().c_str());
        } else if (seen_version != new_version.as_str()) {
            logNTC("New server version available: %s", new_version.trunc_print().c_str());
            ql::datum_t changelog = result.body.get_field("link_changelog", ql::NOTHROW);
            if (changelog.has() && changelog.get_type() == ql::datum_t::R_STR) {
                logNTC("Changelog URL: %s", changelog.trunc_print().c_str());
            }
            seen_version = new_version.as_str();
        } else {
            // already logged an update for that version, so no point
            // in spamming them.
        }
    } else {
        logWRN("Remote server gave a status code I don't understand: %s",
               result.body.trunc_print().c_str());
    }
}
