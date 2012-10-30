// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/uuid.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "unittest/gtest.hpp"
#include "utils.hpp"


namespace unittest {
TEST(VectorClock, Construction) {
    vclock_t<int> blank; //contains an unitialized value
    ASSERT_FALSE(blank.in_conflict());

    uuid_t machine1 = generate_uuid();
    vclock_t<int> val(0, machine1);
    ASSERT_FALSE(val.in_conflict());

    semilattice_join(&blank, val);
    ASSERT_FALSE(blank.in_conflict());

    ASSERT_EQ(blank.get(), 0);
}

TEST(VectorClock, NewVersion) {
    vclock_t<int> val;

    uuid_t machine1 = generate_uuid(), machine2 = generate_uuid(),
                       machine3 = generate_uuid(), machine4 = generate_uuid();

    semilattice_join(&val, val.make_new_version(1, machine1));
    ASSERT_EQ(val.get(), 1);
    semilattice_join(&val, val.make_new_version(2, machine2));
    ASSERT_EQ(val.get(), 2);
    semilattice_join(&val, val.make_new_version(3, machine3));
    ASSERT_EQ(val.get(), 3);

    vclock_t<int> change = val;
    change.get_mutable() = 4;
    change.upgrade_version(machine4);

    semilattice_join(&val, change);

    ASSERT_EQ(val.get(), 4);
}

TEST(VectorClock, Conflict) {
    uuid_t machine1 = generate_uuid(), machine2 = generate_uuid();

    vclock_t<int> m_1s_opinion(1, machine1);
    vclock_t<int> m_2s_opinion(2, machine2);

    vclock_t<int> merge;

    semilattice_join(&merge, m_1s_opinion);
    semilattice_join(&merge, m_2s_opinion);

    ASSERT_TRUE(merge.in_conflict());

    //make sure conflicted things throw on gets
    ASSERT_THROW(merge.get(), in_conflict_exc_t);
    ASSERT_THROW(merge.get_mutable(), in_conflict_exc_t);
    ASSERT_THROW(merge.make_new_version(3, machine2), in_conflict_exc_t);
    ASSERT_THROW(merge.upgrade_version(machine2), in_conflict_exc_t);

    uuid_t resolving_machine = generate_uuid();
    vclock_t<int> resolution = merge.make_resolving_version(3, resolving_machine);
    semilattice_join(&merge, resolution);

    ASSERT_FALSE(merge.in_conflict());
    ASSERT_EQ(merge.get(), 3);
}

} //namespace unittest
