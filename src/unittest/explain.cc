// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "unittest/gtest.hpp"

#include "arch/timing.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/explain.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

void approx_eq(double val1, double val2, double error) {
    EXPECT_LT(val2, val1 + error);
    EXPECT_LT(val1, val2 + error);
}

void run_simple_sequence() {
    double naps[] = {50, 100, 200, 400};
    std::string msgs[] = {"Start unit test", "Put your right hand in",
        "Take your right hand out", "Okay, now we're done"};
    explain::task_t root(msgs[0]), *task = &root;
    nap(naps[0]);
    task = task->new_task(msgs[1]);
    nap(naps[1]);
    task = task->new_task(msgs[2]);
    nap(naps[2]);
    task = task->new_task(msgs[3]);
    nap(naps[3]);
    task->finish();

    counted_t<const ql::datum_t> res = root.as_datum();
    for (size_t i = 0; i < res->size(); ++i) {
        approx_eq(naps[i] * 1000000, res->get(i)->get("time")->as_num(), 1000000);
        EXPECT_EQ(msgs[i], res->get(i)->get("description")->as_str());
    }
}

TEST(Explain, SimpleSequence) {
    run_in_thread_pool(&run_simple_sequence);
}

} //namespace unittest
