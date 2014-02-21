// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <functional>

#include "arch/runtime/coroutines.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/rwlock.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"
#include "utils.hpp"

namespace unittest {

struct read_after_write_state_t {
    rwlock_t lock;
    cond_t write_acquired;
    cond_t read_acquiring;
    cond_t write_released;
    cond_t read_acquired;
};

void read_after_write_cases(read_after_write_state_t *s, int i) {
    if (i == 0) {
        {
            rwlock_acq_t acq(&s->lock, access_t::write);
            s->write_acquired.pulse();
            s->read_acquiring.wait();
            // Read should not have acquired the lock.  It should currently be
            // waiting for the lock.  This is redundant with the
            // write_releasing.is_pulsed() assertion in the other branch.
            ASSERT_FALSE(s->read_acquired.is_pulsed());
        }
        s->write_released.pulse();

    } else if (i == 1) {
        s->write_acquired.wait();
        s->read_acquiring.pulse();
        rwlock_acq_t acq(&s->lock, access_t::read);
        // This assertion caught a bug in the original code, where read acquisition
        // would only check if the prev node was read-acquired and not
        // write-acquired.
        ASSERT_TRUE(s->write_released.is_pulsed());
        s->read_acquired.pulse();

    } else {
        unreachable();
    }
}

void read_after_write() {
    read_after_write_state_t s;

    pmap(2, std::bind(&read_after_write_cases, &s, ph::_1));
}

TEST(RwlockTest, ReadAfterWrite) {
    run_in_thread_pool(&read_after_write);
}



}  // namespace unittest
