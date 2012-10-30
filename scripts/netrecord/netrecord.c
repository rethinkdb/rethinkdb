// Copyright 2010-2012 RethinkDB, all rights reserved.
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

typedef int fd_t;

typedef struct {
    fd_t socket;
    int id;
    const char *log_filename;
    struct sockaddr_in *server_addr;
    struct timeval start_time;
} start_info_t;

void interrupt_handler(int signum) {
    /* Do nothing; we will break out of the accept() loop when it gets EINTR */
}

void *handle_connection(void *);

int main(int argc, char *argv[]) {
    
    int res;
    
    /* Parse arguments */
    
    if (argc != 5) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "%s <server host> <server port> <proxy port> <log file root>\n", argv[0]);
        goto exit_badly;
    }
    
    struct hostent *server_host = gethostbyname(argv[1]);
    if (!server_host) {
        fprintf(stderr, "Could not find host '%s': %s\n", argv[1], hstrerror(h_errno));
        goto exit_badly;
    }
    
    int server_port = htons(atoi(argv[2]));
    if (server_port == 0) {
        fprintf(stderr, "Could not parse port '%s'\n", argv[2]);
        goto exit_badly;
    }
    
    int proxy_port = atoi(argv[3]);
    if (proxy_port == 0) {
        fprintf(stderr, "Could not parse port '%s'\n", argv[3]);
        goto exit_badly;
    }
    
    const char *log_filename = argv[4];
    
    /* Record start time */
    
    struct timeval start_time;
    res = gettimeofday(&start_time, NULL);
    if (res < 0) {
        fprintf(stderr, "Could not get time of day: %s\n", strerror(errno));
        goto exit_badly;
    }
    
    /* Prepare server address */
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = server_port;
    memcpy(&server_addr.sin_addr, server_host->h_addr_list[0], 4);
    
    /* Start listener socket */
    
    fd_t listener = socket(PF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        fprintf(stderr, "Could not create listener: %s\n", strerror(errno));
        goto exit_badly;
    }
    
    struct sockaddr_in proxy_addr;
    memset(&proxy_addr, 0, sizeof(struct sockaddr_in));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(proxy_port);
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    
    res = bind(listener, (struct sockaddr *)&proxy_addr, sizeof(struct sockaddr_in));
    if (res < 0) {
        fprintf(stderr, "Could not bind socket: %s\n", strerror(errno));
        goto close_listener_and_exit_badly;
    }
    
    res = listen(listener, 1);
    if (res < 0) {
        fprintf(stderr, "Could not listen: %s\n", strerror(errno));
        goto close_listener_and_exit_badly;
    }
    
    /* Install signal handler */
    
    struct sigaction action;
    bzero((char*)&action, sizeof(action));
    action.sa_handler = interrupt_handler;
    res = sigaction(SIGINT, &action, NULL);
    if (res < 0) {
        fprintf(stderr, "Could not install SIGINT handler: %s\n", strerror(errno));
        goto close_listener_and_exit_badly;
    }
    
    /* Main loop */
    
    fprintf(stderr, "Listening on port %d.\n", proxy_port);
    
    int next_connection_id = 1;
    
    for (;;) {
        
        /* Accept a new connection */
        
        fd_t client = accept(listener, NULL, NULL);
        if (client < 0) {
            if (errno == EINTR) {
                break;
            } else {
                fprintf(stderr, "Could not accept: %s\n", strerror(errno));
                goto close_listener_and_exit_badly;
            }
        }
        
        /* Start a new thread to handle it */
        
        start_info_t *info = malloc(sizeof(start_info_t));
        info->socket = client;
        info->id = next_connection_id++;
        info->log_filename = log_filename;
        info->server_addr = &server_addr;
        info->start_time = start_time;
        
        pthread_t thread_id;
        res = pthread_create(&thread_id, NULL, handle_connection, info);
        if (res < 0) {
            fprintf(stderr, "Could not create thread: %s\n", strerror(errno));
            goto close_listener_and_exit_badly;
        }
    }
    
    /* Clean up */
    
    fprintf(stderr, "Shutting down.\n");
    
    close(listener);
    
    return 0;
    
    /* Error handling */

close_listener_and_exit_badly:
    close(listener);
exit_badly:
    return 1;
}

typedef struct {
    int id;
    FILE *log;
    struct timeval start_time;
} thread_info_t;

typedef struct {
    fd_t fd;
    struct pollfd *pollfd;
    const char *name;
    char log_name;
} entity_info_t;

