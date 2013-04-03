// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/ptr_bag.hpp"
#include "unittest/gtest.hpp"

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

TEST(PointerBag, PointerBagAddDelete) {
    int allocated(0);

    {
        // Test one allocation
        ptr_bag_t *bag = new ptr_bag_t();
        bag->add(new bagged_t(allocated));
        ASSERT_EQ(allocated, 1);
        ASSERT_EQ(allocated * sizeof(bagged_t) * ptr_bag_t::mem_estimate_multiplier, bag->get_mem_estimate());
        delete bag;
    }
    ASSERT_EQ(allocated, 0);

    {
        // Test a hundred allocations
        ptr_bag_t *bag = new ptr_bag_t();
        for (int i = 0; i < 100; ++i) {
            ASSERT_EQ(i, allocated);
            bag->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated * sizeof(bagged_t) * ptr_bag_t::mem_estimate_multiplier, bag->get_mem_estimate());
        delete bag;
    }
    ASSERT_EQ(allocated, 0);
}

void init_bag_array(ptr_bag_t **array, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        array[i] = new ptr_bag_t();
    }
}

TEST(PointerBag, PointerBagMultiAddDelete) {
    int allocated(0);

    {
        // Set up pointer bags in a heirarchy as such:
        // With initial allocations in 1 and 2
        //  0
        // 1 2
        //    3
        ptr_bag_t *bags[4];
        init_bag_array(bags, 4);
        for (int i = 0; i < 3; ++i) {
            bags[1]->add(new bagged_t(allocated));
            bags[2]->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated, 6);
        bags[0]->add(bags[1]);
        bags[0]->add(bags[2]);
        bags[2]->add(bags[3]);
        ASSERT_EQ(allocated, 6);
        for (int i = 0; i < 40; ++i) {
            bags[i % 4]->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated, 46);
        delete bags[0];
    }
    ASSERT_EQ(allocated, 0);

    {
        // Set up pointer bags in a heirarchy as such:
        // With initial allocations in 0 and 3
        //  0
        // 1 2
        //    3
        ptr_bag_t *bags[4];
        init_bag_array(bags, 4);
        for (int i = 0; i < 3; ++i) {
            bags[0]->add(new bagged_t(allocated));
            bags[3]->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated, 6);
        bags[0]->add(bags[1]);
        bags[0]->add(bags[2]);
        bags[2]->add(bags[3]);
        ASSERT_EQ(allocated, 6);
        for (int i = 0; i < 40; ++i) {
            bags[i % 4]->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated, 46);
        delete bags[0];
    }
    ASSERT_EQ(allocated, 0);

    {
        // Set up pointer bags in a heirarchy as such:
        // With initial allocations in 1, 2, 4, and 6
        //  0         4
        // 1 2       5
        //    3     6 7
        ptr_bag_t *bags[8];
        init_bag_array(bags, 8);
        for (int i = 0; i < 3; ++i) {
            bags[1]->add(new bagged_t(allocated));
            bags[2]->add(new bagged_t(allocated));
            bags[4]->add(new bagged_t(allocated));
            bags[6]->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated, 12);
        bags[2]->add(bags[3]);
        bags[0]->add(bags[1]);
        bags[0]->add(bags[2]);
        bags[4]->add(bags[5]);
        bags[5]->add(bags[6]);
        bags[5]->add(bags[7]);
        ASSERT_EQ(allocated, 12);
        for (int i = 0; i < 80; ++i) {
            bags[i % 8]->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated, 92);

        // Join the pointer bags into the following hierarchy:
        //     0
        //    1 2
        //   4   3
        //  5
        // 6 7
        bags[1]->add(bags[4]);
        ASSERT_EQ(allocated, 92);
        for (int i = 0; i < 80; ++i) {
            bags[i % 8]->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated, 172);
        delete bags[0];
    }
    ASSERT_EQ(allocated, 0);

    {
        // Set up pointer bags in a heirarchy as such:
        // With initial allocations in 0, 3, 5, and 7
        //  0         4
        // 1 2       5
        //    3     6 7
        ptr_bag_t *bags[8];
        init_bag_array(bags, 8);
        for (int i = 0; i < 3; ++i) {
            bags[0]->add(new bagged_t(allocated));
            bags[3]->add(new bagged_t(allocated));
            bags[5]->add(new bagged_t(allocated));
            bags[7]->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated, 12);
        bags[2]->add(bags[3]);
        bags[0]->add(bags[1]);
        bags[0]->add(bags[2]);
        bags[4]->add(bags[5]);
        bags[5]->add(bags[6]);
        bags[5]->add(bags[7]);
        ASSERT_EQ(allocated, 12);
        for (int i = 0; i < 80; ++i) {
            bags[i % 8]->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated, 92);

        // Join the pointer bags into the following hierarchy:
        //     0
        //    1 2
        //   4   3
        //  5
        // 6 7
        bags[1]->add(bags[4]);
        ASSERT_EQ(allocated, 92);
        for (int i = 0; i < 80; ++i) {
            bags[i % 8]->add(new bagged_t(allocated));
        }
        ASSERT_EQ(allocated, 172);
        delete bags[0];
    }
    ASSERT_EQ(allocated, 0);
}

TEST(PointerBag, PointerBagYield) {
    int allocated(0);
    ptr_bag_t *bags[3];
    init_bag_array(bags, 3);
    bagged_t *baggeds[3];
    baggeds[0] = new bagged_t(allocated);
    baggeds[1] = new bagged_t(allocated);
    baggeds[2] = new bagged_t(allocated);
    ASSERT_EQ(allocated, 3);

    // Set up the initial state
    bags[0]->add(baggeds[0]);
    bags[1]->add(baggeds[1]);
    bags[1]->add(baggeds[2]);

    // Move some baggables around
    bags[0]->yield_to(bags[1], baggeds[0]);
    bags[1]->yield_to(bags[0], baggeds[0]);
    bags[1]->yield_to(bags[0], baggeds[1]);
    bags[0]->yield_to(bags[1], baggeds[0]);
    bags[1]->yield_to(bags[0], baggeds[2]);

    delete bags[0];
    delete bags[1];
    delete bags[2];
    ASSERT_EQ(allocated, 0);
}

}   /* namespace unittest */

