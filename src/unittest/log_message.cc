#include "unittest/gtest.hpp"

#include "clustering/administration/logger.hpp"

namespace unittest {

TEST(LogMessageTest, ParseFormat) {

    struct timespec ts;
    int res = clock_gettime(CLOCK_MONOTONIC, &ts);
    ASSERT_EQ(0, res);

    log_message_t message(time(NULL), ts, log_level_info, "test message *(&Q(!#@LJVO");
    std::string formatted = format_log_message(message);
    log_message_t parsed = parse_log_message(formatted);
    EXPECT_EQ(message.timestamp, parsed.timestamp);
    EXPECT_EQ(message.uptime.tv_sec, parsed.uptime.tv_sec);
    EXPECT_EQ(message.level, parsed.level);
    EXPECT_EQ(message.message, parsed.message);
}

}  // namespace unittest
