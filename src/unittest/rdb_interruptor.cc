// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <functional>
#include <stdexcept>

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "unittest/gtest.hpp"
#include "unittest/rdb_env.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

void add_string_arg(Term *term, const std::string &str) {
    Term *arg = term->add_args();
    arg->set_type(Term::DATUM);
    Datum *datum = arg->mutable_datum();
    datum->set_type(Datum::R_STR);
    datum->set_r_str(str);
}

void add_db_arg(Term *term, const std::string &db_name) {
    Term *arg = term->add_args();
    arg->set_type(Term::DB);
    add_string_arg(arg, db_name);
}

void add_table_arg(Term *term, const std::string &db_name, const std::string &table_name) {
    Term *arg = term->add_args();
    arg->set_type(Term::TABLE);
    add_db_arg(arg, db_name);
    add_string_arg(arg, table_name);
}

Term *add_term_arg(Term *term, Term::TermType type) {
    Term *new_term = term->add_args();
    new_term->set_type(type);
    return new_term;
}

Datum *add_object_arg(Term *term) {
    Term *arg = term->add_args();
    arg->set_type(Term::DATUM);
    Datum *datum = arg->mutable_datum();
    datum->set_type(Datum::R_OBJECT);
    return datum;
}

void add_object_str(Datum *datum, const std::string &key, const std::string &value) {
    guarantee(datum->type() == Datum::R_OBJECT);
    Datum_AssocPair *pair = datum->add_r_object();
    pair->set_key(key);
    Datum *pair_value = pair->mutable_val();
    pair_value->set_type(Datum::R_STR);
    pair_value->set_r_str(value);
}

void add_object_bool(Datum *datum, const std::string &key, bool value) {
    guarantee(datum->type() == Datum::R_OBJECT);
    Datum_AssocPair *pair = datum->add_r_object();
    pair->set_key(key);
    Datum *pair_value = pair->mutable_val();
    pair_value->set_type(Datum::R_BOOL);
    pair_value->set_r_bool(value);
}

void add_object_num(Datum *datum, const std::string &key, double value) {
    guarantee(datum->type() == Datum::R_OBJECT);
    Datum_AssocPair *pair = datum->add_r_object();
    pair->set_key(key);
    Datum *pair_value = pair->mutable_val();
    pair_value->set_type(Datum::R_NUM);
    pair_value->set_r_num(value);
}

Datum *add_object_array_item(Datum *datum, const std::string &key, Datum::DatumType item_type) {
    guarantee(datum->type() == Datum::R_OBJECT);
    Datum_AssocPair *pair = datum->add_r_object();
    pair->set_key(key);
    Datum *pair_value = pair->mutable_val();
    pair_value->set_type(item_type);
    return pair_value;
}

Datum *add_object_object(Datum *datum, const std::string &key) {
    guarantee(datum->type() == Datum::R_OBJECT);
    Datum_AssocPair *pair = datum->add_r_object();
    pair->set_key(key);
    Datum *pair_value = pair->mutable_val();
    pair_value->set_type(Datum::R_OBJECT);
    return pair_value;
}

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
    rassert(*count_out > 0);
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
    } catch (const interrupted_exc_t &ex) {
        guarantee(verify_callback->verify(env_instance.get()));
        return;
    }
    guarantee(false);
}

class exists_verify_callback_t : public verify_callback_t {
public:
    exists_verify_callback_t(const database_id_t &_db_id, const std::string &_table_name,
            bool _should_exist, const std::string& _key) :
        key("S" + _key),
        db_id(_db_id),
        should_exist(_should_exist)
    {
        if (!table_name.assign_value(_table_name)) {
            throw invalid_name_exc_t(_table_name);
        }
    }
    virtual ~exists_verify_callback_t() { }

    bool verify(test_rdb_env_t::instance_t *env_instance) {
        const std::map<store_key_t, scoped_cJSON_t *> *data =
            env_instance->get_data(db_id, table_name);
        bool exists = data->find(key) != data->end();
        return should_exist == exists;
    }

private:
    const store_key_t key;
    const database_id_t db_id;
    name_string_t table_name;
    const bool should_exist;
};

