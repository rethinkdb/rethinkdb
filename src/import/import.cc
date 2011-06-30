#include "arch/arch.hpp"
#include "import/import.hpp"
#include <math.h>
#include "db_thread_info.hpp"
#include "protocol/memcached/memcached.hpp"
#include "diskinfo.hpp"
#include "concurrency/cond_var.hpp"
#include "logger.hpp"
#include "replication/master.hpp"
#include "replication/slave.hpp"
#include "gated_store.hpp"
#include "concurrency/promise.hpp"

namespace import {

void usage() {
}

/* cmd_config_t parse_cmd_args(int argc, char *argv[]) {
    return cmd_config_t()
} */

//eventually import should have its own main function. Right n

/* void import_main(cmd_config_t *cmd_config, thread_pool_t *thread_pool) {
} */

} //namespace import

/* int run_import(int argc, char *argv[]) {
    return 0;
} */

