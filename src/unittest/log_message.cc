// Copyright 2010-2012 RethinkDB, all rights reserved.

#include "clustering/administration/logs/log_writer.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

TEST(LogMessageTest, ParseFormat) {

    struct timespec timestamp = clock_realtime();
    struct timespec uptime = clock_monotonic();

    log_message_t message(timestamp, uptime, log_level_info, "test message *(&Q(!#@LJVO");
    std::string formatted = format_log_message(message);
    log_message_t parsed = parse_log_message(formatted);
    EXPECT_EQ(message.timestamp.tv_sec, parsed.timestamp.tv_sec);
    EXPECT_EQ(message.uptime.tv_sec, parsed.uptime.tv_sec);
    EXPECT_EQ(message.level, parsed.level);
    EXPECT_EQ(message.message, parsed.message);
}

void test_chunks(const std::vector<size_t> &sizes) {
#ifdef _WIN32
    std::string filename = strprintf("c:\\windows\\temp\\rethinkdb-unittest-file-reverse-reader-%09d", randint(1000000000));
    HANDLE handle = CreateFile(filename.c_str(), GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    guarantee_winerr(handle != INVALID_HANDLE_VALUE, "CreateFile failed");
    scoped_fd_t fd(handle);
#else
    char filename[] = "/tmp/rethinkdb-unittest-file-reverse-reader-XXXXXX";
    scoped_fd_t fd(mkstemp(filename));
#endif
    guarantee(fd.get() != INVALID_FD);
#ifndef _WIN32
    int unlink_res = unlink(filename);
    guarantee_err(unlink_res == 0, "unlink failed");
#endif
    for (size_t i = 0; i < sizes.size(); ++i) {
        std::string line(sizes[i], 'A' + i);
        line += '\n';
#ifdef _WIN32
        DWORD res;
        BOOL ret = WriteFile(fd.get(), line.data(), line.size(), &res, nullptr);
        guarantee_winerr(ret, "WriteFile failed");
#else
        int res = write(fd.get(), line.data(), line.size());
#endif
        ASSERT_EQ(line.size(), res);
    }
#ifndef _WIN32
    fsync(fd.get());
#endif
    file_reverse_reader_t rr(std::move(fd));
    for (ssize_t i = sizes.size() - 1; i >= 0; --i) {
        std::string expected(sizes[i], 'A' + i);
        std::string actual;
        bool ok = rr.get_next(&actual);
        ASSERT_TRUE(ok);
        ASSERT_EQ(expected, actual);
    }
    std::string dummy;
    bool ok = rr.get_next(&dummy);
    ASSERT_FALSE(ok);
}

TPTEST(LogMessageTest, FileReverseReader) {
    for (size_t a = 0; a < 10; ++a) {
        for (size_t b = 0; b < 10; ++b) {
            for (size_t c = 0; c < 10; ++c) {
                test_chunks({4096-5+a, b, c});
            }
        }
    }
}

}  // namespace unittest
