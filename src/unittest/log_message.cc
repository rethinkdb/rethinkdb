#include "unittest/gtest.hpp"

#include "clustering/administration/logger.hpp"

namespace unittest {

TEST(LogMessageTest, ParseFormat) {

    struct timespec timestamp;
    int res = clock_gettime(CLOCK_REALTIME, &timestamp);
    ASSERT_EQ(0, res);

    struct timespec uptime;
    res = clock_gettime(CLOCK_MONOTONIC, &uptime);
    ASSERT_EQ(0, res);

    log_message_t message(timestamp, uptime, log_level_info, "test message *(&Q(!#@LJVO");
    std::string formatted = format_log_message(message);
    log_message_t parsed = parse_log_message(formatted);
    EXPECT_EQ(message.timestamp.tv_sec, parsed.timestamp.tv_sec);
    EXPECT_EQ(message.uptime.tv_sec, parsed.uptime.tv_sec);
    EXPECT_EQ(message.level, parsed.level);
    EXPECT_EQ(message.message, parsed.message);
}

}  // namespace unittest