#define BUFFER_SIZE 4096

typedef struct {
    char buffer[BUFFER_SIZE];
    size_t backup;
    entity_info_t *in, *out;
} pipe_info_t;

int write_log_entry(thread_info_t *thread, const char *name, const char *msg);
int setnonblocking(thread_info_t *thread, entity_info_t *ent);
int check(thread_info_t *thread, entity_info_t *ent, entity_info_t *other, int *running);
int pump(thread_info_t *thread, pipe_info_t *pipe);

void *handle_connection(void *data) {
    
    int res;
    
    /* Initialize thread */
    
    start_info_t *start_info = data;
    
    thread_info_t thread;
    thread.id = start_info->id;
    thread.start_time = start_info->start_time;
    
    struct pollfd pollfds[2];
    
    fprintf(stderr, "Opened connection %d.\n", thread.id);
    
    /* Create log file */
    
    char log_filename[PATH_MAX];
    snprintf(log_filename, PATH_MAX, "%s_%d", start_info->log_filename, thread.id);
    
    thread.log = fopen(log_filename, "w");
    if (!thread.log) {
        fprintf(stderr, "Connection %d could not open log file '%s': %s\n",
            thread.id, log_filename, strerror(errno));
        goto free_start_info;
    }
    
    /* Write first log entry to show when connection was established */
    
    res = write_log_entry(&thread, "client", "con");
    if (res < 0) goto close_log;
    
    /* Prepare client */
    
    entity_info_t client;
    client.name = "client";
    
    client.fd = start_info->socket;
    
    res = setnonblocking(&thread, &client);
    if (res == -1) goto close_log;
    
    pollfds[0].fd = client.fd;
    pollfds[0].events = POLLIN | POLLHUP | POLLRDHUP | POLLERR;
    client.pollfd = &pollfds[0];
    
    /* Connect to the server */
    
    entity_info_t server;
    server.name = "server";
    
    server.fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server.fd < 0) {
        fprintf(stderr, "Connection %d socket() failed: %s\n", thread.id, strerror(errno));
        goto close_log;
    }
    
    res = connect(server.fd, (struct sockaddr *)start_info->server_addr, sizeof(struct sockaddr_in));
    if (res < 0) {
        fprintf(stderr, "Connection %d could not connect to server: %s\n", thread.id, strerror(errno));
        goto close_server;
    }
    
    res = setnonblocking(&thread, &server);
    if (res < 0) goto close_server;
    
    pollfds[1].fd = server.fd;
    pollfds[1].events = POLLIN | POLLHUP | POLLRDHUP | POLLERR;
    server.pollfd = &pollfds[1];
    
    /* Create pipes */
    
    pipe_info_t client_to_server;
    client_to_server.backup = 0;
    client_to_server.in = &client;
    client_to_server.out = &server;
    
    pipe_info_t server_to_client;
    server_to_client.backup = 0;
    server_to_client.in = &server;
    server_to_client.out = &client;
    
    /* Main loop */
    
    for (;;) {
    
        res = poll(pollfds, 2, -1);
        if (res <= 0) fprintf(stderr, "poll() failed with rc %d: %s\n", res, strerror(errno));
        
        /* Check for error conditions and hangups */
        
        int running = 1;
        
        res = check(&thread, &client, &server, &running);
        if (res < 0) goto close_server;
        if (!running) break;
        
        res = check(&thread, &server, &client, &running);
        if (res < 0) goto close_server;
        if (!running) break;
        
        /* Transfer data from client to server */
        
        res = pump(&thread, &client_to_server);
        if (res < 0) goto close_server;
        
        /* Transfer data from server to client */
        
        res = pump(&thread, &server_to_client);
        if (res < 0) goto close_server;
    }
    
    /* Clean up */

close_server:
    close(server.fd);

close_log:
    fclose(thread.log);
    
free_start_info:
    close(start_info->socket);
    free(start_info);
    
    return NULL;
}

