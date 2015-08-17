// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef UNITTEST_UNITTEST_UTILS_HPP_
#define UNITTEST_UNITTEST_UTILS_HPP_

#include <functional>
#include <set>
#include <string>

#include "arch/address.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/serialize_macros.hpp"

namespace unittest {

std::string rand_string(int len);

serializer_filepath_t make_unittest_filepaths(const std::string &permanent_path,
                                              const std::string &temporary_path);

class temp_file_t {
public:
    temp_file_t();
    ~temp_file_t();
    serializer_filepath_t name() const;

private:
    std::string filename;

    DISABLE_COPYING(temp_file_t);
};

void let_stuff_happen();

std::set<ip_address_t> get_unittest_addresses();

void run_in_thread_pool(const std::function<void()> &fun, int num_workers = 1);

read_t make_sindex_read(
    ql::datum_t key, const std::string &id);

/* Easy way to make shard ranges. Examples:
 - `quick_range("A-Z")` contains any key starting with a capital letter.
 - `quick_range("A-M")` includes "Aardvark" and "Mammoth" but not "Quail".
 - `quick_range("*-*")` is `key_range_t::universe()`.
 - `quick_range("*-M")` and `quick_range("N-*")` are adjacent but do not overlap.
The `quick_region()` variant just wraps the `key_range_t` in a `region_t`. */
key_range_t quick_range(const char *bounds);
region_t quick_region(const char *bounds);

state_timestamp_t make_state_timestamp(int n);

}  // namespace unittest


#define TPTEST(group, name, ...) void run_##group##_##name();           \
    TEST(group, name) {                                                 \
        ::unittest::run_in_thread_pool(run_##group##_##name, ##__VA_ARGS__);      \
    }                                                                   \
    void run_##group##_##name()

#define TPTEST_MULTITHREAD(group, name, j) void run_##group##_##name(); \
    TEST(group, name##MultiThread) {                                    \
        ::unittest::run_in_thread_pool(run_##group##_##name, j);        \
    }                                                                   \
    TPTEST(group, name)


#endif /* UNITTEST_UNITTEST_UTILS_HPP_ */
