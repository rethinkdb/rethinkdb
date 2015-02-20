// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <functional>
#include <stdexcept>

#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "unittest/gtest.hpp"
#include "unittest/rdb_env.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

class count_callback_t : public ql::env_t::eval_callback_t {
public:
    explicit count_callback_t(uint32_t *_count_out) : count_out(_count_out) {
        *count_out = 0;
    }

    virtual ~count_callback_t() { }

    void eval_callback() {
        ++(*count_out);
    }

private:
    uint32_t *count_out;
};

class interrupt_callback_t : public ql::env_t::eval_callback_t {
public:
    interrupt_callback_t(uint32_t _delay, test_rdb_env_t::instance_t *_test_env_instance) :
        delay(_delay),
        test_env_instance(_test_env_instance)
    {
        if (delay == 0) {
            test_env_instance->interrupt();
        }
    }

    virtual ~interrupt_callback_t() { }

    void eval_callback() {
        --delay;
        if (delay == 0) {
            test_env_instance->interrupt();
        }
    }

private:
    uint32_t delay;
    test_rdb_env_t::instance_t *test_env_instance;
};

class verify_callback_t {
public:
    virtual ~verify_callback_t() { }
    virtual bool verify(test_rdb_env_t::instance_t *) = 0;
};

void count_evals(test_rdb_env_t *test_env, ql::protob_t<const Term> term, uint32_t *count_out,
                 verify_callback_t *verify_callback) {
    scoped_ptr_t<test_rdb_env_t::instance_t> env_instance = test_env->make_env();

    count_callback_t callback(count_out);
    env_instance->get()->set_eval_callback(&callback);

    ql::compile_env_t compile_env((ql::var_visibility_t()));
    counted_t<const ql::term_t> compiled_term = ql::compile_term(&compile_env, term);

    ql::scope_env_t scope_env(env_instance->get(), ql::var_scope_t());
    UNUSED scoped_ptr_t<ql::val_t> result = compiled_term->eval(&scope_env);
    guarantee(*count_out > 0);
    guarantee(verify_callback->verify(env_instance.get()));
}

void interrupt_test(test_rdb_env_t *test_env,
                    ql::protob_t<const Term> term,
                    uint32_t interrupt_phase,
                    verify_callback_t *verify_callback) {
    scoped_ptr_t<test_rdb_env_t::instance_t> env_instance = test_env->make_env();

    interrupt_callback_t callback(interrupt_phase, env_instance.get());
    env_instance->get()->set_eval_callback(&callback);

    ql::compile_env_t compile_env((ql::var_visibility_t()));
    counted_t<const ql::term_t> compiled_term = ql::compile_term(&compile_env, term);

    try {
        ql::scope_env_t scope_env(env_instance->get(), ql::var_scope_t());
        UNUSED scoped_ptr_t<ql::val_t> result = compiled_term->eval(&scope_env);
        guarantee(false);
    } catch (const interrupted_exc_t &ex) {
        // Do nothing
    }
    guarantee(verify_callback->verify(env_instance.get()));
}

class exists_verify_callback_t : public verify_callback_t {
public:
    exists_verify_callback_t(const std::string &_db_name,
                             const std::string &_table_name,
                             const ql::datum_t &_key,
                             bool _should_exist) :
        key(_key.print_primary()),
        db_name(name_string_t::guarantee_valid(_db_name.c_str())),
        table_name(name_string_t::guarantee_valid(_table_name.c_str())),
        should_exist(_should_exist) { }

    virtual ~exists_verify_callback_t() { }

    bool verify(test_rdb_env_t::instance_t *env_instance) {
        const std::map<store_key_t, ql::datum_t> *data =
            env_instance->get_data(db_name, table_name);
        bool exists = data->find(key) != data->end();
        return should_exist == exists;
    }

private:
    const store_key_t key;
    const name_string_t db_name;
    const name_string_t table_name;
    const bool should_exist;
};

TEST(RDBInterrupt, InsertOp) {
    uint32_t eval_count;

    ql::datum_object_builder_t row;
    row.overwrite("id", ql::datum_t(datum_string_t("key")));
    row.overwrite("value", ql::datum_t(datum_string_t("stuff")));

    ql::protob_t<const Term> insert_proto =
        ql::r::db("db").table("table").insert(std::move(row).to_datum()).release_counted();

    {
        test_rdb_env_t test_env;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id");
        exists_verify_callback_t verify_callback(
            "db", "table", ql::datum_t(datum_string_t("key")), true);
        unittest::run_in_thread_pool(std::bind(count_evals,
                                               &test_env,
                                               insert_proto,
                                               &eval_count,
                                               &verify_callback));
    }
    for (uint64_t i = 0; i <= eval_count; ++i) {
        test_rdb_env_t test_env;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id");
        exists_verify_callback_t verify_callback(
            "db", "table", ql::datum_t(datum_string_t("key")), false);
        unittest::run_in_thread_pool(std::bind(interrupt_test,
                                               &test_env,
                                               insert_proto,
                                               i,
                                               &verify_callback));
    }
}

class dummy_callback_t : public verify_callback_t {
public:
    virtual ~dummy_callback_t() { }
    bool verify(UNUSED test_rdb_env_t::instance_t *env_instance) {
        return true;
    }
};

TEST(RDBInterrupt, GetOp) {
    uint32_t eval_count;
    std::set<ql::datum_t, latest_version_optional_datum_less_t> initial_data;

    ql::datum_object_builder_t row;
    row.overwrite("id", ql::datum_t(datum_string_t("key")));
    row.overwrite("value", ql::datum_t(datum_string_t("stuff")));
    initial_data.insert(std::move(row).to_datum());

    ql::protob_t<const Term> get_proto =
        ql::r::db("db").table("table").get_("key").release_counted();

    {
        test_rdb_env_t test_env;
        dummy_callback_t dummy_callback;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id", initial_data);
        unittest::run_in_thread_pool(std::bind(count_evals,
                                               &test_env,
                                               get_proto,
                                               &eval_count,
                                               &dummy_callback));
    }
    for (uint64_t i = 0; i <= eval_count; ++i) {
        test_rdb_env_t test_env;
        dummy_callback_t dummy_callback;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id", initial_data);
        unittest::run_in_thread_pool(std::bind(interrupt_test, &test_env, get_proto, i,
                                               &dummy_callback));
    }
}

TEST(RDBInterrupt, DeleteOp) {
    uint32_t eval_count;
    std::set<ql::datum_t, latest_version_optional_datum_less_t> initial_data;

    ql::datum_object_builder_t row;
    row.overwrite("id", ql::datum_t(datum_string_t("key")));
    row.overwrite("value", ql::datum_t(datum_string_t("stuff")));
    initial_data.insert(std::move(row).to_datum());

    ql::protob_t<const Term> delete_proto =
        ql::r::db("db").table("table").get_("key").delete_().release_counted();

    {
        test_rdb_env_t test_env;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id", initial_data);
        exists_verify_callback_t verify_callback(
            "db", "table", ql::datum_t(datum_string_t("key")), false);
        unittest::run_in_thread_pool(std::bind(count_evals,
                                               &test_env,
                                               delete_proto,
                                               &eval_count,
                                               &verify_callback));
    }
    for (uint64_t i = 0; i <= eval_count; ++i) {
        test_rdb_env_t test_env;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id", initial_data);
        exists_verify_callback_t verify_callback(
            "db", "table", ql::datum_t(datum_string_t("key")), true);
        unittest::run_in_thread_pool(std::bind(interrupt_test,
                                               &test_env,
                                               delete_proto,
                                               i,
                                               &verify_callback));
    }
}

}  // namespace unittest
