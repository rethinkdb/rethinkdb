// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/bind.hpp>
#include "unittest/unittest_utils.hpp"
#include "unittest/gtest.hpp"
#include "unittest/rdb_env.hpp"
#include <stdexcept>

namespace unittest {

class bagged_t : public ptr_baggable_t {
public:
    bagged_t(int &_alloc_tracker) :
        alloc_tracker(_alloc_tracker) {
        ++alloc_tracker;
    }

    ~bagged_t() {
        --alloc_tracker;
    }

private:
    int &alloc_tracker;
};

void add_ptrs(ql::env_t *env, int count, int &alloc_tracker) {
    int old_allocs = alloc_tracker;
    for (int i = 0; i < count; ++i) {
        env->add_ptr(new bagged_t(alloc_tracker));
    }
    ASSERT_EQ(alloc_tracker, old_allocs + count);
}

class test_datum_t : public ql::datum_t {
public:
    template <class type_a>
    test_datum_t(int *_alloc_tracker, type_a param_a) :
        datum_t(param_a),
        alloc_tracker(_alloc_tracker) {
        ++(*alloc_tracker);
    }

    template <class type_a, class type_b>
    test_datum_t(int *_alloc_tracker, type_a param_a, type_b param_b) :
        datum_t(param_a, param_b),
        alloc_tracker(_alloc_tracker) {
        ++(*alloc_tracker);
    }

