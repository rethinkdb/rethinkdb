#include "unittest/gtest.hpp"

#include <string>
#include <cstdlib>
#include "server/cmd_args.hpp"

namespace unittest {

class temp_file_t {
    char * filename;
public:
    temp_file_t(const char * tmpl) {
        size_t len = strlen(tmpl);
        filename = new char[len+1];
        strncpy(filename, tmpl, len);
        int fd = mkstemp(filename);
        guarantee_err(fd != -1, "Couldn't create a temporary file");
        close(fd);
    }
    operator const char *() {
        return filename;
    }
    ~temp_file_t() {
        unlink(filename);
        delete [] filename;
    }
};

TEST(SnapshotsTest, server_start) {
    /*
    ASSERT_FALSE(merged.next());
    ASSERT_EQ(a->blocked_without_prefetch, 0);
    */
    temp_file_t db_file("/tmp/rdb_unittest.XXXXXX");
}

}
