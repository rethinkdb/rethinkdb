#include "unittest/gtest.hpp"

#include "arch/linux/disk/conflict_resolving.hpp"
#include <list>
#include <boost/scoped_array.hpp>

namespace unittest {

/* dummy_diskmgr_t is a disk manager used to test conflict_resolving_diskmgr_t. It just records
actions and then runs them all when you call run_all(). It also checks manually to confirm that
they do not conflict. */

struct dummy_diskmgr_t {

    struct action_t {
        action_t(bool is_read, fd_t fd, void *buf, size_t count, off_t offset, linux_iocallback_t *cb)
            : is_read(is_read), fd(fd), buf(buf), count(count), offset(offset), cb(cb) { }

        bool get_is_read() const { return is_read; }
        fd_t get_fd() const { return fd; }
        void *get_buf() const { return buf; }
        size_t get_count() const { return count; }
        off_t get_offset() const { return offset; }

        bool is_read;
        fd_t fd;
        void *buf;
        size_t count;
        off_t offset;
        linux_iocallback_t *cb;
    };

    std::list<action_t*> running_actions;
    std::vector<char> data;

    void act(action_t *a) {
        /* The conflict_resolving_diskmgr_t should not have sent us two potentially
        conflicting actions */
        for (std::list<action_t*>::iterator it = running_actions.begin();
             it != running_actions.end(); it++) {
            if (!(a->is_read && (*it)->is_read)) {
                /* They aren't both reads, so they should be non-overlapping. */
                ASSERT_TRUE(
                    (int)a->offset >= (int)((*it)->offset + (*it)->count) ||
                    (int)(*it)->offset >= (int)(a->offset + a->count));
            }
        }
        running_actions.push_back(a);
    }

    bool is_running(bool is_read, fd_t fd, void *buf, size_t count, off_t offset) {
        for (std::list<action_t*>::iterator it = running_actions.begin();
             it != running_actions.end(); it++) {
            if ((*it)->is_read == is_read && (*it)->fd == fd && (*it)->buf == buf &&
                    (*it)->count == count && (*it)->offset == offset) {
                return true;
            }
        }
        return false;
    }

    void complete_all() {
        while (!running_actions.empty()) {
            action_t *a = running_actions.front();
            running_actions.pop_front();
            if (a->offset + a->count > data.size()) data.resize(a->offset + a->count, 0);
            if (a->is_read) {
                memcpy(a->buf, data.data() + a->offset, a->count);
            } else {
                memcpy(data.data() + a->offset, a->buf, a->count);
            }
            a->cb->on_io_complete();
        }
    }
};

typedef conflict_resolving_diskmgr_t<dummy_diskmgr_t> test_diskmgr_t;

/* test_driver_t, read_test_t, and write_test_t are together used to specify workloads
to send to the conflict_resolving_diskmgr_t. */

struct test_driver_t {

    dummy_diskmgr_t dummy_diskmgr;
    test_diskmgr_t test_diskmgr;

    test_driver_t() : test_diskmgr(&dummy_diskmgr) { }

    void complete_all() {
        dummy_diskmgr.complete_all();
    }
};

struct read_test_t : public linux_iocallback_t {

    read_test_t(test_driver_t *driver, off_t o, std::string e) :
        driver(driver),
        offset(o),
        expected(e),
        buffer(new char[expected.size()]),
        action(true, 0, buffer.get(), expected.size(), offset, this),
        completed(false)
    {
        driver->test_diskmgr.act(&action);
    }
    test_driver_t *driver;
    off_t offset;
    std::string expected;
    boost::scoped_array<char> buffer;
    test_diskmgr_t::action_t action;
    bool completed;
    bool was_sent() {
        return completed || driver->dummy_diskmgr.is_running(
            true, 0, buffer.get(), expected.size(), offset);
    }
    void on_io_complete() {
        ASSERT_FALSE(completed);
        completed = true;
        ASSERT_TRUE(0 == memcmp(expected.data(), buffer.get(), expected.size())) << "Read returned wrong data.";
    }
    ~read_test_t() {
        EXPECT_TRUE(completed);
    }
};

struct write_test_t : public linux_iocallback_t {

    write_test_t(test_driver_t *driver, off_t o, std::string d) :
        driver(driver),
        offset(o),
        data(d),
        action(false, 0, const_cast<void*>(reinterpret_cast<const void*>(data.data())), data.size(), offset, this),
        completed(false)
    {
        driver->test_diskmgr.act(&action);
    }
    test_driver_t *driver;
    off_t offset;
    std::string data;
    test_diskmgr_t::action_t action;
    bool completed;
    bool was_sent() {
        return completed || driver->dummy_diskmgr.is_running(
            false, 0, const_cast<void*>(reinterpret_cast<const void*>(data.data())), data.size(), offset);
    }
    void on_io_complete() {
        ASSERT_FALSE(completed);
        completed = true;
    }
    ~write_test_t() {
        EXPECT_TRUE(completed);
    }
};

/* WriteWriteConflict verifies that if two writes are sent, they will be run in the correct
order. */

TEST(DiskConflictTest, WriteWriteConflict) {
    test_driver_t d;
    write_test_t w1(&d, 0, "foo");
    write_test_t w2(&d, 0, "bar");
    read_test_t verifier(&d, 0, "bar");
    ASSERT_TRUE(w1.was_sent());
    d.complete_all();
}

/* WriteReadConflict verifies that if a write and then a read are sent, the write will happen
before the read. */

TEST(DiskConflictTest, WriteReadConflict) {
    test_driver_t d;
    write_test_t initial_write(&d, 0, "initial");
    write_test_t w(&d, 0, "foo");
    read_test_t r(&d, 0, "foo");
    ASSERT_TRUE(initial_write.was_sent());
    d.complete_all();
}

/* ReadWriteConflict verifies that if a read and then a write are sent, the read will happen
before the write. */

TEST(DiskConflictTest, ReadWriteConflict) {
    test_driver_t d;
    write_test_t initial_write(&d, 0, "initial");
    read_test_t r(&d, 0, "init");
    write_test_t w(&d, 0, "something_else");
    ASSERT_TRUE(initial_write.was_sent());
    d.complete_all();
}

/* NoSpuriousConflicts verifies that if two writes that don't overlap are sent, there are
no problems. */

TEST(DiskConflictTest, NoSpuriousConflicts) {
    test_driver_t d;
    write_test_t w1(&d, 0, "foo");
    write_test_t w2(&d, 4096, "bar");
    ASSERT_TRUE(w1.was_sent());
    ASSERT_TRUE(w2.was_sent());
    d.complete_all();
}

/* MetaTest is a sanity check to make sure that the above tests are actually testing something. */

void cause_test_failure() {
    test_driver_t d;
    write_test_t w(&d, 0, "foo");
    read_test_t r(&d, 0, "bar");
    d.complete_all();
}

TEST(DiskConflictTest, MetaTest) {
    EXPECT_FATAL_FAILURE(cause_test_failure(), "Read returned wrong data.");
};

}