TEST(RDBInterrupt, InsertOp) {
    ql::protob_t<Term> insert_proto = ql::make_counted_term();
    uint32_t eval_count;

    insert_proto->set_type(Term::INSERT);
    add_table_arg(insert_proto.get(), "db", "table");

    {
        Datum *object = add_object_arg(insert_proto.get());
        add_object_str(object, "id", "key");
        add_object_str(object, "value", "stuff");
    }

    {
        test_rdb_env_t test_env;
        database_id_t db_id = test_env.add_database("db");
        test_env.add_table("table", db_id, "id",
            std::set<std::map<std::string, std::string> >());
        exists_verify_callback_t verify_callback(db_id, "table", true, "key");
        unittest::run_in_thread_pool(std::bind(count_evals,
                                               &test_env,
                                               insert_proto,
                                               &eval_count,
                                               &verify_callback));
    }
    for (uint64_t i = 0; i <= eval_count; ++i) {
        test_rdb_env_t test_env;
        database_id_t db_id = test_env.add_database("db");
        test_env.add_table("table", db_id, "id",
            std::set<std::map<std::string, std::string> >());
        exists_verify_callback_t verify_callback(db_id, "table", false, "key");
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
    ql::protob_t<Term> get_proto = ql::make_counted_term();
    uint32_t eval_count;
    std::set<std::map<std::string, std::string> > initial_data;

    std::map<std::string, std::string> target_object;
    target_object["id"] = std::string("key");
    target_object["value"] = std::string("stuff");
    initial_data.insert(target_object);

    get_proto->set_type(Term::GET);
    add_table_arg(get_proto.get(), "db", "table");
    add_string_arg(get_proto.get(), "key");

    {
        test_rdb_env_t test_env;
        dummy_callback_t dummy_callback;
        database_id_t db_id = test_env.add_database("db");
        test_env.add_table("table", db_id, "id", initial_data);
        unittest::run_in_thread_pool(std::bind(count_evals,
                                               &test_env,
                                               get_proto,
                                               &eval_count,
                                               &dummy_callback));
    }
    for (uint64_t i = 0; i <= eval_count; ++i) {
        test_rdb_env_t test_env;
        dummy_callback_t dummy_callback;
        database_id_t db_id = test_env.add_database("db");
        test_env.add_table("table", db_id, "id", initial_data);
        unittest::run_in_thread_pool(std::bind(interrupt_test, &test_env, get_proto, i,
                                               &dummy_callback));
    }
}

TEST(RDBInterrupt, DeleteOp) {
    ql::protob_t<Term> delete_proto = ql::make_counted_term();
    uint32_t eval_count;
    std::set<std::map<std::string, std::string> > initial_data;

    std::map<std::string, std::string> target_object;
    target_object["id"] = std::string("key");
    target_object["value"] = std::string("stuff");
    initial_data.insert(target_object);

    delete_proto->set_type(Term::DELETE);
    {
        Term *get_term = add_term_arg(delete_proto.get(), Term::GET);
        add_table_arg(get_term, "db", "table");
        add_string_arg(get_term, "key");
    }

    {
        test_rdb_env_t test_env;
        database_id_t db_id = test_env.add_database("db");
        test_env.add_table("table", db_id, "id", initial_data);
        exists_verify_callback_t verify_callback(db_id, "table", false, "key");
        unittest::run_in_thread_pool(std::bind(count_evals,
                                               &test_env,
                                               delete_proto,
                                               &eval_count,
                                               &verify_callback));
    }
    for (uint64_t i = 0; i <= eval_count; ++i) {
        test_rdb_env_t test_env;
        database_id_t db_id = test_env.add_database("db");
        test_env.add_table("table", db_id, "id", initial_data);
        exists_verify_callback_t verify_callback(db_id, "table", true, "key");
        unittest::run_in_thread_pool(std::bind(interrupt_test,
                                               &test_env,
                                               delete_proto,
                                               i,
                                               &verify_callback));
    }
}

}  // namespace unittest
