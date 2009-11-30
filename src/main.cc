
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
#include "blackhole_alloc.hpp"

void event_handler(event_queue_t *event_queue, event_t *event) {
    int res;
    size_t sz;
    char buf[256];

    if(event->event_type == et_sock_event) {
        bzero(buf, sizeof(buf));
        // TODO: make sure we don't leave any data in the socket
        sz = read(event->source, buf, sizeof(buf));
        check("Could not read from socket", sz == -1);
        if(sz > 0) {
            // See if we have a quit message
            if(strncmp(buf, "quit", 4) == 0) {
                printf("Quitting server...\n");
                res = pthread_kill(event_queue->parent_pool->main_thread, SIGTERM);
                check("Could not send kill signal to main thread", res != 0);
                return;
            }

            // Allocate a buffer to read file into
            void *gbuf = event_queue->alloc.malloc<buffer_t<512> >();

            // Fire off an async IO event
            bzero(gbuf, 512);
            int offset = atoi(buf);
            // TODO: Using parent_pool might cause cache line
            // alignment issues. Can we eliminate it (perhaps by
            // giving each thread its own private copy of the
            // necessary data)?
            schedule_aio_read((int)(long)event_queue->parent_pool->data,
                              offset, 512, gbuf, event_queue, (void*)event->source);
        } else {
            // No data left, close the socket
            printf("Closing socket %d\n", event->source);
            queue_forget_resource(event_queue, event->source);
            close(event->source);
        }
    } else if(event->event_type == et_disk_event) {
        // We got async IO event back
        // TODO: what happens to unfreed memory if event doesn't come back? (is it possible?)
        if(event->result < 0) {
            printf("File notify error (fd %d, res: %d) %s\n",
                   event->source, event->result, strerror(-event->result));
        } else {
            ((char*)event->buf)[11] = 0;
            res = write((int)(long)event->state, event->buf, 10);
            check("Could not write to socket", res == -1);
            // TODO: make sure we write everything we intend to
        }
        event_queue->alloc.free((buffer_t<512>*)event->buf);
    } else if(event->event_type == et_timer_event) {
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
    queue_watch_resource(event_queue, sockfd, eo_read, NULL);
    printf("Opened socket %d\n", sockfd);
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
    
    //int datafd = open("leo.txt", O_DIRECT | O_NOATIME | O_RDONLY);
    int datafd = open("/mnt/ssd/test", O_DIRECT | O_NOATIME | O_RDONLY);
    check("Could not open data file", datafd < 0);
    
    worker_pool.data = (void*)datafd;
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
    while(true) {
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

    // Stop accepting connections
    res = shutdown(sockfd, SHUT_RDWR);
    check("Could not shutdown main socket", res == -1);
    res = close(sockfd);
    check("Could not close main socket", res != 0);
    // Clean up the rest
    destroy_worker_pool(&worker_pool);
    res = close((int)(long)worker_pool.data);
    check("Could not close served file", res != 0);
    printf("Server offline\n");
}

