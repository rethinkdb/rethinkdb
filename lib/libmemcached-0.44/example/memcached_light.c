/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/**
 * What is a library without an example to show you how to use the library?
 * This example use both interfaces to implement a small memcached server.
 * Please note that this is an exemple on how to use the library, not
 * an implementation of a scalable memcached server. If you look closely
 * at the example it isn't even multithreaded ;-)
 *
 * With that in mind, let me give you some pointers into the source:
 *   storage.c/h       - Implements the item store for this server and not really
 *                       interesting for this example.
 *   interface_v0.c    - Shows an implementation of the memcached server by using
 *                       the "raw" access to the packets as they arrive
 *   interface_v1.c    - Shows an implementation of the memcached server by using
 *                       the more "logical" interface.
 *   memcached_light.c - This file sets up all of the sockets and run the main
 *                       message loop.
 *
 *
 * config.h is included so that I can use the ntohll/htonll on platforms that
 * doesn't have that (this is a private function inside libmemcached, so you
 * cannot use it directly from libmemcached without special modifications to
 * the library)
 */

#include "config.h"
#include <assert.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <event.h>

#include <libmemcached/protocol_handler.h>
#include <libmemcached/byteorder.h>
#include "storage.h"
#include "memcached_light.h"

extern memcached_binary_protocol_callback_st interface_v0_impl;
extern memcached_binary_protocol_callback_st interface_v1_impl;

static memcached_socket_t server_sockets[1024];
static int num_server_sockets= 0;

struct connection
{
  void *userdata;
  struct event event;
};

/* The default maximum number of connections... (change with -c) */
static int maxconns = 1024;

static struct connection *socket_userdata_map;
static bool verbose= false;
static struct event_base *event_base;

struct options_st {
  char *pid_file;
  bool has_port;
  in_port_t port;
} global_options;

typedef struct options_st options_st;

/**
 * Callback for driving a client connection
 * @param fd the socket for the client socket
 * @param which identifying the event that occurred (not used)
 * @param arg the connection structure for the client
 */
static void drive_client(memcached_socket_t fd, short which, void *arg)
{
  (void)which;
  struct connection *client= arg;
  struct memcached_protocol_client_st* c= client->userdata;
  assert(c != NULL);

  memcached_protocol_event_t events= memcached_protocol_client_work(c);
  if (events & MEMCACHED_PROTOCOL_ERROR_EVENT)
  {
    memcached_protocol_client_destroy(c);
    closesocket(fd);
  } else {
    short flags = 0;
    if (events & MEMCACHED_PROTOCOL_WRITE_EVENT)
    {
      flags= EV_WRITE;
    }

    if (events & MEMCACHED_PROTOCOL_READ_EVENT)
    {
      flags|= EV_READ;
    }

    event_set(&client->event, (intptr_t)fd, flags, drive_client, client);
    event_base_set(event_base, &client->event);

    if (event_add(&client->event, 0) == -1)
    {
      (void)fprintf(stderr, "Failed to add event for %d\n", fd);
      memcached_protocol_client_destroy(c);
      closesocket(fd);
    }
  }
}

/**
 * Callback for accepting new connections
 * @param fd the socket for the server socket
 * @param which identifying the event that occurred (not used)
 * @param arg the connection structure for the server
 */
static void accept_handler(memcached_socket_t fd, short which, void *arg)
{
  (void)which;
  struct connection *server= arg;
  /* accept new client */
  struct sockaddr_storage addr;
  socklen_t addrlen= sizeof(addr);
  memcached_socket_t sock= accept(fd, (struct sockaddr *)&addr, &addrlen);

  if (sock == INVALID_SOCKET)
  {
    perror("Failed to accept client");
    return ;
  }

#ifndef WIN32
  if (sock >= maxconns)
  {
    (void)fprintf(stderr, "Client outside socket range (specified with -c)\n");
    closesocket(sock);
    return ;
  }
#endif

  struct memcached_protocol_client_st* c;
  c= memcached_protocol_create_client(server->userdata, sock);
  if (c == NULL)
  {
    (void)fprintf(stderr, "Failed to create client\n");
    closesocket(sock);
  }
  else
  {
    struct connection *client = &socket_userdata_map[sock];
    client->userdata= c;

    event_set(&client->event, (intptr_t)sock, EV_READ, drive_client, client);
    event_base_set(event_base, &client->event);
    if (event_add(&client->event, 0) == -1)
    {
      (void)fprintf(stderr, "Failed to add event for %d\n", sock);
      memcached_protocol_client_destroy(c);
      closesocket(sock);
    }
  }
}

