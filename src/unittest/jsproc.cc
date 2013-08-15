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

void run_eval_timeout_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;

    js_runner.begin(&extproc_pool, NULL);

    const std::string loop_source = "for (var x = 0; x < 4e10; x++) {}";

    js_runner_t::req_config_t config;
    config.timeout_ms = 10;

    ASSERT_THROW(js_runner.eval(loop_source, config), interrupted_exc_t);
    ASSERT_FALSE(js_runner.connected());
}

TEST(JSProc, EvalTimeout) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_eval_timeout_test));
}

void run_call_timeout_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;

    js_runner.begin(&extproc_pool, NULL);

    const std::string loop_source = "(function () { for (var x = 0; x < 4e10; x++) {} })";

    js_runner_t::req_config_t config;
    config.timeout_ms = 10000;

    js_result_t result = js_runner.eval(loop_source, config);

    js_id_t *any_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(any_id != NULL);

    config.timeout_ms = 10;

    ASSERT_THROW(js_runner.call(loop_source,
                                std::vector<boost::shared_ptr<scoped_cJSON_t> >(),
                                config), interrupted_exc_t);
    ASSERT_FALSE(js_runner.connected());
}

TEST(JSProc, CallTimeout) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_call_timeout_test));
}

void run_literal_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;

    js_runner.begin(&extproc_pool, NULL);

    const std::string source_code = "9467923";

    js_runner_t::req_config_t config;
    config.timeout_ms = 10000;
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
    config.timeout_ms = 10000;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the id of the function out
    js_id_t *js_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(js_id != NULL);

    // Call the function
    result = js_runner.call(source_code,
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
    config.timeout_ms = 10000;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the id of the function out
    js_id_t *js_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(js_id != NULL);

    // Call the function
    result = js_runner.call(source_code,
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
    config.timeout_ms = 10000;
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

void run_infinite_recursion_function_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;

    js_runner.begin(&extproc_pool, NULL);

    const std::string source_code = "(function f(x) { x = x + f(x); return x; })";

    js_runner_t::req_config_t config;
    config.timeout_ms = 60000;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the id of the function out
    js_id_t *js_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(js_id != NULL);

    // Call the function
    std::vector<boost::shared_ptr<scoped_cJSON_t> > args;
    args.push_back(boost::make_shared<scoped_cJSON_t>(cJSON_CreateNumber(1.0)));
    result = js_runner.call(source_code, args, config);

    std::string *err_msg = boost::get<std::string>(&result);

    ASSERT_EQ(*err_msg, std::string("RangeError: Maximum call stack size exceeded"));
}

TEST(JSProc, InfiniteRecursionFunction) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_infinite_recursion_function_test));
}

void run_overalloc_function_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;

    js_runner.begin(&extproc_pool, NULL);

    const std::string source_code = "(function f() {"
                                     "  var res = \"\";"
                                     "  while (true) {"
                                     "    res = res + \"blah\";"
                                     "  }"
                                     "  return res;"
                                     "})";

    js_runner_t::req_config_t config;
    config.timeout_ms = 60000;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the id of the function out
    js_id_t *js_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(js_id != NULL);

    // Call the function
    ASSERT_THROW(js_runner.call(source_code,
                                std::vector<boost::shared_ptr<scoped_cJSON_t> >(),
                                config), js_worker_exc_t);
}

// Disabling this test because it may cause complications depending on the user's system
/*
TEST(JSProc, OverallocFunction) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_overalloc_function_test));
}
*/