int write_log_entry(thread_info_t *thread, const char *name, const char *msg) {
    
    int res;
    
    struct timeval current_time;
    res = gettimeofday(&current_time, NULL);
    if (res < 0) {
        fprintf(stderr, "Connection %d gettimeofday() failed: %s\n", thread->id, strerror(errno));
        return -1;
    }
    
    struct timeval delta;
    if (current_time.tv_usec > thread->start_time.tv_usec) {
        delta.tv_sec = current_time.tv_sec - thread->start_time.tv_sec;
        delta.tv_usec = current_time.tv_usec - thread->start_time.tv_usec;
    } else {
        delta.tv_sec = current_time.tv_sec - thread->start_time.tv_sec - 1;
        delta.tv_usec = current_time.tv_usec + 1000000 - thread->start_time.tv_usec;
    }
    
    res = fprintf(thread->log, "%c %d.%.6d %s\n", name[0], (int)delta.tv_sec, (int)delta.tv_usec, msg);
    if (res < 0) {
        fprintf(stderr, "Connection %d could not write to log: %s\n", thread->id, strerror(errno));
        return -1;
    }
    
    return 0;
}

int setnonblocking(thread_info_t *thread, entity_info_t *ent) {
    
    int res;
    
    int flags = fcntl(ent->fd, F_GETFL);
    
    flags |= O_NONBLOCK;
    
    res = fcntl(ent->fd, F_SETFL, flags);
    if (res < 0) {
        fprintf(stderr, "Connection %d could not make %s nonblocking: %s\n",
            thread->id, ent->name, strerror(errno));
        return -1;
    }
    
    return 0;
}

int check(thread_info_t *thread, entity_info_t *ent, entity_info_t *other, int *running) {
    
    int res;
    
    if (ent->pollfd->revents & POLLERR) {
        
        res = write_log_entry(thread, ent->name, "err");
        if (res < 0) return -1;
        
        fprintf(stderr, "Connection %d %s error.\n", thread->id, ent->name);
        return -1;
    }
    
    if ((ent->pollfd->revents & POLLHUP) || (ent->pollfd->revents & POLLRDHUP)) {
        
        res = write_log_entry(thread, ent->name, "hup");
        if (res < 0) return -1;
        
        res = shutdown(other->fd, SHUT_RDWR);
        if (res < 0) return -1;
        
        fprintf(stderr, "Connection %d %s hangup.\n", thread->id, ent->name);
        
        *running = 0;
    }
    
    /* TODO: Can we detect and properly handle the case where one half of a duplex connection is
    shut down but the other half remains open? */
    
    return 0;
}

int try_write(thread_info_t *thread, pipe_info_t *pipe);

int pump(thread_info_t *thread, pipe_info_t *pipe) {
    
    int res;
    
    if (pipe->out->pollfd->revents & POLLOUT) {
        
        res = try_write(thread, pipe);
        if (res < 0) return -1;
    }

    if (pipe->in->pollfd->revents & POLLIN && pipe->backup < BUFFER_SIZE) {
        
        ssize_t bytes_read = read(pipe->in->fd, pipe->buffer + pipe->backup, BUFFER_SIZE - pipe->backup);
        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            }
            fprintf(stderr, "Connection %d could not receive from %s: %s\n",
                thread->id, pipe->in->name, strerror(errno));
            return -1;
        }
        
        /* Put an entry into the log. When we write the log, we use blocking IO calls. We presume
        that the disk is faster than the network.
        
        Maybe it would be better to do this in try_write() so that the entry's timestamp is closer
        to the actual time that the data is sent to the server or client. */
        
        char count_buffer[20];
        sprintf(count_buffer, "%d", (int)bytes_read);
        res = write_log_entry(thread, pipe->in->name, count_buffer);
        if (res < 0) return -1;
        
        res = fwrite(pipe->buffer + pipe->backup, 1, bytes_read, thread->log);
        if (res != bytes_read) {
            fprintf(stderr, "Connection %d could not write to log: %s\n", thread->id, strerror(errno));
            return -1;
        }
        
        pipe->backup += bytes_read;
    }
    
    if (pipe->backup) {
        
        res = try_write(thread, pipe);
        if (res < 0) return -1;
    }
    
    if (pipe->backup)
        pipe->out->pollfd->events |= POLLOUT;
    else
        pipe->out->pollfd->events &= ~POLLOUT;
    
    return 0;
}

int try_write(thread_info_t *thread, pipe_info_t *pipe) {
    
    ssize_t bytes_written = write(pipe->out->fd, pipe->buffer, pipe->backup);
    if (bytes_written < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        fprintf(stderr, "Connection %d could not send to %s: %s\n",
            thread->id, pipe->out->name, strerror(errno));
        return -1;
    }
    
    memmove(pipe->buffer, pipe->buffer + bytes_written, pipe->backup - bytes_written);
    pipe->backup -= bytes_written;
    
    return 0;
}
