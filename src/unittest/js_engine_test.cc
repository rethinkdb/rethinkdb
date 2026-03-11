// Copyright 2025 RethinkDB, all rights reserved.
// Unit tests for the pluggable JavaScript engine architecture

#include "extproc/js_engine.hpp"
#include "unittest/gtest.hpp"
#include "rdb_protocol/datum.hpp"

namespace unittest {

// Test that engine types can be parsed correctly
TEST(JSEngine, ParseEngineNames) {
    using namespace rethinkdb::js;
    
    EXPECT_EQ(js_engine_type_t::V8_JITLESS, parse_js_engine_name("v8-jitless"));
    EXPECT_EQ(js_engine_type_t::V8_JITLESS, parse_js_engine_name("v8jitless"));
    EXPECT_EQ(js_engine_type_t::V8_FULL, parse_js_engine_name("v8"));
    EXPECT_EQ(js_engine_type_t::QUICKJS, parse_js_engine_name("quickjs"));
    EXPECT_EQ(js_engine_type_t::QUICKJS_NG, parse_js_engine_name("quickjs-ng"));
    EXPECT_EQ(js_engine_type_t::DUKTAPE, parse_js_engine_name("duktape"));
    EXPECT_EQ(js_engine_type_t::HERMES, parse_js_engine_name("hermes"));
    
    // Invalid engine names should throw
    EXPECT_THROW(parse_js_engine_name("invalid"), std::invalid_argument);
    EXPECT_THROW(parse_js_engine_name(""), std::invalid_argument);
}

// Test that engine names can be converted back to strings
TEST(JSEngine, GetEngineNames) {
    using namespace rethinkdb::js;
    
    EXPECT_EQ("v8-jitless", get_js_engine_name(js_engine_type_t::V8_JITLESS));
    EXPECT_EQ("v8", get_js_engine_name(js_engine_type_t::V8_FULL));
    EXPECT_EQ("quickjs", get_js_engine_name(js_engine_type_t::QUICKJS));
    EXPECT_EQ("quickjs-ng", get_js_engine_name(js_engine_type_t::QUICKJS_NG));
    EXPECT_EQ("duktape", get_js_engine_name(js_engine_type_t::DUKTAPE));
    EXPECT_EQ("hermes", get_js_engine_name(js_engine_type_t::HERMES));
}

// Test that the default engine is valid
TEST(JSEngine, DefaultEngine) {
    using namespace rethinkdb::js;
    
    js_engine_type_t default_engine = get_default_js_engine();
    // Default should be one of the valid engines
    EXPECT_TRUE(default_engine == js_engine_type_t::QUICKJS ||
                default_engine == js_engine_type_t::V8_JITLESS);
}

// Test datum conversion helpers (if available)
TEST(JSEngine, DatumTypes) {
    // Test that datum types work correctly with JS engines
    ql::datum_t null_datum = ql::datum_t::null();
    EXPECT_TRUE(null_datum.has());
    EXPECT_EQ(ql::datum_t::R_NULL, null_datum.get_type());
    
    ql::datum_t bool_datum = ql::datum_t::boolean(true);
    EXPECT_TRUE(bool_datum.has());
    EXPECT_EQ(ql::datum_t::R_BOOL, bool_datum.get_type());
    EXPECT_TRUE(bool_datum.as_bool());
    
    ql::datum_t num_datum = ql::datum_t(42.0);
    EXPECT_TRUE(num_datum.has());
    EXPECT_EQ(ql::datum_t::R_NUM, num_datum.get_type());
    EXPECT_EQ(42.0, num_datum.as_num());
    
    ql::datum_t str_datum = ql::datum_t(datum_string_t("test"));
    EXPECT_TRUE(str_datum.has());
    EXPECT_EQ(ql::datum_t::R_STR, str_datum.get_type());
    EXPECT_EQ("test", str_datum.as_str().to_std());
}

// Test that uninitialized datum comparison works correctly
TEST(JSEngine, UninitializedDatumComparison) {
    ql::datum_t uninitialized1;
    ql::datum_t uninitialized2;
    ql::datum_t null_datum = ql::datum_t::null();
    
    // Two uninitialized datums should be equal
    EXPECT_TRUE(uninitialized1 == uninitialized2);
    EXPECT_FALSE(uninitialized1 != uninitialized2);
    
    // Uninitialized should not equal null
    EXPECT_FALSE(uninitialized1 == null_datum);
    EXPECT_TRUE(uninitialized1 != null_datum);
    
    // Uninitialized should be less than initialized
    EXPECT_TRUE(uninitialized1 < null_datum);
    EXPECT_FALSE(null_datum < uninitialized1);
}

}  // namespace unittest
