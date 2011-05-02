
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "logger.hpp"
#include "server/key_value_store.hpp"
#include "utils.hpp"
#include "store.hpp"
#include "conn_acceptor.hpp"
#include "utils.hpp"
#include "perfmon.hpp"
#include "server/cmd_args.hpp"

/* run_server() is called by main() once it determines that the subprogram to be run
is 'serve' or 'create'. */

int run_server(int argc, char *argv[]);

/* server_main() is meant to be run in a coroutine. It should be started at server
start. It will return right when the server shuts down. */

void server_main(cmd_config_t *cmd_config, thread_pool_t *thread_pool);

#endif // __SERVER_HPP__

