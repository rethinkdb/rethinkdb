#include <stdexcept>
#include <functional>
#include <list>
#include <iostream>
#include "btree/rget_internal.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

struct test_iterator : ordered_data_iterator<int> {
    typedef std::list<std::list<int> > data_blocks_t;
    test_iterator(data_blocks_t data_blocks) : prefetches_count(0), blocked_without_prefetch(0), data_blocks(data_blocks) {
        // remove empty blocks
        data_blocks.remove_if(std::mem_fun_ref(&data_blocks_t::value_type::empty));
        // add first empty block (so that we could check prefetch of the first block
        data_blocks.push_front(std::list<int>());
    }
    bool has_next() {
        if (data_blocks.empty())
            return false;
        if (!data_blocks.front().empty())
            return true;
        blocked_without_prefetch++;
        data_blocks.pop_front();
        return !data_blocks.empty();
    }
    int next() {
        if (data_blocks.empty())
            throw std::runtime_error("tried to get an element from an empty iterator");

        if (data_blocks.front().empty()) {
            blocked_without_prefetch++;
            data_blocks.pop_front();
        }
        if (data_blocks.empty())
            throw std::runtime_error("tried to get an element from an empty iterator");

        int result = data_blocks.front().front();
        data_blocks.front().pop_front();
        return result;
    }
    void prefetch() {
        prefetches_count++;
        if (!data_blocks.empty() && data_blocks.front().empty()) {
            data_blocks.pop_front();
        }
    }

    size_t prefetches_count;
    size_t blocked_without_prefetch;

private:
    data_blocks_t data_blocks;
};

// Helper functions

std::list<int> parse_list_of_ints(const std::string &l) {
    std::list<int> result;
    std::stringstream str(l);
    while (!str.eof()) {
        int v;
        str >> v;
        result.push_back(v);
    }
    return result;
}

// parse the string "1 2 3 | 42 56 | 13" into a list of lists of integers, using '|' as a list separator
test_iterator::data_blocks_t parse_data_blocks(const std::string &db) {
    test_iterator::data_blocks_t result;
    std::stringstream str(db);
    std::list<int> current_list;
    while (!str.eof()) {
        std::string w;
        str >> w;
        if (w == "|") {
            result.push_back(current_list);
            current_list = std::list<int>();
        } else {
            int v;
            std::stringstream(w) >> v;
            current_list.push_back(v);
        }
    }
    if (!current_list.empty())
        result.push_back(current_list);
    return result;
}

std::list<int> data_blocks_to_list_of_ints(test_iterator::data_blocks_t& db) {
    std::list<int> result;
    for (test_iterator::data_blocks_t::iterator it = db.begin(); it != db.end(); ++it) {
        for (std::list<int>::iterator i = (*it).begin(); i != (*it).end(); ++i) {
            result.push_back(*i);
        }
        //std::copy((*it).begin(), (*it).end(), result.end());
    }
    return result;
}

// Tests

TEST(MergeIteratorsTest, merge_empty) {
    test_iterator::data_blocks_t empty_blocks;
    test_iterator a(empty_blocks);
    test_iterator b(empty_blocks);
    test_iterator c(empty_blocks);

    std::vector<ordered_data_iterator<int>*> mergees;
    mergees.push_back(&a);
    mergees.push_back(&b);
    mergees.push_back(&c);

    merge_ordered_data_iterator<int> merged(mergees);
    ASSERT_FALSE(merged.has_next());
    ASSERT_EQ(a.blocked_without_prefetch, 0);
    ASSERT_EQ(b.blocked_without_prefetch, 0);
    ASSERT_EQ(c.blocked_without_prefetch, 0);
}

TEST(MergeIteratorsTest, parse_data_blocks) {
    test_iterator::data_blocks_t db = parse_data_blocks("1 2 3 | 42 56 | 93");
    ASSERT_EQ(db.size(), 3);
    ASSERT_EQ(db.front().size(), 3);
    ASSERT_EQ(db.back().size(), 1);
}

TEST(MergeIteratorsTest, three_way_merge) {
    test_iterator::data_blocks_t a_db = parse_data_blocks("1 2 3 | 7 | 10");
    test_iterator::data_blocks_t b_db = parse_data_blocks("4 | 6 8 9 | 11 13 16");
    test_iterator::data_blocks_t c_db = parse_data_blocks("5 8 | 9 | 12 15 16");

    test_iterator a(a_db);
    test_iterator b(b_db);
    test_iterator c(c_db);
    std::vector<ordered_data_iterator<int>*> mergees;
    mergees.push_back(&a);
    mergees.push_back(&b);
    mergees.push_back(&c);
    merge_ordered_data_iterator<int> merge_iterator(mergees);

    std::list<int> merged;
    while (merge_iterator.has_next()) {
        int v = merge_iterator.next();
        merged.push_back(v);
    }

    std::list<int> a_db_flat = data_blocks_to_list_of_ints(a_db);
    std::list<int> b_db_flat = data_blocks_to_list_of_ints(b_db);
    std::list<int> c_db_flat = data_blocks_to_list_of_ints(c_db);

    std::list<int> merged_expected;
    merged_expected.merge(a_db_flat);
    merged_expected.merge(b_db_flat);
    merged_expected.merge(c_db_flat);
    
    ASSERT_TRUE(std::equal(merged_expected.begin(), merged_expected.end(), merged.begin()));
    ASSERT_EQ(a.blocked_without_prefetch, 0);
    ASSERT_EQ(b.blocked_without_prefetch, 0);
    ASSERT_EQ(c.blocked_without_prefetch, 0);
}

}
