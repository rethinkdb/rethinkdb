#include "unittest/gtest.hpp"

#include "clustering/administration/logger.hpp"

namespace unittest {

TEST(LogMessageTest, ParseFormat) {

    struct timespec timestamp;
    clock_monotonic(&timestamp);

    struct timespec uptime;
    clock_monotonic(&uptime);

    log_message_t message(timestamp, uptime, log_level_info, "test message *(&Q(!#@LJVO");
    std::string formatted = format_log_message(message);
    log_message_t parsed = parse_log_message(formatted);
    EXPECT_EQ(message.timestamp.tv_sec, parsed.timestamp.tv_sec);
    EXPECT_EQ(message.uptime.tv_sec, parsed.uptime.tv_sec);
    EXPECT_EQ(message.level, parsed.level);
    EXPECT_EQ(message.message, parsed.message);
}

}  // namespace unittest