/**
 * Create a socket and bind it to a specific port number
 * @param port the port number to bind to
 */
static int server_socket(const char *port)
{
  struct addrinfo *ai;
  struct addrinfo hints= { .ai_flags= AI_PASSIVE,
                           .ai_family= AF_UNSPEC,
                           .ai_socktype= SOCK_STREAM };

  int error= getaddrinfo("127.0.0.1", port, &hints, &ai);
  if (error != 0)
  {
    if (error != EAI_SYSTEM)
      fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(error));
    else
      perror("getaddrinfo()");

    return 1;
  }

  struct linger ling= {0, 0};

  for (struct addrinfo *next= ai; next; next= next->ai_next)
  {
    memcached_socket_t sock= socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock == INVALID_SOCKET)
    {
      perror("Failed to create socket");
      continue;
    }

    int flags;
#ifdef WIN32
    u_long arg = 1;
    if (ioctlsocket(sock, FIONBIO, &arg) == SOCKET_ERROR)
    {
      perror("Failed to set nonblocking io");
      closesocket(sock);
      continue;
    }
#else
    flags= fcntl(sock, F_GETFL, 0);
    if (flags == -1)
    {
      perror("Failed to get socket flags");
      closesocket(sock);
      continue;
    }

    if ((flags & O_NONBLOCK) != O_NONBLOCK)
    {
      if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
      {
        perror("Failed to set socket to nonblocking mode");
        closesocket(sock);
        continue;
      }
    }
#endif

    flags= 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags)) != 0)
      perror("Failed to set SO_REUSEADDR");

    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) != 0)
      perror("Failed to set SO_KEEPALIVE");

    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling)) != 0)
      perror("Failed to set SO_LINGER");

    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) != 0)
      perror("Failed to set TCP_NODELAY");

    if (bind(sock, next->ai_addr, next->ai_addrlen) == SOCKET_ERROR)
    {
      if (get_socket_errno() != EADDRINUSE)
      {
        perror("bind()");
        freeaddrinfo(ai);
      }
      closesocket(sock);
      continue;
    }

    if (listen(sock, 1024) == SOCKET_ERROR)
    {
      perror("listen()");
      closesocket(sock);
      continue;
    }

    server_sockets[num_server_sockets++]= sock;
  }

  freeaddrinfo(ai);

  return (num_server_sockets > 0) ? 0 : 1;
}

/**
 * Convert a command code to a textual string
 * @param cmd the comcode to convert
 * @return a textual string with the command or NULL for unknown commands
 */
static const char* comcode2str(uint8_t cmd)
{
  static const char * const text[] = {
    "GET", "SET", "ADD", "REPLACE", "DELETE",
    "INCREMENT", "DECREMENT", "QUIT", "FLUSH",
    "GETQ", "NOOP", "VERSION", "GETK", "GETKQ",
    "APPEND", "PREPEND", "STAT", "SETQ", "ADDQ",
    "REPLACEQ", "DELETEQ", "INCREMENTQ", "DECREMENTQ",
    "QUITQ", "FLUSHQ", "APPENDQ", "PREPENDQ"
  };

  if (cmd <= PROTOCOL_BINARY_CMD_PREPENDQ)
    return text[cmd];

  return NULL;
}

/**
 * Print out the command we are about to execute
 */
static void pre_execute(const void *cookie __attribute__((unused)),
                        protocol_binary_request_header *header __attribute__((unused)))
{
  if (verbose)
  {
    const char *cmd= comcode2str(header->request.opcode);
    if (cmd != NULL)
      fprintf(stderr, "pre_execute from %p: %s\n", cookie, cmd);
    else
      fprintf(stderr, "pre_execute from %p: 0x%02x\n", cookie, header->request.opcode);
  }
}

/**
 * Print out the command we just executed
 */
static void post_execute(const void *cookie __attribute__((unused)),
                         protocol_binary_request_header *header __attribute__((unused)))
{
  if (verbose)
  {
    const char *cmd= comcode2str(header->request.opcode);
    if (cmd != NULL)
      fprintf(stderr, "post_execute from %p: %s\n", cookie, cmd);
    else
      fprintf(stderr, "post_execute from %p: 0x%02x\n", cookie, header->request.opcode);
  }
}

