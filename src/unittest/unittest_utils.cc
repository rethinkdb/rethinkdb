// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/unittest_utils.hpp"

#include <stdlib.h>

#include <functional>

#include "arch/timing.hpp"
#include "arch/runtime/starter.hpp"
#include "rdb_protocol/protocol.hpp"
#include "unittest/gtest.hpp"
#include "utils.hpp"

namespace unittest {

struct make_sindex_read_t {
    static rdb_protocol_t::read_t make_sindex_read(
            counted_t<const ql::datum_t> key, const std::string &id) {
        datum_range_t rng(key, key_range_t::closed, key, key_range_t::closed);
        return rdb_protocol_t::read_t(
            rdb_protocol_t::rget_read_t(
                rdb_protocol_t::region_t::universe(),
                std::map<std::string, ql::wire_func_t>(),
                ql::batchspec_t::user(ql::batch_type_t::NORMAL,
                                      counted_t<const ql::datum_t>()),
                std::vector<rdb_protocol_details::transform_variant_t>(),
                boost::optional<rdb_protocol_details::terminal_variant_t>(),
                rdb_protocol_t::sindex_rangespec_t(
                    id,
                    rdb_protocol_t::region_t(rng.to_sindex_keyrange()),
                    rng),
                sorting_t::UNORDERED),
            profile_bool_t::PROFILE);
    }
};

rdb_protocol_t::read_t make_sindex_read(
        counted_t<const ql::datum_t> key, const std::string &id) {
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
    return get_local_ips(std::set<ip_address_t>(), false);
}

void run_in_thread_pool(const std::function<void()> &fun, int num_workers) {
    ::run_in_thread_pool(fun, num_workers);
}

}  // namespace unittest
