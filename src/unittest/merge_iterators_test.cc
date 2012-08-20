#include <stdexcept>
#include <functional>
#include <list>

#include "containers/iterators.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

class test_iterator_t : public one_way_iterator_t<int> {
public:
    typedef std::list<std::list<int> > data_blocks_t;
    test_iterator_t(const data_blocks_t& _data_blocks, size_t *_blocked_without_prefetch)
        : prefetches_count(0), blocked_without_prefetch(_blocked_without_prefetch), data_blocks(_data_blocks) {
        *blocked_without_prefetch = 0;

        // remove empty blocks
        data_blocks.remove_if(std::mem_fun_ref(&data_blocks_t::value_type::empty));
        // add first empty block (so that we could check prefetch of the first block
        data_blocks.push_front(std::list<int>());
    }
    boost::optional<int> next() {
        if (data_blocks.empty())
            return boost::none;

        if (data_blocks.front().empty()) {
            ++*blocked_without_prefetch;
            data_blocks.pop_front();
        }
        if (data_blocks.empty())
            return boost::none;

        int result = data_blocks.front().front();
        data_blocks.front().pop_front();
        return boost::optional<int>(result);
    }
    void prefetch() {
        prefetches_count++;
        if (!data_blocks.empty() && data_blocks.front().empty()) {
            data_blocks.pop_front();
        }
    }

    size_t prefetches_count;
    size_t *blocked_without_prefetch;

private:
    data_blocks_t data_blocks;

    DISABLE_COPYING(test_iterator_t);
};

class delete_check_iterator_t : public test_iterator_t {
public:
    delete_check_iterator_t(const data_blocks_t& data_blocks, size_t *blocked_without_prefetch, bool *_deleted)
        : test_iterator_t(data_blocks, blocked_without_prefetch), deleted(_deleted) { *deleted = false; }
    virtual ~delete_check_iterator_t() {
        *deleted = true;
    }
private:
    bool *deleted;

    DISABLE_COPYING(delete_check_iterator_t);
};

// Helper functions

std::list<int> parse_list_of_ints(const std::string &_l) {
    std::string l = _l;
    l += " ";

    std::list<int> result;
    std::string w;
    for (int i = 0, e = l.size(); i < e; ++i) {
        if (l[i] != ' ') {
            w.push_back(l[i]);
        } else if (!w.empty()) {
            int v = atoi(w.c_str());
            rassert(v != 0);
            result.push_back(v);

            w.clear();
        }
    }

    // we put a space at the end of l for a reason!
    rassert(w.empty());

    return result;
}

// parse the string "1 2 3 | 42 56 | 13" into a list of lists of integers, using '|' as a list separator
test_iterator_t::data_blocks_t parse_data_blocks(const std::string &_db) {
    std::string db = _db;
    db += " ";
    test_iterator_t::data_blocks_t result;
    std::string w;
    std::list<int> current_list;  // oh god

    for (int i = 0, e = db.size(); i < e; ++i) {
        if (db[i] != ' ') {
            w.push_back(db[i]);
        } else if (!w.empty()) {
            if (w == "|") {
                result.push_back(current_list);
                current_list.clear();
            } else {
                int v = atoi(w.c_str());
                rassert(v != 0);
                current_list.push_back(v);
            }
            w.clear();
        }
    }

    // we put a space at the end of db for a reason!
    rassert(w.empty());

    if (!current_list.empty()) {
        result.push_back(current_list);
    }

    return result;
}

std::list<int> data_blocks_to_list_of_ints(const test_iterator_t::data_blocks_t& db) {
    std::list<int> result;
    for (test_iterator_t::data_blocks_t::const_iterator it = db.begin(); it != db.end(); ++it) {
        for (std::list<int>::const_iterator i = it->begin(); i != it->end(); ++i) {
            result.push_back(*i);
        }
    }
    return result;
}

// Tests

TEST(MergeIteratorsTest, merge_empty) {
    test_iterator_t::data_blocks_t empty_blocks;
    size_t blocked_without_prefetch_a, blocked_without_prefetch_b, blocked_without_prefetch_c;
    test_iterator_t *a = new test_iterator_t(empty_blocks, &blocked_without_prefetch_a);
    test_iterator_t *b = new test_iterator_t(empty_blocks, &blocked_without_prefetch_b);
    test_iterator_t *c = new test_iterator_t(empty_blocks, &blocked_without_prefetch_c);

    merge_ordered_data_iterator_t<int> merged;
    merged.add_mergee(a);
    merged.add_mergee(b);
    merged.add_mergee(c);

    ASSERT_FALSE(merged.next());
    ASSERT_EQ(blocked_without_prefetch_a, 0);
    ASSERT_EQ(blocked_without_prefetch_b, 0);
    ASSERT_EQ(blocked_without_prefetch_c, 0);
}

TEST(MergeIteratorsTest, parse_data_blocks) {
    test_iterator_t::data_blocks_t db = parse_data_blocks("1 2 3 | 42 56 | 93");
    ASSERT_EQ(db.size(), 3);
    ASSERT_EQ(db.front().size(), 3);
    ASSERT_EQ(db.back().size(), 1);
}

