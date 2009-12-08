
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "worker_pool.hpp"

int start_server(worker_pool_t *worker_pool);
void stop_server(int sockfd);

#endif // __SERVER_HPP__

