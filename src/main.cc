
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <libaio.h>
#include "utils.hpp"
#include "worker_pool.hpp"

/******
 * Main loops
 **/
void* aio_poll_routine(void *arg) {
    int res;
    io_event events[10];
    worker_t *self = (worker_t*)arg;
    do {
        res = io_getevents(self->aio_context, 1, sizeof(events), events, NULL);
        check("Could not get AIO events", res < 0);
        for(int i = 0; i < res; i++) {
            printf("Completion notification for AIO event %d\n", i);
        }
    } while(1);
}

void* epoll_routine(void *arg) {
    int res;
    ssize_t sz;
    worker_t *self = (worker_t*)arg;
    epoll_event events[10];
    char buf[256];
    
    do {
        res = epoll_wait(self->epoll_fd, events, sizeof(events), -1);
        check("Waiting for an event failed", res == -1);

        for(int i = 0; i < res; i++) {
            sz = -1;
            if(events[i].events == EPOLLIN) {
                bzero(buf, sizeof(buf));
                sz = read(events[i].data.fd, buf, sizeof(buf));
                check("Could not read from socket", sz == -1);
                if(sz > 0) {
                    printf("(worker: %d, event: %d, size: %d) Msg: %s", self->id, i, (int)sz, buf);
                    int offset = atoi(buf);
                    iocb request;
                    // TODO: gotta have a buffer per IO
                    io_prep_pread(&request, self->pool->file_fd, buf, 15, offset);
                    iocb* requests[1];
                    requests[0] = &request;
                    res = io_submit(self->aio_context, 1, requests);
                    check("Could not submit IO request", res < 1);
                }
            }
            if(events[i].events == EPOLLRDHUP ||
               events[i].events == EPOLLERR ||
               events[i].events == EPOLLHUP ||
               sz == 0) {
                printf("Closing socket %d\n", events[i].data.fd);
                res = epoll_ctl(self->epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
                check("Could remove socket from watching", res != 0);
                close(events[i].data.fd);
            }
        }
    } while(1);
    
    return NULL;
}

/******
 * Socket handling
 **/
void process_socket(int sockfd, worker_pool_t *worker_pool) {
    int nworker = next_active_worker(worker_pool);
    worker_t *worker = &worker_pool->workers[nworker];
    epoll_event event;
    event.events = EPOLLIN; // TODO: figure out edge vs. level triggering
    event.data.ptr = NULL;
    event.data.fd = sockfd;
    int res = epoll_ctl(worker->epoll_fd, EPOLL_CTL_ADD, sockfd, &event);
    check("Could not pass socket to worker", res != 0);
    printf("Connected to socket %d\n", sockfd);
}

int main(int argc, char *argv[])
{
    // Create a pool of workers
    int ncpus = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Number of CPUs: %d\n", ncpus);
    worker_pool_t worker_pool;
    worker_pool.file_fd = open("leo.txt", O_DIRECT | O_NOATIME | O_RDONLY);
    create_worker_pool(ncpus, &worker_pool, epoll_routine, aio_poll_routine);
    
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
    res = close(worker_pool.file_fd);
    check("Could not close served file", res != 0);
    destroy_worker_pool(&worker_pool);
}

