#include "concurrency/rwlock.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

void run_rwlock_test() {
    rwlock_t lock;
    scoped_ptr_t<rwlock_acq_t> a;
    scoped_ptr_t<rwlock_acq_t> b;
    scoped_ptr_t<rwlock_acq_t> c;
    scoped_ptr_t<rwlock_acq_t> d;
    scoped_ptr_t<rwlock_acq_t> e;
    scoped_ptr_t<rwlock_acq_t> f;
    scoped_ptr_t<rwlock_acq_t> g;
    scoped_ptr_t<rwlock_acq_t> h;
    scoped_ptr_t<rwlock_acq_t> i;
    scoped_ptr_t<rwlock_acq_t> j;


    a.init(new rwlock_acq_t(&lock, rwlock_access_t::read));

    ASSERT_TRUE(a->rwlock_acq_signal()->is_pulsed());

    b.init(new rwlock_acq_t(&lock, rwlock_access_t::read));

    ASSERT_TRUE(b->rwlock_acq_signal()->is_pulsed());

    c.init(new rwlock_acq_t(&lock, rwlock_access_t::write));

    ASSERT_FALSE(c->rwlock_acq_signal()->is_pulsed());

    a.reset();

    ASSERT_TRUE(b->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(c->rwlock_acq_signal()->is_pulsed());

    b.reset();

    ASSERT_TRUE(c->rwlock_acq_signal()->is_pulsed());

    d.init(new rwlock_acq_t(&lock, rwlock_access_t::read));
    e.init(new rwlock_acq_t(&lock, rwlock_access_t::read));

    ASSERT_FALSE(d->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(e->rwlock_acq_signal()->is_pulsed());

    c.reset();

    ASSERT_TRUE(d->rwlock_acq_signal()->is_pulsed());
    ASSERT_TRUE(e->rwlock_acq_signal()->is_pulsed());

    f.init(new rwlock_acq_t(&lock, rwlock_access_t::write));
    g.init(new rwlock_acq_t(&lock, rwlock_access_t::read));
    h.init(new rwlock_acq_t(&lock, rwlock_access_t::read));
    i.init(new rwlock_acq_t(&lock, rwlock_access_t::read));
    j.init(new rwlock_acq_t(&lock, rwlock_access_t::write));

    ASSERT_TRUE(d->rwlock_acq_signal()->is_pulsed());
    ASSERT_TRUE(e->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(f->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(g->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(h->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(i->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(j->rwlock_acq_signal()->is_pulsed());

    h.reset();

    ASSERT_TRUE(d->rwlock_acq_signal()->is_pulsed());
    ASSERT_TRUE(e->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(f->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(g->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(i->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(j->rwlock_acq_signal()->is_pulsed());

    f.reset();

    ASSERT_TRUE(d->rwlock_acq_signal()->is_pulsed());
    ASSERT_TRUE(e->rwlock_acq_signal()->is_pulsed());
    ASSERT_TRUE(g->rwlock_acq_signal()->is_pulsed());
    ASSERT_TRUE(i->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(j->rwlock_acq_signal()->is_pulsed());

    i.reset();

    ASSERT_TRUE(d->rwlock_acq_signal()->is_pulsed());
    ASSERT_TRUE(e->rwlock_acq_signal()->is_pulsed());
    ASSERT_TRUE(g->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(j->rwlock_acq_signal()->is_pulsed());

    e.reset();
    ASSERT_TRUE(d->rwlock_acq_signal()->is_pulsed());
    ASSERT_TRUE(g->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(j->rwlock_acq_signal()->is_pulsed());

    d.reset();
    ASSERT_TRUE(g->rwlock_acq_signal()->is_pulsed());
    ASSERT_FALSE(j->rwlock_acq_signal()->is_pulsed());

    j.reset();
    ASSERT_TRUE(g->rwlock_acq_signal()->is_pulsed());

    g.reset();
}

TEST(RwlockTest, OneAcq) {
    run_in_thread_pool(&run_rwlock_test);
}



}  // namespace unittest
