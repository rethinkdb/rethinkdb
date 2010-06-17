
#include <retest.hpp>
#include "concurrency/rwi_lock.hpp"
#include "alloc/malloc.hpp"

struct mock_config_t {
    typedef malloc_alloc_t alloc_t;
};

typedef rwi_lock<mock_config_t> rwi_lock_t;

/* Read, followed by an op */
void test_basic_rr() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_read, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_read, NULL), true);
}

void test_basic_rw() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_read, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_write, NULL), false);
}

void test_basic_ri() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_read, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_intent, NULL), true);
}

void test_basic_rii() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_read, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_intent, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_intent, NULL), false);
}

/* Write followed by an op */
void test_basic_wr() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_write, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_read, NULL), false);
}

void test_basic_ww() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_write, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_write, NULL), false);
}

void test_basic_wi() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_write, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_intent, NULL), false);
}

/* Intent followed by an op (or two) */
void test_basic_ir() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_intent, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_read, NULL), true);
}

void test_basic_iw() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_intent, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_write, NULL), false);
}

void test_basic_ii() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_intent, NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_intent, NULL), false);
}

void test_basic_iu() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_intent, NULL), true);
    assert_eq(lock.upgrade_intent_to_write(NULL), true);
}

void test_basic_iur() {
    rwi_lock_t lock;
    assert_eq(lock.lock(rwi_lock_t::rwi_intent, NULL), true);
    assert_eq(lock.upgrade_intent_to_write(NULL), true);
    assert_eq(lock.lock(rwi_lock_t::rwi_read, NULL), false);
}

