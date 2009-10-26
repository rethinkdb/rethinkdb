
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "utils.hpp"
#include "worker_pool.hpp"
#include "async_io.hpp"
#include "alloc_blackhole.hpp"

void event_handler(event_queue_t *event_queue, event_t *event) {
    int res;
    size_t sz;
    char buf[256];

    if(event->event_type == et_sock_event) {
        bzero(buf, sizeof(buf));
        sz = read(event->source, buf, sizeof(buf));
        check("Could not read from socket", sz == -1);
        if(sz > 0) {
            printf("(worker: %d, size: %d) Msg: %s", event_queue->queue_id, (int)sz, buf);
            if(strncmp(buf, "quit", 4) == 0) {
                printf("Quitting server...\n");
                res = pthread_kill(event_queue->parent_pool->main_thread, SIGTERM);
                check("Could not send kill signal to main thread", res != 0);
                return;
            }
            int offset = atoi(buf);
            char *gbuf = (char*)malloc(&event_queue->allocator, 512);
            bzero(gbuf, 512);
            schedule_aio_read((int)(long)event_queue->parent_pool->data,
                              offset, 512, gbuf, event_queue,
                              &event_queue->allocator);
        } else {
            printf("Closing socket %d\n", event->source);
            queue_forget_resource(event_queue, event->source);
            close(event->source);
        }
    } else {
        if(event->result < 0) {
            printf("File notify (fd %d, res: %d) %s\n",
                   event->source, event->result, strerror(-event->result));
        } else {
            printf("File notify (fd %d, res: %d) %s\n",
                   event->source, event->result, (char*)event->buf);
        }
    }
}

void term_handler(int signum) {
    // Do nothing, we'll naturally break out of the main loop because
    // the accept syscall will get interrupted.
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
    int res;
    
    // Setup termination handlers
    struct sigaction action;
    bzero((char*)&action, sizeof(action));
    action.sa_handler = term_handler;
    res = sigaction(SIGTERM, &action, NULL);
    check("Could not install TERM handler", res < 0);
    
    bzero((char*)&action, sizeof(action));
    action.sa_handler = term_handler;
    res = sigaction(SIGINT, &action, NULL);
    check("Could not install INT handler", res < 0);
    
    // Create a pool of workers
    worker_pool_t worker_pool;
    worker_pool.data = (void*)open("leo.txt", O_DIRECT | O_NOATIME | O_RDONLY);
    create_worker_pool(&worker_pool, event_handler, pthread_self());
    
    // Create the socket
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    check("Couldn't create socket", sockfd == -1);

    int sockoptval = 1;
    res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(sockoptval));
    check("Could not set REUSEADDR option", res == -1);

    // Bind the socket
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    res = bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    check("Couldn't bind socket", res != 0);

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
        // Break out of the loop on sigterm
        if(newsockfd == -1 && errno == EINTR)
            break;
        check("Could not accept connection", newsockfd == -1);

        // Process the socket
        res = fcntl(newsockfd, F_SETFL, O_NONBLOCK);
        check("Could not make socket non-blocking", res != 0);
        process_socket(newsockfd, &worker_pool);
    }

    // Cleanup the resources
    destroy_worker_pool(&worker_pool);
    res = shutdown(sockfd, SHUT_RDWR);
    check("Could not shutdown main socket", res == -1);
    res = close(sockfd);
    check("Could not close main socket", res != 0);
    res = close((int)(long)worker_pool.data);
    check("Could not close served file", res != 0);
    printf("Server offline\n");
}

