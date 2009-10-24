
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

/******
 * Utils
 **/
void check(const char *str, int error) {
    if (error) {
        if(errno == 0)
            errno = EINVAL;
        perror(str);
        exit(-1);
    }
}

/******
 * Worker
 **/
struct worker_t {
    pthread_t thread;
    int epoll_fd;
};

void* worker_routine(void *arg) {
    int res;
    worker_t *self = (worker_t*)arg;
    epoll_event events[10];
    char buf[256];
    
    do {
        res = epoll_wait(self->epoll_fd, events, sizeof(events), -1);
        check("Waiting for an event failed", res == -1);

        for(int i = 0; i < res; i++) {
            bzero(buf, sizeof(buf));
            read(events[i].data.fd, buf, sizeof(buf));
            printf("Msg: %s\n", buf);
        }
    } while(1);
    
    return NULL;
}

void create_worker(worker_t *worker) {
    // Create a poll fd
    worker->epoll_fd = epoll_create(0);
    check("Could not create epoll fd", worker->epoll_fd == -1);

    // Start the thread
    int res = pthread_create(&worker->thread, NULL, worker_routine, (void*)worker);
    check("Could not create worker thread", res != 0);
}

/******
 * Worker Pool
 **/
struct worker_pool_t {
    worker_t *workers;
    int nworkers;
    int active_worker;
};

void create_worker_pool(int workers, worker_pool_t *worker_pool) {
    worker_pool->nworkers = workers;
    worker_pool->workers = (worker_t*)malloc(sizeof(worker_t) * workers);
    for(int i = 0; i < workers; i++) {
        create_worker(&worker_pool->workers[i]);
    }
    worker_pool->active_worker = 0;
}

void destroy_worker_pool(worker_pool_t *worker_pool) {
    // TODO: ensure threads are terminated
    free(worker_pool->workers);
    worker_pool->nworkers = 0;
}

int next_active_worker(worker_pool_t *worker_pool) {
    int worker = worker_pool->active_worker++;
    if(worker_pool->active_worker >= worker_pool->nworkers)
        worker_pool->active_worker = 0;
    return worker;
}

/******
 * Main loop
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
}

int main(int argc, char *argv[])
{
    // Create a pool of workers
    int ncpus = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Number of CPUs: %d\n", ncpus);
    worker_pool_t worker_pool;
    create_worker_pool(ncpus, &worker_pool);
    
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
    destroy_worker_pool(&worker_pool);
}

