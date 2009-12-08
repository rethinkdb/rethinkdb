
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <signal.h>
#include "utils.hpp"
#include "worker_pool.hpp"
#include "event_queue.hpp"
#include "server.hpp"

static pthread_t server_thread;
struct loop_info_t {
    int sockfd;
    worker_pool_t *worker_pool;
};
static loop_info_t loop_info;

void process_socket(int sockfd, worker_pool_t *worker_pool) {
    event_queue_t *event_queue = next_active_worker(worker_pool);
    queue_watch_resource(event_queue, sockfd, eo_read, NULL);
    printf("Opened socket %d\n", sockfd);
}

void* do_server_loop(void *arg) {
    loop_info_t *loop_info = (loop_info_t*)arg;
    int sockfd = loop_info->sockfd;
    worker_pool_t *worker_pool = loop_info->worker_pool;
    
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
        int res = fcntl(newsockfd, F_SETFL, O_NONBLOCK);
        check("Could not make socket non-blocking", res != 0);
        process_socket(newsockfd, worker_pool);
    }
}

int start_server(worker_pool_t *worker_pool) {
    // Create the socket
    int res, sockfd;
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

    // Start the server loop
    loop_info.sockfd = sockfd;
    loop_info.worker_pool = worker_pool;
    res = pthread_create(&server_thread, NULL, do_server_loop, (void*)&loop_info);
    check("Could not create server thread", res != 0);

    return sockfd;
}

void stop_server(int sockfd) {
    int res;
    
    // Break the loop
    res = pthread_kill(server_thread, SIGINT);
    check("Could not send kill signal to server thread", res != 0);
    
    res = pthread_join(server_thread, NULL);
    check("Could not join with the server thread", res != 0);

    // Stop accepting connections
    res = shutdown(sockfd, SHUT_RDWR);
    check("Could not shutdown main socket", res == -1);
    res = close(sockfd);
    check("Could not close main socket", res != 0);
}

