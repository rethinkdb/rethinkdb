
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
#include "tty.hpp"
#include "server.hpp"
#include "fsm.hpp"

/*
void event_handler(event_queue_t *event_queue, event_t *event) {
    int res;
    size_t sz;
    char buf[256];

    if(event->event_type == et_sock) {
        bzero(buf, sizeof(buf));
        // TODO: make sure we don't leave any data in the socket
        sz = read(event->source, buf, sizeof(buf));
        check("Could not read from socket", sz == -1);
        if(sz > 0) {
            // See if we have a quit message
            if(strncmp(buf, "quit", 4) == 0) {
                printf("Quitting server...\n");
                res = pthread_kill(event_queue->parent_pool->main_thread, SIGINT);
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
    } else if(event->event_type == et_disk) {
        // We got async IO event back
        // TODO: what happens to unfreed memory if event doesn't come back? (is it possible?)
        if(event->result < 0) {
            printf("File notify error (fd %d, res: %d) %s\n",
                   event->source, event->result, strerror(-event->result));
            res = write((int)(long)event->state, "err", 4);
            check("Could not write to socket", res == -1);
            // TODO: make sure we write everything we intend to
        } else {
            ((char*)event->buf)[11] = 0;
            res = write((int)(long)event->state, event->buf, 10);
            check("Could not write to socket", res == -1);
            // TODO: make sure we write everything we intend to
        }
        event_queue->alloc.free((buffer_t<512>*)event->buf);
    } else if(event->event_type == et_timer) {
    }
}
*/

void event_handler(event_queue_t *event_queue, event_t *event) {
    if(event->event_type != et_timer) {
        fsm_do_transition(event_queue, event);
    }
}

void term_handler(int signum) {
    // We'll naturally break out of the main loop because the accept
    // syscall will get interrupted.
}

void install_handlers() {
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
};

int main(int argc, char *argv[])
{
    int res;

    // Setup signal handlers
    install_handlers();

    // Create a pool of workers
    worker_pool_t worker_pool;
    
    int datafd = open("leo.txt", O_DIRECT | O_NOATIME | O_RDONLY);
    //int datafd = open("/mnt/ssd/test", O_DIRECT | O_NOATIME | O_RDONLY);
    check("Could not open data file", datafd < 0);
    
    worker_pool.data = (void*)datafd;
    create_worker_pool(&worker_pool, event_handler, pthread_self());

    // Start the server (in a separate thread)
    int sockfd = start_server(&worker_pool);

    // Feed the terminal into the listening socket
    do_tty_loop(sockfd);

    // At this point we broke out of the tty loop. Stop the server.
    stop_server(sockfd);

    // Clean up the rest
    destroy_worker_pool(&worker_pool);
    res = close((int)(long)worker_pool.data);
    check("Could not close served file", res != 0);
    printf("Server offline\n");
}

