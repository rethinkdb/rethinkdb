// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "containers/archive/archive.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"
#include "extproc/js_runner.hpp"
#include "rpc/serialize_macros.hpp"
#include "unittest/extproc_test.hpp"
#include "unittest/gtest.hpp"
#include "rdb_protocol/env.hpp"

SPAWNER_TEST(JSProc, EvalTimeout) {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;
    ql::configured_limits_t limits;

    js_runner.begin(&extproc_pool, nullptr, limits);

    const std::string loop_source = "for (var x = 0; x < 4e10; x++) {}";

    js_runner_t::req_config_t config;
    config.timeout_ms = 10;

    js_result_t result = js_runner.eval(loop_source, config);
    std::string value = boost::get<std::string>(result);
    ASSERT_EQ(strprintf("JavaScript query `%s` timed out after 0.010 seconds.", loop_source.c_str()), value);
    ASSERT_FALSE(js_runner.connected());
}

SPAWNER_TEST(JSProc, CallTimeout) {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;
    ql::configured_limits_t limits;

    js_runner.begin(&extproc_pool, nullptr, limits);

    const std::string loop_source = "(function () { for (var x = 0; x < 4e10; x++) {}})";

    js_runner_t::req_config_t config;
    config.timeout_ms = 10000;

    js_result_t result = js_runner.eval(loop_source, config);

    js_id_t *any_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(any_id != nullptr);

    config.timeout_ms = 10;

    result = js_runner.call(loop_source, std::vector<ql::datum_t>(), config);
    std::string value = boost::get<std::string>(result);
    ASSERT_EQ(strprintf("JavaScript query `%s` timed out after 0.010 seconds.", loop_source.c_str()), value);
    ASSERT_FALSE(js_runner.connected());
}

void run_datum_test(const std::string &source_code, ql::datum_t *res_out) {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;
    ql::configured_limits_t limits;

    js_runner.begin(&extproc_pool, nullptr, limits);

    js_runner_t::req_config_t config;
    config.timeout_ms = 10000;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    ql::datum_t *res_datum =
        boost::get<ql::datum_t>(&result);
    ASSERT_TRUE(res_datum != nullptr);
    *res_out = *res_datum;
}

SPAWNER_TEST(JSProc, LiteralNumber) {
    ql::datum_t result;
    run_datum_test("9467923", &result);
    ASSERT_TRUE(result.has());
    ASSERT_TRUE(result.get_type() == ql::datum_t::R_NUM);
    ASSERT_EQ(result.as_int(), 9467923);
}

SPAWNER_TEST(JSProc, LiteralString) {
    ql::datum_t result;
    run_datum_test("\"string data\"", &result);
    ASSERT_TRUE(result.has());
    ASSERT_TRUE(result.get_type() == ql::datum_t::R_STR);
    ASSERT_EQ(result.as_str(), "string data");
}

SPAWNER_TEST(JSProc, EvalAndCall) {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;
    ql::configured_limits_t limits;

    js_runner.begin(&extproc_pool, nullptr, limits);

    const std::string source_code = "(function () { return 10337; })";

    js_runner_t::req_config_t config;
    config.timeout_ms = 10000;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the id of the function out
    js_id_t *js_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(js_id != nullptr);

    // Call the function
    result = js_runner.call(source_code,
                            std::vector<ql::datum_t>(),
                            config);
    ASSERT_TRUE(js_runner.connected());

    // Check results
    ql::datum_t *res_datum = boost::get<ql::datum_t>(&result);
    ASSERT_TRUE(res_datum != nullptr);
    ASSERT_TRUE(res_datum->has());
    ASSERT_TRUE(res_datum->get_type() == ql::datum_t::R_NUM);
    ASSERT_EQ(res_datum->as_int(), 10337);
}

SPAWNER_TEST(JSProc, BrokenFunction) {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;
    ql::configured_limits_t limits;

    js_runner.begin(&extproc_pool, nullptr, limits);

    const std::string source_code = "(function () { return 4 / 0; })";

    js_runner_t::req_config_t config;
    config.timeout_ms = 10000;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the id of the function out
    js_id_t *js_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(js_id != nullptr);

    // Call the function
    result = js_runner.call(source_code,
                            std::vector<ql::datum_t>(),
                            config);
    ASSERT_TRUE(js_runner.connected());

    // Get the error message
    std::string *error = boost::get<std::string>(&result);
    ASSERT_TRUE(error != nullptr);
}

SPAWNER_TEST(JSProc, InvalidFunction) {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;
    ql::configured_limits_t limits;

    js_runner.begin(&extproc_pool, nullptr, limits);

    const std::string source_code = "(function() {)";

    js_runner_t::req_config_t config;
    config.timeout_ms = 10000;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the error message
    std::string *error = boost::get<std::string>(&result);
    ASSERT_TRUE(error != nullptr);
}

