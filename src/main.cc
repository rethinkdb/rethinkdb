
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "utils.hpp"
#include "worker_pool.hpp"

void event_handler(event_queue_t *event_queue, event_t *event) {
    int res;
    ssize_t sz;
    char buf[256];

    if(event != NULL) {
        bzero(buf, sizeof(buf));
        sz = read(event->resource, buf, sizeof(buf));
        check("Could not read from socket", sz == -1);
        if(sz > 0) {
            printf("(worker: %d, size: %d) Msg: %s", event_queue->queue_id, (int)sz, buf);
            int offset = atoi(buf);
            /*
            iocb request;
            // TODO: gotta have a buffer per IO
            io_prep_pread(&request, event_queue->pool->file_fd, buf, 15, offset);
            iocb* requests[1];
            requests[0] = &request;
            res = io_submit(event_queue->aio_context, 1, requests);
            check("Could not submit IO request", res < 1);*/
        } else {
            printf("Closing socket %d\n", event->resource);
            queue_forget_resource(event_queue, event->resource);
            close(event->resource);
        }
    }
}

/******
 * Socket handling
 **/
void process_socket(int sockfd, worker_pool_t *worker_pool) {
    event_queue_t *event_queue = next_active_worker(worker_pool);
    queue_watch_resource(event_queue, sockfd);
    printf("Connected to socket %d\n", sockfd);
}

int main(int argc, char *argv[])
{
    // Create a pool of workers
    worker_pool_t worker_pool;
    worker_pool.data = (void*)open("leo.txt", O_DIRECT | O_NOATIME | O_RDONLY);
    create_worker_pool(&worker_pool, event_handler);
    
    // Create the socket
    int sockfd, res;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    check("Couldn't create socket", sockfd == -1);

    // Bind the socket
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    res = bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    check("Couldn't create socket", res != 0);

    // Start listening to connections
    res = listen(sockfd, 5);
    check("Couldn't listen to the socket", res != 0);

    // Accept incoming connections
    while(1) {
        // TODO: add a sound way to get out of this loop
        int newsockfd;
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        newsockfd = accept(sockfd, 
                           (sockaddr*)&client_addr, 
                           &client_addr_len);
        check("Could not accept connection", newsockfd == -1);

        // Process the socket
        res = fcntl(newsockfd, F_SETFL, O_NONBLOCK);
        check("Could not make socket non-blocking", res != 0);
        process_socket(newsockfd, &worker_pool);
    }

    // Cleanup the resources
    res = close(sockfd);
    check("Could not shutdown socket", res != 0);
    res = close((int)(long)worker_pool.data);
    check("Could not close served file", res != 0);
    destroy_worker_pool(&worker_pool);
}

