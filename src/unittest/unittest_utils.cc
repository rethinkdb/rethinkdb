// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/unittest_utils.hpp"

#include <stdlib.h>

#include <functional>

#include "arch/timing.hpp"
#include "arch/runtime/starter.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/protocol.hpp"
#include "unittest/gtest.hpp"
#include "utils.hpp"

namespace unittest {

std::string rand_string(int len) {
    std::string res;

    int seed = randint(RAND_MAX);

    while (len --> 0) {
        res.push_back((seed % 26) + 'A');
        seed ^= seed >> 17;
        seed += seed << 11;
        seed ^= seed >> 29;
    }

    return res;
}

struct make_sindex_read_t {
    static read_t make_sindex_read(ql::datum_t key, const std::string &id) {
        ql::datum_range_t rng(key, key_range_t::closed, key, key_range_t::closed);
        return read_t(
            rget_read_t(
                boost::optional<changefeed_stamp_t>(),
                region_t::universe(),
                std::map<std::string, ql::wire_func_t>(),
                "",
                ql::batchspec_t::default_for(ql::batch_type_t::NORMAL),
                std::vector<ql::transform_variant_t>(),
                boost::optional<ql::terminal_variant_t>(),
                sindex_rangespec_t(id, boost::none, rng),
                sorting_t::UNORDERED),
            profile_bool_t::PROFILE,
            read_mode_t::SINGLE);
    }
};

read_t make_sindex_read(
        ql::datum_t key, const std::string &id) {
    return make_sindex_read_t::make_sindex_read(key, id);
}

serializer_filepath_t manual_serializer_filepath(const std::string &permanent_path,
                                                 const std::string &temporary_path) {

    return serializer_filepath_t(permanent_path, temporary_path);
}


static const char *const temp_file_create_suffix = ".create";

temp_file_t::temp_file_t() {
    for (;;) {
        char tmpl[] = "/tmp/rdb_unittest.XXXXXX";
        const int fd = mkstemp(tmpl);
        guarantee_err(fd != -1, "Couldn't create a temporary file");
        close(fd);

        // Check that both the permanent and temporary file paths are unused.
        const std::string tmpfilename = std::string(tmpl) + temp_file_create_suffix;
        if (::access(tmpfilename.c_str(), F_OK) == -1 && get_errno() == ENOENT) {
            filename = tmpl;
            break;
        } else {
            const int unlink_res = ::unlink(tmpl);
            EXPECT_EQ(0, unlink_res);
        }
    }
}

temp_file_t::~temp_file_t() {
    // Unlink both possible locations of the file.
    const int res1 = ::unlink(name().temporary_path().c_str());
    EXPECT_TRUE(res1 == 0 || get_errno() == ENOENT);
    const int res2 = ::unlink(name().permanent_path().c_str());
    EXPECT_TRUE(res2 == 0 || get_errno() == ENOENT);
}

serializer_filepath_t temp_file_t::name() const {
    return manual_serializer_filepath(filename, filename + temp_file_create_suffix);
}


void let_stuff_happen() {
#ifdef VALGRIND
    nap(2000);
#else
    nap(100);
#endif
}

std::set<ip_address_t> get_unittest_addresses() {
    return get_local_ips(std::set<ip_address_t>(),
                         local_ip_filter_t::MATCH_FILTER_OR_LOOPBACK);
}

void run_in_thread_pool(const std::function<void()> &fun, int num_workers) {
    ::run_in_thread_pool(fun, num_workers);
}

key_range_t quick_range(const char *bounds) {
    guarantee(strlen(bounds) == 3);
    char left = bounds[0];
    guarantee(bounds[1] == '-');
    char right = bounds[2];
    if (left != '*' && right != '*') {
        guarantee(left <= right);
    }
    key_range_t r;
    r.left = (left == '*')
        ? store_key_t()
        : store_key_t(std::string(1, left));
    r.right = (right == '*')
        ? key_range_t::right_bound_t()
        : key_range_t::right_bound_t(store_key_t(std::string(1, right+1)));
    return r;
}

region_t quick_region(const char *bounds) {
    return region_t(quick_range(bounds));
}

state_timestamp_t make_state_timestamp(int n) {
    state_timestamp_t t;
    t.num = n;
    return t;
}

}  // namespace unittest