TEST(MergeIteratorsTest, three_way_merge) {
    test_iterator_t::data_blocks_t a_db = parse_data_blocks("1 2 3 | 7 | 10");
    test_iterator_t::data_blocks_t b_db = parse_data_blocks("4 | 6 8 9 | 11 13 16");
    test_iterator_t::data_blocks_t c_db = parse_data_blocks("5 8 | 9 | 12 15 16");

    size_t blocked_without_prefetch_a, blocked_without_prefetch_b, blocked_without_prefetch_c;
    test_iterator_t *a = new test_iterator_t(a_db, &blocked_without_prefetch_a);
    test_iterator_t *b = new test_iterator_t(b_db, &blocked_without_prefetch_b);
    test_iterator_t *c = new test_iterator_t(c_db, &blocked_without_prefetch_c);

    merge_ordered_data_iterator_t<int> merge_iterator;
    merge_iterator.add_mergee(a);
    merge_iterator.add_mergee(b);
    merge_iterator.add_mergee(c);

    std::list<int> merged;
    boost::optional<int> next;
    while ((next = merge_iterator.next())) {
        merged.push_back(next.get());
    }

    std::list<int> a_db_flat = data_blocks_to_list_of_ints(a_db);
    std::list<int> b_db_flat = data_blocks_to_list_of_ints(b_db);
    std::list<int> c_db_flat = data_blocks_to_list_of_ints(c_db);

    std::list<int> merged_expected;
    merged_expected.merge(a_db_flat);
    merged_expected.merge(b_db_flat);
    merged_expected.merge(c_db_flat);

    ASSERT_TRUE(std::equal(merged_expected.begin(), merged_expected.end(), merged.begin()));
    ASSERT_EQ(blocked_without_prefetch_a, 0);
    ASSERT_EQ(blocked_without_prefetch_b, 0);
    ASSERT_EQ(blocked_without_prefetch_c, 0);
}

TEST(MergeIteratorsTest, iterators_get_deleted) {
    test_iterator_t::data_blocks_t a_db = parse_data_blocks("1 2 3 | 7 | 10");
    test_iterator_t::data_blocks_t b_db = parse_data_blocks("4 | 6 8 9 | 11 13 16");
    test_iterator_t::data_blocks_t c_db = parse_data_blocks("5 8 | 9 | 12 15 16");
    test_iterator_t::data_blocks_t d_db = parse_data_blocks("");

    size_t blocked_without_prefetch_a, blocked_without_prefetch_b, blocked_without_prefetch_c, blocked_without_prefetch_d;
    bool a_deleted = false, b_deleted = false, c_deleted = false, d_deleted = false;
    test_iterator_t *a = new delete_check_iterator_t(a_db, &blocked_without_prefetch_a, &a_deleted);
    test_iterator_t *b = new delete_check_iterator_t(b_db, &blocked_without_prefetch_b, &b_deleted);
    test_iterator_t *c = new delete_check_iterator_t(c_db, &blocked_without_prefetch_c, &c_deleted);
    test_iterator_t *d = new delete_check_iterator_t(d_db, &blocked_without_prefetch_d, &d_deleted);

    merge_ordered_data_iterator_t<int> merge_iterator;
    merge_iterator.add_mergee(a);
    merge_iterator.add_mergee(b);
    merge_iterator.add_mergee(c);
    merge_iterator.add_mergee(d);

    (void) merge_iterator.next();
    EXPECT_TRUE(d_deleted);

    boost::optional<int> next;
    bool expect_a_deleted = false;
    while ((next = merge_iterator.next())) {
        if (expect_a_deleted) {
            EXPECT_TRUE(a_deleted) << "merge_ordered_data_iterator_t should delete the iterator after it has exhausted";
        }

        if (next.get() == 10) {
            expect_a_deleted = true; // when the next element is fetched, merge iterator will recognize that
                                     // 'a' iterator has exhausted, so it can delete it
        }
    }

    EXPECT_TRUE(a_deleted) << "merge_ordered_data_iterator_t should have deleted the 'a' iterator long ago";
    EXPECT_TRUE(b_deleted && c_deleted) << "merge_ordered_data_iterator_t should delete the iterator after it has exhausted";

    ASSERT_EQ(blocked_without_prefetch_a, 0);
    ASSERT_EQ(blocked_without_prefetch_b, 0);
    ASSERT_EQ(blocked_without_prefetch_c, 0);
    ASSERT_EQ(blocked_without_prefetch_d, 0);

    a_deleted = false;
    b_deleted = false;
    {
        test_iterator_t *a = new delete_check_iterator_t(a_db, &blocked_without_prefetch_a, &a_deleted);
        test_iterator_t *b = new delete_check_iterator_t(b_db, &blocked_without_prefetch_b, &b_deleted);

        merge_ordered_data_iterator_t<int> merge_iterator;
        merge_iterator.add_mergee(a);
        merge_iterator.add_mergee(b);

        // get some data from the merge iterator, but don't exhaust the iterators
        (void) merge_iterator.next();
        (void) merge_iterator.next();
        (void) merge_iterator.next();
    }
    EXPECT_TRUE(a_deleted && b_deleted) << "merge_ordered_data_iterator_t should delete the iterators on destruction, even when they have not been exhausted";
    ASSERT_EQ(blocked_without_prefetch_a, 0);
    ASSERT_EQ(blocked_without_prefetch_b, 0);
}

}  // namespace unittest
