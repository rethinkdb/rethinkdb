// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/bind.hpp>

#include "unittest/unittest_utils.hpp"

#include "containers/archive/archive.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"
#include "extproc/js_runner.hpp"
#include "rpc/serialize_macros.hpp"
#include "unittest/gtest.hpp"

void run_timeout_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;

    js_runner.begin(&extproc_pool, NULL);

    const std::string loop_source = "for (var x = 0; x < 4e8; x++) {}";

    js_runner_t::req_config_t config;
    config.timeout_ms = 20;

    ASSERT_THROW(js_runner.eval(loop_source, config), interrupted_exc_t);
    ASSERT_FALSE(js_runner.connected());
}

TEST(JSProc, Timeout) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_timeout_test));
}

void run_literal_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;

    js_runner.begin(&extproc_pool, NULL);

    const std::string source_code = "9467923";

    js_runner_t::req_config_t config;
    config.timeout_ms = 100;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Check results
    boost::shared_ptr<scoped_cJSON_t> *res_data =
        boost::get<boost::shared_ptr<scoped_cJSON_t> >(&result);
    ASSERT_TRUE(res_data != NULL);
    ASSERT_TRUE(res_data->get() != NULL);
    ASSERT_TRUE(res_data->get()->get() != NULL);
    ASSERT_TRUE(res_data->get()->get()->type == cJSON_Number);
    ASSERT_EQ(res_data->get()->get()->valueint, 9467923);
}

TEST(JSProc, Literal) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_literal_test));
}

void run_eval_and_call_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;

    js_runner.begin(&extproc_pool, NULL);

    const std::string source_code = "(function () { return 10337; })";

    js_runner_t::req_config_t config;
    config.timeout_ms = 100;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the id of the function out
    js_id_t *js_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(js_id != NULL);

    // Call the function
    result = js_runner.call(*js_id,
                            std::vector<boost::shared_ptr<scoped_cJSON_t> >(),
                            config);
    ASSERT_TRUE(js_runner.connected());

    // Check results
    boost::shared_ptr<scoped_cJSON_t> *res_data =
        boost::get<boost::shared_ptr<scoped_cJSON_t> >(&result);
    ASSERT_TRUE(res_data != NULL);
    ASSERT_TRUE(res_data->get() != NULL);
    ASSERT_TRUE(res_data->get()->get() != NULL);
    ASSERT_TRUE(res_data->get()->get()->type == cJSON_Number);
    ASSERT_EQ(res_data->get()->get()->valueint, 10337);
}

TEST(JSProc, EvalAndCall) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_eval_and_call_test));
}

void run_broken_function_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;

    js_runner.begin(&extproc_pool, NULL);

    const std::string source_code = "(function () { return 4 / 0; })";

    js_runner_t::req_config_t config;
    config.timeout_ms = 100;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the id of the function out
    js_id_t *js_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(js_id != NULL);

    // Call the function
    result = js_runner.call(*js_id,
                            std::vector<boost::shared_ptr<scoped_cJSON_t> >(),
                            config);
    ASSERT_TRUE(js_runner.connected());

    // Get the error message
    std::string *error = boost::get<std::string>(&result);
    ASSERT_TRUE(error != NULL);
}

TEST(JSProc, BrokenFunction) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_broken_function_test));
}

void run_invalid_function_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;

    js_runner.begin(&extproc_pool, NULL);

    const std::string source_code = "(function() {)";

    js_runner_t::req_config_t config;
    config.timeout_ms = 100;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the error message
    std::string *error = boost::get<std::string>(&result);
    ASSERT_TRUE(error != NULL);
}

TEST(JSProc, InvalidFunction) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_invalid_function_test));
}