/**
 * Callback handler for all unknown commands.
 * Send an unknown command back to the client
 */
static protocol_binary_response_status unknown(const void *cookie,
                                               protocol_binary_request_header *header,
                                               memcached_binary_protocol_raw_response_handler response_handler)
{
  protocol_binary_response_no_extras response= {
    .message= {
      .header.response= {
        .magic= PROTOCOL_BINARY_RES,
        .opcode= header->request.opcode,
        .status= htons(PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND),
        .opaque= header->request.opaque
      }
    }
  };

  return response_handler(cookie, header, (void*)&response);
}

/**
 * Program entry point. Bind to the specified port(s) and serve clients
 *
 * @param argc number of items in the argument vector
 * @param argv argument vector
 * @return 0 on success, 1 otherwise
 */
int main(int argc, char **argv)
{
  int cmd;
  memcached_binary_protocol_callback_st *interface= &interface_v0_impl;

  memset(&global_options, 0, sizeof(global_options));

  event_base= event_init();
  if (event_base == NULL)
  {
    fprintf(stderr, "Failed to create an instance of libevent\n");
    return 1;
  }

  /*
   * We need to initialize the handlers manually due to a bug in the
   * warnings generated by struct initialization in gcc (all the way up to 4.4)
   */
  initialize_interface_v0_handler();

  while ((cmd= getopt(argc, argv, "v1p:P:?hc:")) != EOF)
  {
    switch (cmd) {
    case '1':
      interface= &interface_v1_impl;
      break;
    case 'P':
      global_options.pid_file= strdup(optarg);
      break;
    case 'p':
      global_options.has_port= true;
      (void)server_socket(optarg);
      break;
    case 'v':
      verbose= true;
      break;
    case 'c':
      maxconns= atoi(optarg);
      break;
    case 'h':  /* FALLTHROUGH */
    case '?':  /* FALLTHROUGH */
    default:
      (void)fprintf(stderr, "Usage: %s [-p port] [-v] [-1] [-c #clients] [-P pidfile]\n",
                    argv[0]);
      return 1;
    }
  }

  if (! initialize_storage())
  {
    /* Error message already printed */
    return 1;
  }

  if (! global_options.has_port)
    (void)server_socket("9999");

  if (global_options.pid_file)
  {
    FILE *pid_file;
    uint32_t pid;

    pid_file= fopen(global_options.pid_file, "w+");

    if (pid_file == NULL)
    {
      perror(strerror(get_socket_errno()));
      abort();
    }

    pid= (uint32_t)getpid();
    fprintf(pid_file, "%u\n", pid);
    fclose(pid_file);
  }

  if (num_server_sockets == 0)
  {
    fprintf(stderr, "I don't have any server sockets\n");
    return 1;
  }

  /*
   * Create and initialize the handles to the protocol handlers. I want
   * to be able to trace the traffic throught the pre/post handlers, and
   * set up a common handler for unknown messages
   */
  interface->pre_execute= pre_execute;
  interface->post_execute= post_execute;
  interface->unknown= unknown;

  struct memcached_protocol_st *protocol_handle;
  if ((protocol_handle= memcached_protocol_create_instance()) == NULL)
  {
    fprintf(stderr, "Failed to allocate protocol handle\n");
    return 1;
  }

  socket_userdata_map= calloc((size_t)(maxconns), sizeof(struct connection));
  if (socket_userdata_map == NULL)
  {
    fprintf(stderr, "Failed to allocate room for connections\n");
    return 1;
  }

  memcached_binary_protocol_set_callbacks(protocol_handle, interface);
  memcached_binary_protocol_set_pedantic(protocol_handle, true);

  for (int xx= 0; xx < num_server_sockets; ++xx)
  {
    struct connection *conn= &socket_userdata_map[server_sockets[xx]];
    conn->userdata= protocol_handle;
    event_set(&conn->event, (intptr_t)server_sockets[xx], EV_READ | EV_PERSIST,
              accept_handler, conn);
    event_base_set(event_base, &conn->event);
    if (event_add(&conn->event, 0) == -1)
    {
      fprintf(stderr, "Failed to add event for %d\n", server_sockets[xx]);
      closesocket(server_sockets[xx]);
    }
  }

  /* Serve all of the clients */
  event_base_loop(event_base, 0);

  /* NOTREACHED */
  return 0;
}