SPAWNER_TEST(JSProc, InfiniteRecursionFunction) {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;
    ql::configured_limits_t limits;

    js_runner.begin(&extproc_pool, nullptr, limits);

    const std::string source_code = "(function f(x) { x = x + f(x); return x; })";

    js_runner_t::req_config_t config;
    config.timeout_ms = 60000;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the id of the function out
    js_id_t *js_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(js_id != nullptr);

    // Call the function
    std::vector<ql::datum_t> args;
    args.push_back(ql::datum_t(1.0));
    result = js_runner.call(source_code, args, config);

    std::string *err_msg = boost::get<std::string>(&result);

    ASSERT_EQ(*err_msg, std::string("RangeError: Maximum call stack size exceeded"));
}

void run_overalloc_function_test() {
    extproc_pool_t extproc_pool(1);
    js_runner_t js_runner;
    ql::configured_limits_t limits;

    js_runner.begin(&extproc_pool, nullptr, limits);

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
    ASSERT_TRUE(js_id != nullptr);

    // Call the function
    ASSERT_THROW(js_runner.call(source_code,
                                std::vector<ql::datum_t>(),
                                config), extproc_worker_exc_t);
}

// Disabling this test because it may cause complications depending on the user's system
// ^^^ WHAT COMPLICATIONS???
/*
TEST(JSProc, OverallocFunction) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(run_overalloc_function_test);
}
*/

void passthrough_test_internal(extproc_pool_t *pool, const ql::datum_t &arg) {
    guarantee(arg.has());

    ql::configured_limits_t limits;
    js_runner_t js_runner;
    js_runner.begin(pool, nullptr, limits);

    const std::string source_code = "(function f(arg) { return arg; })";

    js_runner_t::req_config_t config;
    config.timeout_ms = 60000;
    js_result_t result = js_runner.eval(source_code, config);
    ASSERT_TRUE(js_runner.connected());

    // Get the id of the function out
    js_id_t *js_id = boost::get<js_id_t>(&result);
    ASSERT_TRUE(js_id != nullptr);

    // Call the function
    js_result_t res = js_runner.call(source_code,
                                     std::vector<ql::datum_t>(1, arg),
                                     config);

    ql::datum_t *res_datum = boost::get<ql::datum_t>(&res);
    ASSERT_TRUE(res_datum != nullptr);
    ASSERT_TRUE(res_datum->has());
    ASSERT_EQ(*res_datum, arg);
}

// This test will make sure that conversion of datum_t to and from v8 types works
// correctly
SPAWNER_TEST(JSProc, Passthrough) {
    extproc_pool_t pool(1);
    ql::configured_limits_t limits;

    // Number
    passthrough_test_internal(&pool, ql::datum_t(99.9999));
    passthrough_test_internal(&pool, ql::datum_t(99.9999));

    // String
    passthrough_test_internal(&pool, ql::datum_t(""));
    passthrough_test_internal(&pool, ql::datum_t("string str"));
    passthrough_test_internal(&pool, ql::datum_t(datum_string_t()));
    passthrough_test_internal(&pool, ql::datum_t(datum_string_t("string str")));

    // Boolean
    passthrough_test_internal(&pool, ql::datum_t::boolean(true));
    passthrough_test_internal(&pool, ql::datum_t::boolean(false));

    // Array
    ql::datum_t array_datum;
    {
        std::vector<ql::datum_t> array_data;
        array_datum = ql::datum_t(std::move(array_data), limits);
        passthrough_test_internal(&pool, array_datum);

        for (size_t i = 0; i < 100; ++i) {
            array_data.push_back(
                ql::datum_t(datum_string_t(std::string(i, 'a'))));
            std::vector<ql::datum_t> copied_data(array_data);
            array_datum = ql::datum_t(std::move(copied_data), limits);
            passthrough_test_internal(&pool, array_datum);
        }
    }


    // Object
    ql::datum_t object_datum;
    {
        std::map<datum_string_t, ql::datum_t> object_data;
        object_datum = ql::datum_t(std::move(object_data));
        passthrough_test_internal(&pool, array_datum);

        for (size_t i = 0; i < 100; ++i) {
            object_data.insert(std::make_pair(datum_string_t(std::string(i, 'a')),
                                              ql::datum_t(static_cast<double>(i))));
            std::map<datum_string_t, ql::datum_t> copied_data(object_data);
            object_datum = ql::datum_t(std::move(copied_data));
            passthrough_test_internal(&pool, array_datum);
        }
    }

    // Nested structure
    ql::datum_t nested_datum;
    {
        std::vector<ql::datum_t> nested_data;
        nested_datum = ql::datum_t(std::move(nested_data), limits);
        passthrough_test_internal(&pool, nested_datum);

        nested_data.push_back(array_datum);
        std::vector<ql::datum_t> copied_data(nested_data);
        nested_datum = ql::datum_t(std::move(copied_data), limits);
        passthrough_test_internal(&pool, nested_datum);

        nested_data.push_back(object_datum);
        copied_data = nested_data;
        nested_datum = ql::datum_t(std::move(copied_data), limits);
        passthrough_test_internal(&pool, nested_datum);
    }
}
