// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/bind.hpp>

#include "unittest/unittest_utils.hpp"

#include "containers/archive/archive.hpp"
#include "extproc/job.hpp"
#include "extproc/pool.hpp"
#include "rdb_protocol/js.hpp"
#include "rpc/serialize_macros.hpp"
#include "unittest/gtest.hpp"

typedef void (*test_t)(js::runner_t *runner);

static void run_jsproc_test(extproc::spawner_t::info_t *spawner_info, test_t func) {
    extproc::pool_group_t pool_group(spawner_info, extproc::pool_group_t::DEFAULTS);
    js::runner_t runner;
    runner.begin(pool_group.get());
    func(&runner);
    if (runner.connected()) {
        runner.finish();
    }
}

static void main_jsproc_test(test_t func) {
    extproc::spawner_t::info_t spawner_info;
    extproc::spawner_t::create(&spawner_info);
    unittest::run_in_thread_pool(boost::bind(run_jsproc_test, &spawner_info, func));
}

// TODO: should have some way to make sure that we finish a test within a given
// time-out, to avoid infinite loops on bugs.

// ----- Tests
void run_timeout_test(js::runner_t *runner) {
    // TODO(rntz): there must be a better way to do this.
    const std::string longloop = "for (var x = 0; x < 4e8; x++) {}";

    std::string errmsg;
    id_t id = runner->compile(std::vector<std::string>(), longloop, &errmsg);
    ASSERT_NE(js::INVALID_ID, id);

    // Now, the timeout test.
    js::runner_t::req_config_t config;
    config.timeout_ms = 20;

    try {
        runner->call(id,
                     boost::shared_ptr<scoped_cJSON_t>(),
                     std::vector<boost::shared_ptr<scoped_cJSON_t> >(),
                     &errmsg,
                     &config);
        FAIL() << "didn't time out";
    } catch (interrupted_exc_t) {}

    // Interruption should have quit the job.
    ASSERT_FALSE(runner->connected());
}

TEST(JSProc, Timeout) { main_jsproc_test(run_timeout_test); }