    virtual ~test_datum_t() {
        --(*alloc_tracker);
    }

private:
    int *alloc_tracker;
};

// Spew a bunch of random data into an object or two
ql::datum_t *construct_random_datum(int *alloc_tracker, ql::env_t *env, size_t level = 0) {
    // TODO: this is kind of fragile
    ql::datum_t::type_t type_to_construct = static_cast<ql::datum_t::type_t>(randint(6) + 1);
    ql::datum_t *result;

    // Don't generate datums more than three levels deep
    if (level == 3 && (type_to_construct == ql::datum_t::R_ARRAY || type_to_construct == ql::datum_t::R_OBJECT))
        type_to_construct = ql::datum_t::R_NUM;

    if (type_to_construct == ql::datum_t::R_NULL) {
        result = new test_datum_t(alloc_tracker, type_to_construct);
    } else if (type_to_construct == ql::datum_t::R_BOOL) {
        result = new test_datum_t(alloc_tracker, type_to_construct, randint(2) == 1);
    } else if (type_to_construct == ql::datum_t::R_NUM) {
        result = new test_datum_t(alloc_tracker, static_cast<double>(randint(INT_MAX)));
    } else if (type_to_construct == ql::datum_t::R_STR) {
        result = new test_datum_t(alloc_tracker, rand_string(randint(30)));
    } else if (type_to_construct == ql::datum_t::R_ARRAY) {
        result = new test_datum_t(alloc_tracker, type_to_construct);
        size_t num_items = randint(30);
        for (size_t i = 0; i < num_items; ++i) {
            result->add(construct_random_datum(alloc_tracker, env, level + 1));
        }
    } else if (type_to_construct == ql::datum_t::R_OBJECT) {
        result = new test_datum_t(alloc_tracker, type_to_construct);
        size_t num_items = randint(30);
        for (size_t i = 0; i < num_items; ++i) {
            ql::datum_t *new_datum = construct_random_datum(alloc_tracker, env, level + 1);
            if (!result->add(rand_string(randint(30)), new_datum)) {
                // Do nothing?  the datum will be cleaned up later, and this shouldn't cause other problems
            }
        }
    } else {
        throw std::runtime_error("unexpected type_t value");
    }

    env->add_ptr(result);
    return result;
}

void add_data(int *alloc_tracker, ql::env_t *env, size_t base_count) {
    for (size_t i = 0; i < base_count; ++i) {
        construct_random_datum(alloc_tracker, env);
    }
}

void checkpoint_discard_test(test_rdb_env_t *test_env) {
    int alloc_tracker(0);
    scoped_ptr_t<test_rdb_env_t::instance_t> env_instance;
    test_env->make_env(&env_instance);

    add_ptrs(env_instance->get(), 5, alloc_tracker);
    ASSERT_TRUE(env_instance->get()->num_checkpoints() == 0);
    {
        ql::env_checkpoint_t checkpoint1(env_instance->get(), &ql::env_t::discard_checkpoint);
        ASSERT_TRUE(env_instance->get()->num_checkpoints() == 1);
        add_ptrs(env_instance->get(), 5, alloc_tracker);
        ql::env_checkpoint_t checkpoint2(env_instance->get(), &ql::env_t::discard_checkpoint);
        ASSERT_TRUE(env_instance->get()->num_checkpoints() == 2);
        add_ptrs(env_instance->get(), 5, alloc_tracker);
    }
    ASSERT_TRUE(env_instance->get()->num_checkpoints() == 0);
    ASSERT_EQ(alloc_tracker, 5);

    int keep_allocs = alloc_tracker;
    ql::datum_t *keep = construct_random_datum(&alloc_tracker, env_instance->get());
    keep_allocs = alloc_tracker - keep_allocs;
    {
        ql::env_checkpoint_t checkpoint1(env_instance->get(), &ql::env_t::discard_checkpoint);
        ASSERT_TRUE(env_instance->get()->num_checkpoints() == 1);
        add_data(&alloc_tracker, env_instance->get(), 5);
        env_instance->get()->add_ptr(keep);
        add_data(&alloc_tracker, env_instance->get(), 2);
        ASSERT_GT(alloc_tracker, 11 + keep_allocs);
        ASSERT_TRUE(env_instance->get()->num_checkpoints() == 1);
        checkpoint1.gc(keep);
        ASSERT_TRUE(env_instance->get()->num_checkpoints() == 1);
        ASSERT_EQ(alloc_tracker, 5 + keep_allocs);
    }
}

TEST(Checkpoint, CheckpointDiscard) {
    test_rdb_env_t test_env;
    unittest::run_in_thread_pool(boost::bind(checkpoint_discard_test, &test_env));
}

void checkpoint_do_stuff(bool *is_merge, int *alloc_tracker, int *allocs_delta, ql::env_t *env, ql::env_checkpoint_t *checkpoint) {
    // Do nothing half the time
    if (randint(2) == 0) {
        int changed_checkpoint_type = randint(3);
        int new_data_before = randint(5);
        int new_data_after = randint(5);
        int old_allocs = *alloc_tracker;

        add_data(alloc_tracker, env, new_data_before);

        if (changed_checkpoint_type == 0) {
            checkpoint->reset(&ql::env_t::discard_checkpoint);
            *is_merge = false;
        } else if (changed_checkpoint_type == 1) {
            checkpoint->reset(&ql::env_t::merge_checkpoint);
            *is_merge = true;
        }

        add_data(alloc_tracker, env, new_data_after);
        *allocs_delta += *alloc_tracker - old_allocs;
    }
}

void checkpoint_all_test(test_rdb_env_t *test_env) {
    int alloc_tracker = 0;
    {
        scoped_ptr_t<test_rdb_env_t::instance_t> env_instance;
        test_env->make_env(&env_instance);
        int expected_allocs = 0;

        ASSERT_TRUE(env_instance->get()->num_checkpoints() == 0);

        for (size_t i = 0; i < 5; ++i) {
            int checkpoint_data = 0;
            int checkpoint_type = randint(2);
            ql::env_checkpoint_t checkpoint(env_instance->get(), checkpoint_type == 0 ?
                                            &ql::env_t::discard_checkpoint : &ql::env_t::merge_checkpoint);
            bool checkpoint_merge = (checkpoint_type != 0);

            ASSERT_EQ(alloc_tracker, expected_allocs);
            checkpoint_do_stuff(&checkpoint_merge, &alloc_tracker, &checkpoint_data, env_instance->get(), &checkpoint);

            for (size_t i = 0; i < 3; ++i) {
                int subcheckpoint_type = randint(2);
                int subcheckpoint_data = 0;
                ql::env_checkpoint_t subcheckpoint(env_instance->get(), subcheckpoint_type == 0 ?
                                                   &ql::env_t::discard_checkpoint : &ql::env_t::merge_checkpoint);
                bool subcheckpoint_merge = (subcheckpoint_type != 0);
                checkpoint_do_stuff(&subcheckpoint_merge, &alloc_tracker, &subcheckpoint_data, env_instance->get(), &subcheckpoint);

                if (subcheckpoint_merge) {
                    checkpoint_data += subcheckpoint_data;
                }
            }

            ASSERT_EQ(alloc_tracker, expected_allocs + checkpoint_data);
            checkpoint_do_stuff(&checkpoint_merge, &alloc_tracker, &checkpoint_data, env_instance->get(), &checkpoint);

            if (checkpoint_merge) {
                expected_allocs += checkpoint_data;
            }
        }

        ASSERT_EQ(alloc_tracker, expected_allocs);
    }
    ASSERT_EQ(alloc_tracker, 0);
}

TEST(Checkpoint, CheckpointAll) {
    for (size_t i = 0; i < 50; ++i) {
        test_rdb_env_t test_env;
        unittest::run_in_thread_pool(boost::bind(checkpoint_all_test, &test_env));
    }
}

}   /* namespace unittest */

