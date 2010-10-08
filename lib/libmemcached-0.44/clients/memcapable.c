/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary:
 *
 */

/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#undef NDEBUG
#include "config.h"
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>

#include <libmemcached/memcached.h>
#include <libmemcached/memcached/protocol_binary.h>
#include <libmemcached/byteorder.h>
#include "utilities.h"

#ifdef linux
/* /usr/include/netinet/in.h defines macros from ntohs() to _bswap_nn to
 * optimize the conversion functions, but the prototypes generate warnings
 * from gcc. The conversion methods isn't the bottleneck for my app, so
 * just remove the warnings by undef'ing the optimization ..
 */
#undef ntohs
#undef ntohl
#endif

/* Should we generate coredumps when we enounter an error (-c) */
static bool do_core= false;
/* connection to the server */
static memcached_socket_t sock;
/* Should the output from test failures be verbose or quiet? */
static bool verbose= false;

/* The number of seconds to wait for an IO-operation */
static int timeout= 2;

/*
 * Instead of having to cast between the different datatypes we create
 * a union of all of the different types of pacages we want to send.
 * A lot of the different commands use the same packet layout, so I'll
 * just define the different types I need. The typedefs only contain
 * the header of the message, so we need some space for keys and body
 * To avoid to have to do multiple writes, lets add a chunk of memory
 * to use. 1k should be more than enough for header, key and body.
 */
typedef union
{
  protocol_binary_request_no_extras plain;
  protocol_binary_request_flush flush;
  protocol_binary_request_incr incr;
  protocol_binary_request_set set;
  char bytes[1024];
} command;

typedef union
{
  protocol_binary_response_no_extras plain;
  protocol_binary_response_incr incr;
  protocol_binary_response_decr decr;
  char bytes[1024];
} response;

enum test_return
{
  TEST_SKIP, TEST_PASS, TEST_PASS_RECONNECT, TEST_FAIL
};

/**
 * Try to get an addrinfo struct for a given port on a given host
 */
static struct addrinfo *lookuphost(const char *hostname, const char *port)
{
  struct addrinfo *ai= 0;
  struct addrinfo hints= {.ai_family=AF_UNSPEC,
    .ai_protocol=IPPROTO_TCP,
    .ai_socktype=SOCK_STREAM};
  int error= getaddrinfo(hostname, port, &hints, &ai);

  if (error != 0)
  {
    if (error != EAI_SYSTEM)
      fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(error));
    else
      perror("getaddrinfo()");
  }

  return ai;
}

/**
 * Set the socket in nonblocking mode
 * @return -1 if failure, the socket otherwise
 */
static memcached_socket_t set_noblock(void)
{
#ifdef WIN32
  u_long arg = 1;
  if (ioctlsocket(sock, FIONBIO, &arg) == SOCKET_ERROR)
  {
    perror("Failed to set nonblocking io");
    closesocket(sock);
    return INVALID_SOCKET;
  }
#else
  int flags= fcntl(sock, F_GETFL, 0);
  if (flags == -1)
  {
    perror("Failed to get socket flags");
    closesocket(sock);
    return INVALID_SOCKET;
  }

  if ((flags & O_NONBLOCK) != O_NONBLOCK)
  {
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      perror("Failed to set socket to nonblocking mode");
      closesocket(sock);
      return INVALID_SOCKET;
    }
  }
#endif
  return sock;
}

/**
 * Try to open a connection to the server
 * @param hostname the name of the server to connect to
 * @param port the port number (or service) to connect to
 * @return positive integer if success, -1 otherwise
 */
static memcached_socket_t connect_server(const char *hostname, const char *port)
{
  struct addrinfo *ai= lookuphost(hostname, port);
  sock= INVALID_SOCKET;
  if (ai != NULL)
  {
    if ((sock= socket(ai->ai_family, ai->ai_socktype,
                      ai->ai_protocol)) != INVALID_SOCKET)
    {
      if (connect(sock, ai->ai_addr, ai->ai_addrlen) == SOCKET_ERROR)
      {
        fprintf(stderr, "Failed to connect socket: %s\n",
                strerror(get_socket_errno()));
        closesocket(sock);
        sock= INVALID_SOCKET;
      }
      else
      {
        sock= set_noblock();
      }
    }
    else
      fprintf(stderr, "Failed to create socket: %s\n",
              strerror(get_socket_errno()));

    freeaddrinfo(ai);
  }

  return sock;
}

static ssize_t timeout_io_op(memcached_socket_t fd, short direction, void *buf, size_t len)
{
  ssize_t ret;

  if (direction == POLLOUT)
     ret= send(fd, buf, len, 0);
  else
     ret= recv(fd, buf, len, 0);

  if (ret == SOCKET_ERROR && get_socket_errno() == EWOULDBLOCK) {
    struct pollfd fds= {
      .events= direction,
      .fd= fd
    };

    int err= poll(&fds, 1, timeout * 1000);

    if (err == 1)
    {
      if (direction == POLLOUT)
         ret= send(fd, buf, len, 0);
      else
         ret= recv(fd, buf, len, 0);
    }
    else if (err == 0)
    {
      errno= ETIMEDOUT;
    }
    else
    {
      perror("Failed to poll");
      return -1;
    }
  }

  return ret;
}

/**
 * Ensure that an expression is true. If it isn't print out a message similar
 * to assert() and create a coredump if the user wants that. If not an error
 * message is returned.
 *
 */
static enum test_return ensure(bool val, const char *expression, const char *file, int line)
{
  if (!val)
  {
    if (verbose)
      fprintf(stderr, "\n%s:%d: %s", file, line, expression);

    if (do_core)
      abort();

    return TEST_FAIL;
  }

  return TEST_PASS;
}

#define verify(expression) do { if (ensure(expression, #expression, __FILE__, __LINE__) == TEST_FAIL) return TEST_FAIL; } while (0)
#define execute(expression) do { if (ensure(expression == TEST_PASS, #expression, __FILE__, __LINE__) == TEST_FAIL) return TEST_FAIL; } while (0)

/**
 * Send a chunk of memory over the socket (retry if the call is iterrupted
 */
static enum test_return retry_write(const void* buf, size_t len)
{
  size_t offset= 0;
  const char* ptr= buf;

  do
  {
    size_t num_bytes= len - offset;
    ssize_t nw= timeout_io_op(sock, POLLOUT, (void*)(ptr + offset), num_bytes);
    if (nw == -1)
      verify(get_socket_errno() == EINTR || get_socket_errno() == EAGAIN);
    else
      offset+= (size_t)nw;
  } while (offset < len);

  return TEST_PASS;
}

/**
 * Resend a packet to the server (All fields in the command header should
 * be in network byte order)
 */
static enum test_return resend_packet(command *cmd)
{
  size_t length= sizeof (protocol_binary_request_no_extras) +
          ntohl(cmd->plain.message.header.request.bodylen);

  execute(retry_write(cmd, length));
  return TEST_PASS;
}

/**
 * Send a command to the server. The command header needs to be updated
 * to network byte order
 */
static enum test_return send_packet(command *cmd)
{
  /* Fix the byteorder of the header */
  cmd->plain.message.header.request.keylen=
          ntohs(cmd->plain.message.header.request.keylen);
  cmd->plain.message.header.request.bodylen=
          ntohl(cmd->plain.message.header.request.bodylen);
  cmd->plain.message.header.request.cas=
          ntohll(cmd->plain.message.header.request.cas);

  execute(resend_packet(cmd));
  return TEST_PASS;
}

/**
 * Read a fixed length chunk of data from the server
 */
static enum test_return retry_read(void *buf, size_t len)
{
  size_t offset= 0;
  do
  {
    ssize_t nr= timeout_io_op(sock, POLLIN, ((char*) buf) + offset, len - offset);
    switch (nr) {
    case -1 :
       fprintf(stderr, "Errno: %d %s\n", get_socket_errno(), strerror(errno));
      verify(get_socket_errno() == EINTR || get_socket_errno() == EAGAIN);
      break;
    case 0:
      return TEST_FAIL;
    default:
      offset+= (size_t)nr;
    }
  } while (offset < len);

  return TEST_PASS;
}

/**
 * Receive a response from the server and conver the fields in the header
 * to local byte order
 */
static enum test_return recv_packet(response *rsp)
{
  execute(retry_read(rsp, sizeof(protocol_binary_response_no_extras)));

  /* Fix the byte order in the packet header */
  rsp->plain.message.header.response.keylen=
          ntohs(rsp->plain.message.header.response.keylen);
  rsp->plain.message.header.response.status=
          ntohs(rsp->plain.message.header.response.status);
  rsp->plain.message.header.response.bodylen=
          ntohl(rsp->plain.message.header.response.bodylen);
  rsp->plain.message.header.response.cas=
          ntohll(rsp->plain.message.header.response.cas);

  size_t bodysz= rsp->plain.message.header.response.bodylen;
  if (bodysz > 0)
    execute(retry_read(rsp->bytes + sizeof (protocol_binary_response_no_extras), bodysz));

  return TEST_PASS;
}

/**
 * Create a storage command (add, set, replace etc)
 *
 * @param cmd destination buffer
 * @param cc the storage command to create
 * @param key the key to store
 * @param keylen the length of the key
 * @param dta the data to store with the key
 * @param dtalen the length of the data to store with the key
 * @param flags the flags to store along with the key
 * @param exptime the expiry time for the key
 */
static void storage_command(command *cmd,
                            uint8_t cc,
                            const void* key,
                            size_t keylen,
                            const void* dta,
                            size_t dtalen,
                            uint32_t flags,
                            uint32_t exptime)
{
  /* all of the storage commands use the same command layout */
  protocol_binary_request_set *request= &cmd->set;

  memset(request, 0, sizeof (*request));
  request->message.header.request.magic= PROTOCOL_BINARY_REQ;
  request->message.header.request.opcode= cc;
  request->message.header.request.keylen= (uint16_t)keylen;
  request->message.header.request.extlen= 8;
  request->message.header.request.bodylen= (uint32_t)(keylen + 8 + dtalen);
  request->message.header.request.opaque= 0xdeadbeef;
  request->message.body.flags= flags;
  request->message.body.expiration= exptime;

  off_t key_offset= sizeof (protocol_binary_request_no_extras) + 8;
  memcpy(cmd->bytes + key_offset, key, keylen);
  if (dta != NULL)
    memcpy(cmd->bytes + key_offset + keylen, dta, dtalen);
}

/**
 * Create a basic command to send to the server
 * @param cmd destination buffer
 * @param cc the command to create
 * @param key the key to store
 * @param keylen the length of the key
 * @param dta the data to store with the key
 * @param dtalen the length of the data to store with the key
 */
static void raw_command(command *cmd,
                        uint8_t cc,
                        const void* key,
                        size_t keylen,
                        const void* dta,
                        size_t dtalen)
{
  /* all of the storage commands use the same command layout */
  memset(cmd, 0, sizeof (*cmd));
  cmd->plain.message.header.request.magic= PROTOCOL_BINARY_REQ;
  cmd->plain.message.header.request.opcode= cc;
  cmd->plain.message.header.request.keylen= (uint16_t)keylen;
  cmd->plain.message.header.request.bodylen= (uint32_t)(keylen + dtalen);
  cmd->plain.message.header.request.opaque= 0xdeadbeef;

  off_t key_offset= sizeof (protocol_binary_request_no_extras);

  if (key != NULL)
    memcpy(cmd->bytes + key_offset, key, keylen);

  if (dta != NULL)
    memcpy(cmd->bytes + key_offset + keylen, dta, dtalen);
}

/**
 * Create the flush command
 * @param cmd destination buffer
 * @param cc the command to create (FLUSH/FLUSHQ)
 * @param exptime when to flush
 * @param use_extra to force using of the extra field?
 */
static void flush_command(command *cmd,
                          uint8_t cc, uint32_t exptime, bool use_extra)
{
  memset(cmd, 0, sizeof (cmd->flush));
  cmd->flush.message.header.request.magic= PROTOCOL_BINARY_REQ;
  cmd->flush.message.header.request.opcode= cc;
  cmd->flush.message.header.request.opaque= 0xdeadbeef;

  if (exptime != 0 || use_extra)
  {
    cmd->flush.message.header.request.extlen= 4;
    cmd->flush.message.body.expiration= htonl(exptime);
    cmd->flush.message.header.request.bodylen= 4;
  }
}

/**
 * Create a incr/decr command
 * @param cc the cmd to create (FLUSH/FLUSHQ)
 * @param key the key to operate on
 * @param keylen the number of bytes in the key
 * @param delta the number to add/subtract
 * @param initial the initial value if the key doesn't exist
 * @param exptime when the key should expire if it isn't set
 */
static void arithmetic_command(command *cmd,
                               uint8_t cc,
                               const void* key,
                               size_t keylen,
                               uint64_t delta,
                               uint64_t initial,
                               uint32_t exptime)
{
  memset(cmd, 0, sizeof (cmd->incr));
  cmd->incr.message.header.request.magic= PROTOCOL_BINARY_REQ;
  cmd->incr.message.header.request.opcode= cc;
  cmd->incr.message.header.request.keylen= (uint16_t)keylen;
  cmd->incr.message.header.request.extlen= 20;
  cmd->incr.message.header.request.bodylen= (uint32_t)(keylen + 20);
  cmd->incr.message.header.request.opaque= 0xdeadbeef;
  cmd->incr.message.body.delta= htonll(delta);
  cmd->incr.message.body.initial= htonll(initial);
  cmd->incr.message.body.expiration= htonl(exptime);

  off_t key_offset= sizeof (protocol_binary_request_no_extras) + 20;
  memcpy(cmd->bytes + key_offset, key, keylen);
}

/**
 * Validate the response header from the server
 * @param rsp the response to check
 * @param cc the expected command
 * @param status the expected status
 */
static enum test_return do_validate_response_header(response *rsp,
                                                    uint8_t cc, uint16_t status)
{
  verify(rsp->plain.message.header.response.magic == PROTOCOL_BINARY_RES);
  verify(rsp->plain.message.header.response.opcode == cc);
  verify(rsp->plain.message.header.response.datatype == PROTOCOL_BINARY_RAW_BYTES);
  verify(rsp->plain.message.header.response.status == status);
  verify(rsp->plain.message.header.response.opaque == 0xdeadbeef);

  if (status == PROTOCOL_BINARY_RESPONSE_SUCCESS)
  {
    switch (cc) {
    case PROTOCOL_BINARY_CMD_ADDQ:
    case PROTOCOL_BINARY_CMD_APPENDQ:
    case PROTOCOL_BINARY_CMD_DECREMENTQ:
    case PROTOCOL_BINARY_CMD_DELETEQ:
    case PROTOCOL_BINARY_CMD_FLUSHQ:
    case PROTOCOL_BINARY_CMD_INCREMENTQ:
    case PROTOCOL_BINARY_CMD_PREPENDQ:
    case PROTOCOL_BINARY_CMD_QUITQ:
    case PROTOCOL_BINARY_CMD_REPLACEQ:
    case PROTOCOL_BINARY_CMD_SETQ:
      verify("Quiet command shouldn't return on success" == NULL);
    default:
      break;
    }

    switch (cc) {
    case PROTOCOL_BINARY_CMD_ADD:
    case PROTOCOL_BINARY_CMD_REPLACE:
    case PROTOCOL_BINARY_CMD_SET:
    case PROTOCOL_BINARY_CMD_APPEND:
    case PROTOCOL_BINARY_CMD_PREPEND:
      verify(rsp->plain.message.header.response.keylen == 0);
      verify(rsp->plain.message.header.response.extlen == 0);
      verify(rsp->plain.message.header.response.bodylen == 0);
      verify(rsp->plain.message.header.response.cas != 0);
      break;
    case PROTOCOL_BINARY_CMD_FLUSH:
    case PROTOCOL_BINARY_CMD_NOOP:
    case PROTOCOL_BINARY_CMD_QUIT:
    case PROTOCOL_BINARY_CMD_DELETE:
      verify(rsp->plain.message.header.response.keylen == 0);
      verify(rsp->plain.message.header.response.extlen == 0);
      verify(rsp->plain.message.header.response.bodylen == 0);
      verify(rsp->plain.message.header.response.cas == 0);
      break;

    case PROTOCOL_BINARY_CMD_DECREMENT:
    case PROTOCOL_BINARY_CMD_INCREMENT:
      verify(rsp->plain.message.header.response.keylen == 0);
      verify(rsp->plain.message.header.response.extlen == 0);
      verify(rsp->plain.message.header.response.bodylen == 8);
      verify(rsp->plain.message.header.response.cas != 0);
      break;

    case PROTOCOL_BINARY_CMD_STAT:
      verify(rsp->plain.message.header.response.extlen == 0);
      /* key and value exists in all packets except in the terminating */
      verify(rsp->plain.message.header.response.cas == 0);
      break;

    case PROTOCOL_BINARY_CMD_VERSION:
      verify(rsp->plain.message.header.response.keylen == 0);
      verify(rsp->plain.message.header.response.extlen == 0);
      verify(rsp->plain.message.header.response.bodylen != 0);
      verify(rsp->plain.message.header.response.cas == 0);
      break;

    case PROTOCOL_BINARY_CMD_GET:
    case PROTOCOL_BINARY_CMD_GETQ:
      verify(rsp->plain.message.header.response.keylen == 0);
      verify(rsp->plain.message.header.response.extlen == 4);
      verify(rsp->plain.message.header.response.cas != 0);
      break;

    case PROTOCOL_BINARY_CMD_GETK:
    case PROTOCOL_BINARY_CMD_GETKQ:
      verify(rsp->plain.message.header.response.keylen != 0);
      verify(rsp->plain.message.header.response.extlen == 4);
      verify(rsp->plain.message.header.response.cas != 0);
      break;

    default:
      /* Undefined command code */
      break;
    }
  }
  else
  {
    verify(rsp->plain.message.header.response.cas == 0);
    verify(rsp->plain.message.header.response.extlen == 0);
    if (cc != PROTOCOL_BINARY_CMD_GETK)
    {
      verify(rsp->plain.message.header.response.keylen == 0);
    }
  }

  return TEST_PASS;
}

/* We call verify(validate_response_header), but that macro
 * expects a boolean expression, and the function returns
 * an enum.... Let's just create a macro to avoid cluttering
 * the code with all of the == TEST_PASS ;-)
 */
#define validate_response_header(a,b,c) \
        do_validate_response_header(a,b,c) == TEST_PASS


static enum test_return send_binary_noop(void)
{
  command cmd;
  raw_command(&cmd, PROTOCOL_BINARY_CMD_NOOP, NULL, 0, NULL, 0);
  execute(send_packet(&cmd));
  return TEST_PASS;
}

static enum test_return receive_binary_noop(void)
{
  response rsp;
  execute(recv_packet(&rsp));
  verify(validate_response_header(&rsp, PROTOCOL_BINARY_CMD_NOOP,
                                  PROTOCOL_BINARY_RESPONSE_SUCCESS));
  return TEST_PASS;
}

static enum test_return test_binary_noop(void)
{
  execute(send_binary_noop());
  execute(receive_binary_noop());
  return TEST_PASS;
}

static enum test_return test_binary_quit_impl(uint8_t cc)
{
  command cmd;
  response rsp;
  raw_command(&cmd, cc, NULL, 0, NULL, 0);

  execute(send_packet(&cmd));
  if (cc == PROTOCOL_BINARY_CMD_QUIT)
  {
    execute(recv_packet(&rsp));
    verify(validate_response_header(&rsp, PROTOCOL_BINARY_CMD_QUIT,
                                    PROTOCOL_BINARY_RESPONSE_SUCCESS));
  }

  /* Socket should be closed now, read should return 0 */
  verify(timeout_io_op(sock, POLLIN, rsp.bytes, sizeof(rsp.bytes)) == 0);

  return TEST_PASS_RECONNECT;
}

static enum test_return test_binary_quit(void)
{
  return test_binary_quit_impl(PROTOCOL_BINARY_CMD_QUIT);
}

static enum test_return test_binary_quitq(void)
{
  return test_binary_quit_impl(PROTOCOL_BINARY_CMD_QUITQ);
}

static enum test_return test_binary_set_impl(const char* key, uint8_t cc)
{
  command cmd;
  response rsp;

  uint64_t value= 0xdeadbeefdeadcafe;
  storage_command(&cmd, cc, key, strlen(key), &value, sizeof (value), 0, 0);

  /* set should always work */
  for (int ii= 0; ii < 10; ii++)
  {
    if (ii == 0)
      execute(send_packet(&cmd));
    else
      execute(resend_packet(&cmd));

    if (cc == PROTOCOL_BINARY_CMD_SET)
    {
      execute(recv_packet(&rsp));
      verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_SUCCESS));
    }
    else
      execute(test_binary_noop());
  }

  /*
   * We need to get the current CAS id, and at this time we haven't
   * verified that we have a working get
   */
  if (cc == PROTOCOL_BINARY_CMD_SETQ)
  {
    cmd.set.message.header.request.opcode= PROTOCOL_BINARY_CMD_SET;
    execute(resend_packet(&cmd));
    execute(recv_packet(&rsp));
    verify(validate_response_header(&rsp, PROTOCOL_BINARY_CMD_SET,
                                    PROTOCOL_BINARY_RESPONSE_SUCCESS));
    cmd.set.message.header.request.opcode= PROTOCOL_BINARY_CMD_SETQ;
  }

  /* try to set with the correct CAS value */
  cmd.plain.message.header.request.cas=
          htonll(rsp.plain.message.header.response.cas);
  execute(resend_packet(&cmd));
  if (cc == PROTOCOL_BINARY_CMD_SET)
  {
    execute(recv_packet(&rsp));
    verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_SUCCESS));
  }
  else
    execute(test_binary_noop());

  /* try to set with an incorrect CAS value */
  cmd.plain.message.header.request.cas=
          htonll(rsp.plain.message.header.response.cas - 1);
  execute(resend_packet(&cmd));
  execute(send_binary_noop());
  execute(recv_packet(&rsp));
  verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS));
  execute(receive_binary_noop());

  return TEST_PASS;
}

static enum test_return test_binary_set(void)
{
  return test_binary_set_impl("test_binary_set", PROTOCOL_BINARY_CMD_SET);
}

static enum test_return test_binary_setq(void)
{
  return test_binary_set_impl("test_binary_setq", PROTOCOL_BINARY_CMD_SETQ);
}

static enum test_return test_binary_add_impl(const char* key, uint8_t cc)
{
  command cmd;
  response rsp;
  uint64_t value= 0xdeadbeefdeadcafe;
  storage_command(&cmd, cc, key, strlen(key), &value, sizeof (value), 0, 0);

  /* first add should work, rest of them should fail (even with cas
     as wildcard */
  for (int ii=0; ii < 10; ii++)
  {
    if (ii == 0)
      execute(send_packet(&cmd));
    else
      execute(resend_packet(&cmd));

    if (cc == PROTOCOL_BINARY_CMD_ADD || ii > 0)
    {
      uint16_t expected_result;
      if (ii == 0)
        expected_result= PROTOCOL_BINARY_RESPONSE_SUCCESS;
      else
        expected_result= PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS;

      execute(send_binary_noop());
      execute(recv_packet(&rsp));
      execute(receive_binary_noop());
      verify(validate_response_header(&rsp, cc, expected_result));
    }
    else
      execute(test_binary_noop());
  }

  return TEST_PASS;
}

static enum test_return test_binary_add(void)
{
  return test_binary_add_impl("test_binary_add", PROTOCOL_BINARY_CMD_ADD);
}

static enum test_return test_binary_addq(void)
{
  return test_binary_add_impl("test_binary_addq", PROTOCOL_BINARY_CMD_ADDQ);
}

static enum test_return binary_set_item(const char *key, const char *value)
{
  command cmd;
  response rsp;
  storage_command(&cmd, PROTOCOL_BINARY_CMD_SET, key, strlen(key),
                  value, strlen(value), 0, 0);
  execute(send_packet(&cmd));
  execute(recv_packet(&rsp));
  verify(validate_response_header(&rsp, PROTOCOL_BINARY_CMD_SET,
                                  PROTOCOL_BINARY_RESPONSE_SUCCESS));
  return TEST_PASS;
}

static enum test_return test_binary_replace_impl(const char* key, uint8_t cc)
{
  command cmd;
  response rsp;
  uint64_t value= 0xdeadbeefdeadcafe;
  storage_command(&cmd, cc, key, strlen(key), &value, sizeof (value), 0, 0);

  /* first replace should fail, successive should succeed (when the
     item is added! */
  for (int ii= 0; ii < 10; ii++)
  {
    if (ii == 0)
      execute(send_packet(&cmd));
    else
      execute(resend_packet(&cmd));

    if (cc == PROTOCOL_BINARY_CMD_REPLACE || ii == 0)
    {
      uint16_t expected_result;
      if (ii == 0)
        expected_result=PROTOCOL_BINARY_RESPONSE_KEY_ENOENT;
      else
        expected_result=PROTOCOL_BINARY_RESPONSE_SUCCESS;

      execute(send_binary_noop());
      execute(recv_packet(&rsp));
      execute(receive_binary_noop());
      verify(validate_response_header(&rsp, cc, expected_result));

      if (ii == 0)
        execute(binary_set_item(key, key));
    }
    else
      execute(test_binary_noop());
  }

  /* verify that replace with CAS value works! */
  cmd.plain.message.header.request.cas=
          htonll(rsp.plain.message.header.response.cas);
  execute(resend_packet(&cmd));

  if (cc == PROTOCOL_BINARY_CMD_REPLACE)
  {
    execute(recv_packet(&rsp));
    verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_SUCCESS));
  }
  else
    execute(test_binary_noop());

  /* try to set with an incorrect CAS value */
  cmd.plain.message.header.request.cas=
          htonll(rsp.plain.message.header.response.cas - 1);
  execute(resend_packet(&cmd));
  execute(send_binary_noop());
  execute(recv_packet(&rsp));
  execute(receive_binary_noop());
  verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS));

  return TEST_PASS;
}

static enum test_return test_binary_replace(void)
{
  return test_binary_replace_impl("test_binary_replace", PROTOCOL_BINARY_CMD_REPLACE);
}

static enum test_return test_binary_replaceq(void)
{
  return test_binary_replace_impl("test_binary_replaceq", PROTOCOL_BINARY_CMD_REPLACEQ);
}

static enum test_return test_binary_delete_impl(const char *key, uint8_t cc)
{
  command cmd;
  response rsp;
  raw_command(&cmd, cc, key, strlen(key), NULL, 0);

  /* The delete shouldn't work the first time, because the item isn't there */
  execute(send_packet(&cmd));
  execute(send_binary_noop());
  execute(recv_packet(&rsp));
  verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_KEY_ENOENT));
  execute(receive_binary_noop());
  execute(binary_set_item(key, key));

  /* The item should be present now, resend*/
  execute(resend_packet(&cmd));
  if (cc == PROTOCOL_BINARY_CMD_DELETE)
  {
    execute(recv_packet(&rsp));
    verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_SUCCESS));
  }

  execute(test_binary_noop());

  return TEST_PASS;
}

static enum test_return test_binary_delete(void)
{
  return test_binary_delete_impl("test_binary_delete", PROTOCOL_BINARY_CMD_DELETE);
}

static enum test_return test_binary_deleteq(void)
{
  return test_binary_delete_impl("test_binary_deleteq", PROTOCOL_BINARY_CMD_DELETEQ);
}

static enum test_return test_binary_get_impl(const char *key, uint8_t cc)
{
  command cmd;
  response rsp;

  raw_command(&cmd, cc, key, strlen(key), NULL, 0);
  execute(send_packet(&cmd));
  execute(send_binary_noop());

  if (cc == PROTOCOL_BINARY_CMD_GET || cc == PROTOCOL_BINARY_CMD_GETK)
  {
    execute(recv_packet(&rsp));
    verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_KEY_ENOENT));
  }

  execute(receive_binary_noop());

  execute(binary_set_item(key, key));
  execute(resend_packet(&cmd));
  execute(send_binary_noop());

  execute(recv_packet(&rsp));
  verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_SUCCESS));
  execute(receive_binary_noop());

  return TEST_PASS;
}

static enum test_return test_binary_get(void)
{
  return test_binary_get_impl("test_binary_get", PROTOCOL_BINARY_CMD_GET);
}

static enum test_return test_binary_getk(void)
{
  return test_binary_get_impl("test_binary_getk", PROTOCOL_BINARY_CMD_GETK);
}

static enum test_return test_binary_getq(void)
{
  return test_binary_get_impl("test_binary_getq", PROTOCOL_BINARY_CMD_GETQ);
}

static enum test_return test_binary_getkq(void)
{
  return test_binary_get_impl("test_binary_getkq", PROTOCOL_BINARY_CMD_GETKQ);
}

static enum test_return test_binary_incr_impl(const char* key, uint8_t cc)
{
  command cmd;
  response rsp;
  arithmetic_command(&cmd, cc, key, strlen(key), 1, 0, 0);

  uint64_t ii;
  for (ii= 0; ii < 10; ++ii)
  {
    if (ii == 0)
      execute(send_packet(&cmd));
    else
      execute(resend_packet(&cmd));

    if (cc == PROTOCOL_BINARY_CMD_INCREMENT)
    {
      execute(recv_packet(&rsp));
      verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_SUCCESS));
      verify(ntohll(rsp.incr.message.body.value) == ii);
    }
    else
      execute(test_binary_noop());
  }

  /* @todo add incorrect CAS */
  return TEST_PASS;
}

static enum test_return test_binary_incr(void)
{
  return test_binary_incr_impl("test_binary_incr", PROTOCOL_BINARY_CMD_INCREMENT);
}

static enum test_return test_binary_incrq(void)
{
  return test_binary_incr_impl("test_binary_incrq", PROTOCOL_BINARY_CMD_INCREMENTQ);
}

static enum test_return test_binary_decr_impl(const char* key, uint8_t cc)
{
  command cmd;
  response rsp;
  arithmetic_command(&cmd, cc, key, strlen(key), 1, 9, 0);

  int ii;
  for (ii= 9; ii > -1; --ii)
  {
    if (ii == 9)
      execute(send_packet(&cmd));
    else
      execute(resend_packet(&cmd));

    if (cc == PROTOCOL_BINARY_CMD_DECREMENT)
    {
      execute(recv_packet(&rsp));
      verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_SUCCESS));
      verify(ntohll(rsp.decr.message.body.value) == (uint64_t)ii);
    }
    else
      execute(test_binary_noop());
  }

  /* decr 0 should not wrap */
  execute(resend_packet(&cmd));
  if (cc == PROTOCOL_BINARY_CMD_DECREMENT)
  {
    execute(recv_packet(&rsp));
    verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_SUCCESS));
    verify(ntohll(rsp.decr.message.body.value) == 0);
  }
  else
  {
    /* @todo get the value and verify! */

  }

  /* @todo add incorrect cas */
  execute(test_binary_noop());
  return TEST_PASS;
}

static enum test_return test_binary_decr(void)
{
  return test_binary_decr_impl("test_binary_decr",
                               PROTOCOL_BINARY_CMD_DECREMENT);
}

static enum test_return test_binary_decrq(void)
{
  return test_binary_decr_impl("test_binary_decrq",
                               PROTOCOL_BINARY_CMD_DECREMENTQ);
}

static enum test_return test_binary_version(void)
{
  command cmd;
  response rsp;
  raw_command(&cmd, PROTOCOL_BINARY_CMD_VERSION, NULL, 0, NULL, 0);

  execute(send_packet(&cmd));
  execute(recv_packet(&rsp));
  verify(validate_response_header(&rsp, PROTOCOL_BINARY_CMD_VERSION,
                                  PROTOCOL_BINARY_RESPONSE_SUCCESS));

  return TEST_PASS;
}

static enum test_return test_binary_flush_impl(const char *key, uint8_t cc)
{
  command cmd;
  response rsp;

  for (int ii= 0; ii < 2; ++ii)
  {
    execute(binary_set_item(key, key));
    flush_command(&cmd, cc, 0, ii == 0);
    execute(send_packet(&cmd));

    if (cc == PROTOCOL_BINARY_CMD_FLUSH)
    {
      execute(recv_packet(&rsp));
      verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_SUCCESS));
    }
    else
      execute(test_binary_noop());

    raw_command(&cmd, PROTOCOL_BINARY_CMD_GET, key, strlen(key), NULL, 0);
    execute(send_packet(&cmd));
    execute(recv_packet(&rsp));
    verify(validate_response_header(&rsp, PROTOCOL_BINARY_CMD_GET,
                                    PROTOCOL_BINARY_RESPONSE_KEY_ENOENT));
  }

  return TEST_PASS;
}

static enum test_return test_binary_flush(void)
{
  return test_binary_flush_impl("test_binary_flush", PROTOCOL_BINARY_CMD_FLUSH);
}

static enum test_return test_binary_flushq(void)
{
  return test_binary_flush_impl("test_binary_flushq", PROTOCOL_BINARY_CMD_FLUSHQ);
}

static enum test_return test_binary_concat_impl(const char *key, uint8_t cc)
{
  command cmd;
  response rsp;
  const char *value;

  if (cc == PROTOCOL_BINARY_CMD_APPEND || cc == PROTOCOL_BINARY_CMD_APPENDQ)
    value="hello";
  else
    value=" world";

  execute(binary_set_item(key, value));

  if (cc == PROTOCOL_BINARY_CMD_APPEND || cc == PROTOCOL_BINARY_CMD_APPENDQ)
    value=" world";
  else
    value="hello";

  raw_command(&cmd, cc, key, strlen(key), value, strlen(value));
  execute(send_packet(&cmd));
  if (cc == PROTOCOL_BINARY_CMD_APPEND || cc == PROTOCOL_BINARY_CMD_PREPEND)
  {
    execute(recv_packet(&rsp));
    verify(validate_response_header(&rsp, cc, PROTOCOL_BINARY_RESPONSE_SUCCESS));
  }
  else
    execute(test_binary_noop());

  raw_command(&cmd, PROTOCOL_BINARY_CMD_GET, key, strlen(key), NULL, 0);
  execute(send_packet(&cmd));
  execute(recv_packet(&rsp));
  verify(validate_response_header(&rsp, PROTOCOL_BINARY_CMD_GET,
                                  PROTOCOL_BINARY_RESPONSE_SUCCESS));
  verify(rsp.plain.message.header.response.bodylen - 4 == 11);
  verify(memcmp(rsp.bytes + 28, "hello world", 11) == 0);

  return TEST_PASS;
}

static enum test_return test_binary_append(void)
{
  return test_binary_concat_impl("test_binary_append", PROTOCOL_BINARY_CMD_APPEND);
}

static enum test_return test_binary_prepend(void)
{
  return test_binary_concat_impl("test_binary_prepend", PROTOCOL_BINARY_CMD_PREPEND);
}

static enum test_return test_binary_appendq(void)
{
  return test_binary_concat_impl("test_binary_appendq", PROTOCOL_BINARY_CMD_APPENDQ);
}

static enum test_return test_binary_prependq(void)
{
  return test_binary_concat_impl("test_binary_prependq", PROTOCOL_BINARY_CMD_PREPENDQ);
}

static enum test_return test_binary_stat(void)
{
  command cmd;
  response rsp;

  raw_command(&cmd, PROTOCOL_BINARY_CMD_STAT, NULL, 0, NULL, 0);
  execute(send_packet(&cmd));

  do
  {
    execute(recv_packet(&rsp));
    verify(validate_response_header(&rsp, PROTOCOL_BINARY_CMD_STAT,
                                    PROTOCOL_BINARY_RESPONSE_SUCCESS));
  } while (rsp.plain.message.header.response.keylen != 0);

  return TEST_PASS;
}

static enum test_return send_string(const char *cmd)
{
  execute(retry_write(cmd, strlen(cmd)));
  return TEST_PASS;
}

static enum test_return receive_line(char *buffer, size_t size)
{
  size_t offset= 0;
  while (offset < size)
  {
    execute(retry_read(buffer + offset, 1));
    if (buffer[offset] == '\n')
    {
      if (offset + 1 < size)
      {
        buffer[offset + 1]= '\0';
        return TEST_PASS;
      }
      else
        return TEST_FAIL;
    }
    ++offset;
  }

  return TEST_FAIL;
}

static enum test_return receive_response(const char *msg) {
  char buffer[80];
  execute(receive_line(buffer, sizeof(buffer)));
  if (strcmp(msg, buffer) != 0) {
      fprintf(stderr, "[%s]\n", buffer);
  }
  verify(strcmp(msg, buffer) == 0);
  return TEST_PASS;
}

static enum test_return receive_error_response(void)
{
  char buffer[80];
  execute(receive_line(buffer, sizeof(buffer)));
  verify(strncmp(buffer, "ERROR", 5) == 0 ||
         strncmp(buffer, "CLIENT_ERROR", 12) == 0 ||
         strncmp(buffer, "SERVER_ERROR", 12) == 0);
  return TEST_PASS;
}

static enum test_return test_ascii_quit(void)
{
  /* Verify that quit handles unknown options */
  execute(send_string("quit foo bar\r\n"));
  execute(receive_error_response());

  /* quit doesn't support noreply */
  execute(send_string("quit noreply\r\n"));
  execute(receive_error_response());

  /* Verify that quit works */
  execute(send_string("quit\r\n"));

  /* Socket should be closed now, read should return 0 */
  char buffer[80];
  verify(timeout_io_op(sock, POLLIN, buffer, sizeof(buffer)) == 0);
  return TEST_PASS_RECONNECT;

}

static enum test_return test_ascii_version(void)
{
  /* Verify that version command handles unknown options */
  execute(send_string("version foo bar\r\n"));
  execute(receive_error_response());

  /* version doesn't support noreply */
  execute(send_string("version noreply\r\n"));
  execute(receive_error_response());

  /* Verify that verify works */
  execute(send_string("version\r\n"));
  char buffer[256];
  execute(receive_line(buffer, sizeof(buffer)));
  verify(strncmp(buffer, "VERSION ", 8) == 0);

  return TEST_PASS;
}

static enum test_return test_ascii_verbosity(void)
{
  /* This command does not adhere to the spec! */
  execute(send_string("verbosity foo bar my\r\n"));
  execute(receive_error_response());

  execute(send_string("verbosity noreply\r\n"));
  execute(receive_error_response());

  execute(send_string("verbosity 0 noreply\r\n"));
  execute(test_ascii_version());

  execute(send_string("verbosity\r\n"));
  execute(receive_error_response());

  execute(send_string("verbosity 1\r\n"));
  execute(receive_response("OK\r\n"));

  execute(send_string("verbosity 0\r\n"));
  execute(receive_response("OK\r\n"));

  return TEST_PASS;
}



static enum test_return test_ascii_set_impl(const char* key, bool noreply)
{
  /* @todo add tests for bogus format! */
  char buffer[1024];
  sprintf(buffer, "set %s 0 0 5%s\r\nvalue\r\n", key,
          noreply ? " noreply" : "");
  execute(send_string(buffer));

  if (!noreply)
    execute(receive_response("STORED\r\n"));

  return test_ascii_version();
}

static enum test_return test_ascii_set(void)
{
  return test_ascii_set_impl("test_ascii_set", false);
}

static enum test_return test_ascii_set_noreply(void)
{
  return test_ascii_set_impl("test_ascii_set_noreply", true);
}

static enum test_return test_ascii_add_impl(const char* key, bool noreply)
{
  /* @todo add tests for bogus format! */
  char buffer[1024];
  sprintf(buffer, "add %s 0 0 5%s\r\nvalue\r\n", key,
          noreply ? " noreply" : "");
  execute(send_string(buffer));

  if (!noreply)
    execute(receive_response("STORED\r\n"));

  execute(send_string(buffer));

  if (!noreply)
    execute(receive_response("NOT_STORED\r\n"));

  return test_ascii_version();
}

static enum test_return test_ascii_add(void)
{
  return test_ascii_add_impl("test_ascii_add", false);
}

static enum test_return test_ascii_add_noreply(void)
{
  return test_ascii_add_impl("test_ascii_add_noreply", true);
}

static enum test_return ascii_get_value(const char *key, const char *value)
{

  char buffer[1024];
  size_t datasize= strlen(value);

  verify(datasize < sizeof(buffer));
  execute(receive_line(buffer, sizeof(buffer)));
  verify(strncmp(buffer, "VALUE ", 6) == 0);
  verify(strncmp(buffer + 6, key, strlen(key)) == 0);
  char *ptr= buffer + 6 + strlen(key) + 1;
  char *end;

  unsigned long val= strtoul(ptr, &end, 10); /* flags */
  verify(ptr != end);
  verify(val == 0);
  verify(end != NULL);
  val= strtoul(end, &end, 10); /* size */
  verify(ptr != end);
  verify(val == datasize);
  verify(end != NULL);
  while (*end != '\n' && isspace(*end))
    ++end;
  verify(*end == '\n');

  execute(retry_read(buffer, datasize));
  verify(memcmp(buffer, value, datasize) == 0);

  execute(retry_read(buffer, 2));
  verify(memcmp(buffer, "\r\n", 2) == 0);

  return TEST_PASS;
}

static enum test_return ascii_get_item(const char *key, const char *value,
                                       bool exist)
{
  char buffer[1024];
  size_t datasize= 0;
  if (value != NULL)
    datasize= strlen(value);

  verify(datasize < sizeof(buffer));
  sprintf(buffer, "get %s\r\n", key);
  execute(send_string(buffer));

  if (exist)
    execute(ascii_get_value(key, value));

  execute(retry_read(buffer, 5));
  verify(memcmp(buffer, "END\r\n", 5) == 0);

  return TEST_PASS;
}

static enum test_return ascii_gets_value(const char *key, const char *value,
                                         unsigned long *cas)
{

  char buffer[1024];
  size_t datasize= strlen(value);

  verify(datasize < sizeof(buffer));
  execute(receive_line(buffer, sizeof(buffer)));
  verify(strncmp(buffer, "VALUE ", 6) == 0);
  verify(strncmp(buffer + 6, key, strlen(key)) == 0);
  char *ptr= buffer + 6 + strlen(key) + 1;
  char *end;

  unsigned long val= strtoul(ptr, &end, 10); /* flags */
  verify(ptr != end);
  verify(val == 0);
  verify(end != NULL);
  val= strtoul(end, &end, 10); /* size */
  verify(ptr != end);
  verify(val == datasize);
  verify(end != NULL);
  *cas= strtoul(end, &end, 10); /* cas */
  verify(ptr != end);
  verify(val == datasize);
  verify(end != NULL);

  while (*end != '\n' && isspace(*end))
    ++end;
  verify(*end == '\n');

  execute(retry_read(buffer, datasize));
  verify(memcmp(buffer, value, datasize) == 0);

  execute(retry_read(buffer, 2));
  verify(memcmp(buffer, "\r\n", 2) == 0);

  return TEST_PASS;
}

static enum test_return ascii_gets_item(const char *key, const char *value,
                                        bool exist, unsigned long *cas)
{
  char buffer[1024];
  size_t datasize= 0;
  if (value != NULL)
    datasize= strlen(value);

  verify(datasize < sizeof(buffer));
  sprintf(buffer, "gets %s\r\n", key);
  execute(send_string(buffer));

  if (exist)
    execute(ascii_gets_value(key, value, cas));

  execute(retry_read(buffer, 5));
  verify(memcmp(buffer, "END\r\n", 5) == 0);

  return TEST_PASS;
}

static enum test_return ascii_set_item(const char *key, const char *value)
{
  char buffer[300];
  size_t len= strlen(value);
  sprintf(buffer, "set %s 0 0 %u\r\n", key, (unsigned int)len);
  execute(send_string(buffer));
  execute(retry_write(value, len));
  execute(send_string("\r\n"));
  execute(receive_response("STORED\r\n"));
  return TEST_PASS;
}

static enum test_return test_ascii_replace_impl(const char* key, bool noreply)
{
  char buffer[1024];
  sprintf(buffer, "replace %s 0 0 5%s\r\nvalue\r\n", key,
          noreply ? " noreply" : "");
  execute(send_string(buffer));

  if (noreply)
    execute(test_ascii_version());
  else
    execute(receive_response("NOT_STORED\r\n"));

  execute(ascii_set_item(key, "value"));
  execute(ascii_get_item(key, "value", true));


  execute(send_string(buffer));

  if (noreply)
    execute(test_ascii_version());
  else
    execute(receive_response("STORED\r\n"));

  return test_ascii_version();
}

static enum test_return test_ascii_replace(void)
{
  return test_ascii_replace_impl("test_ascii_replace", false);
}

static enum test_return test_ascii_replace_noreply(void)
{
  return test_ascii_replace_impl("test_ascii_replace_noreply", true);
}

static enum test_return test_ascii_cas_impl(const char* key, bool noreply)
{
  char buffer[1024];
  unsigned long cas;

  execute(ascii_set_item(key, "value"));
  execute(ascii_gets_item(key, "value", true, &cas));

  sprintf(buffer, "cas %s 0 0 6 %lu%s\r\nvalue2\r\n", key, cas,
          noreply ? " noreply" : "");
  execute(send_string(buffer));

  if (noreply)
    execute(test_ascii_version());
  else
    execute(receive_response("STORED\r\n"));

  /* reexecute the same command should fail due to illegal cas */
  execute(send_string(buffer));

  if (noreply)
    execute(test_ascii_version());
  else
    execute(receive_response("EXISTS\r\n"));

  return test_ascii_version();
}

static enum test_return test_ascii_cas(void)
{
  return test_ascii_cas_impl("test_ascii_cas", false);
}

static enum test_return test_ascii_cas_noreply(void)
{
  return test_ascii_cas_impl("test_ascii_cas_noreply", true);
}

static enum test_return test_ascii_delete_impl(const char *key, bool noreply)
{
  execute(ascii_set_item(key, "value"));

  execute(send_string("delete\r\n"));
  execute(receive_error_response());
  /* BUG: the server accepts delete a b */
  execute(send_string("delete a b c d e\r\n"));
  execute(receive_error_response());

  char buffer[1024];
  sprintf(buffer, "delete %s%s\r\n", key, noreply ? " noreply" : "");
  execute(send_string(buffer));

  if (noreply)
    execute(test_ascii_version());
  else
    execute(receive_response("DELETED\r\n"));

  execute(ascii_get_item(key, "value", false));
  execute(send_string(buffer));
  if (noreply)
    execute(test_ascii_version());
  else
    execute(receive_response("NOT_FOUND\r\n"));

  return TEST_PASS;
}

static enum test_return test_ascii_delete(void)
{
  return test_ascii_delete_impl("test_ascii_delete", false);
}

static enum test_return test_ascii_delete_noreply(void)
{
  return test_ascii_delete_impl("test_ascii_delete_noreply", true);
}

static enum test_return test_ascii_get(void)
{
  execute(ascii_set_item("test_ascii_get", "value"));

  execute(send_string("get\r\n"));
  execute(receive_error_response());
  execute(ascii_get_item("test_ascii_get", "value", true));
  execute(ascii_get_item("test_ascii_get_notfound", "value", false));

  return TEST_PASS;
}

static enum test_return test_ascii_gets(void)
{
  execute(ascii_set_item("test_ascii_gets", "value"));

  execute(send_string("gets\r\n"));
  execute(receive_error_response());
  unsigned long cas;
  execute(ascii_gets_item("test_ascii_gets", "value", true, &cas));
  execute(ascii_gets_item("test_ascii_gets_notfound", "value", false, &cas));

  return TEST_PASS;
}

static enum test_return test_ascii_mget(void)
{
  execute(ascii_set_item("test_ascii_mget1", "value"));
  execute(ascii_set_item("test_ascii_mget2", "value"));
  execute(ascii_set_item("test_ascii_mget3", "value"));
  execute(ascii_set_item("test_ascii_mget4", "value"));
  execute(ascii_set_item("test_ascii_mget5", "value"));

  execute(send_string("get test_ascii_mget1 test_ascii_mget2 test_ascii_mget3 "
                      "test_ascii_mget4 test_ascii_mget5 "
                      "test_ascii_mget6\r\n"));
  execute(ascii_get_value("test_ascii_mget1", "value"));
  execute(ascii_get_value("test_ascii_mget2", "value"));
  execute(ascii_get_value("test_ascii_mget3", "value"));
  execute(ascii_get_value("test_ascii_mget4", "value"));
  execute(ascii_get_value("test_ascii_mget5", "value"));

  char buffer[5];
  execute(retry_read(buffer, 5));
  verify(memcmp(buffer, "END\r\n", 5) == 0);
 return TEST_PASS;
}

static enum test_return test_ascii_incr_impl(const char* key, bool noreply)
{
  char cmd[300];
  sprintf(cmd, "incr %s 1%s\r\n", key, noreply ? " noreply" : "");

  execute(ascii_set_item(key, "0"));
  for (int x= 1; x < 11; ++x)
  {
    execute(send_string(cmd));

    if (noreply)
      execute(test_ascii_version());
    else
    {
      char buffer[80];
      execute(receive_line(buffer, sizeof(buffer)));
      int val= atoi(buffer);
      verify(val == x);
    }
  }

  execute(ascii_get_item(key, "10", true));

  return TEST_PASS;
}

static enum test_return test_ascii_incr(void)
{
  return test_ascii_incr_impl("test_ascii_incr", false);
}

static enum test_return test_ascii_incr_noreply(void)
{
  return test_ascii_incr_impl("test_ascii_incr_noreply", true);
}

static enum test_return test_ascii_decr_impl(const char* key, bool noreply)
{
  char cmd[300];
  sprintf(cmd, "decr %s 1%s\r\n", key, noreply ? " noreply" : "");

  execute(ascii_set_item(key, "9"));
  for (int x= 8; x > -1; --x)
  {
    execute(send_string(cmd));

    if (noreply)
      execute(test_ascii_version());
    else
    {
      char buffer[80];
      execute(receive_line(buffer, sizeof(buffer)));
      int val= atoi(buffer);
      verify(val == x);
    }
  }

  execute(ascii_get_item(key, "0", true));

  /* verify that it doesn't wrap */
  execute(send_string(cmd));
  if (noreply)
    execute(test_ascii_version());
  else
  {
    char buffer[80];
    execute(receive_line(buffer, sizeof(buffer)));
  }
  execute(ascii_get_item(key, "0", true));

  return TEST_PASS;
}

static enum test_return test_ascii_decr(void)
{
  return test_ascii_decr_impl("test_ascii_decr", false);
}

static enum test_return test_ascii_decr_noreply(void)
{
  return test_ascii_decr_impl("test_ascii_decr_noreply", true);
}


static enum test_return test_ascii_flush_impl(const char *key, bool noreply)
{
#if 0
  /* Verify that the flush_all command handles unknown options */
  /* Bug in the current memcached server! */
  execute(send_string("flush_all foo bar\r\n"));
  execute(receive_error_response());
#endif

  execute(ascii_set_item(key, key));
  execute(ascii_get_item(key, key, true));

  if (noreply)
  {
    execute(send_string("flush_all noreply\r\n"));
    execute(test_ascii_version());
  }
  else
  {
    execute(send_string("flush_all\r\n"));
    execute(receive_response("OK\r\n"));
  }

  execute(ascii_get_item(key, key, false));

  return TEST_PASS;
}

static enum test_return test_ascii_flush(void)
{
  return test_ascii_flush_impl("test_ascii_flush", false);
}

static enum test_return test_ascii_flush_noreply(void)
{
  return test_ascii_flush_impl("test_ascii_flush_noreply", true);
}

static enum test_return test_ascii_concat_impl(const char *key,
                                               bool append,
                                               bool noreply)
{
  const char *value;

  if (append)
    value="hello";
  else
    value=" world";

  execute(ascii_set_item(key, value));

  if (append)
    value=" world";
  else
    value="hello";

  char cmd[400];
  sprintf(cmd, "%s %s 0 0 %u%s\r\n%s\r\n",
          append ? "append" : "prepend",
          key, (unsigned int)strlen(value), noreply ? " noreply" : "",
          value);
  execute(send_string(cmd));

  if (noreply)
    execute(test_ascii_version());
  else
    execute(receive_response("STORED\r\n"));

  execute(ascii_get_item(key, "hello world", true));

  sprintf(cmd, "%s %s_notfound 0 0 %u%s\r\n%s\r\n",
          append ? "append" : "prepend",
          key, (unsigned int)strlen(value), noreply ? " noreply" : "",
          value);
  execute(send_string(cmd));

  if (noreply)
    execute(test_ascii_version());
  else
    execute(receive_response("NOT_STORED\r\n"));

  return TEST_PASS;
}

static enum test_return test_ascii_append(void)
{
  return test_ascii_concat_impl("test_ascii_append", true, false);
}

static enum test_return test_ascii_prepend(void)
{
  return test_ascii_concat_impl("test_ascii_prepend", false, false);
}

static enum test_return test_ascii_append_noreply(void)
{
  return test_ascii_concat_impl("test_ascii_append_noreply", true, true);
}

static enum test_return test_ascii_prepend_noreply(void)
{
  return test_ascii_concat_impl("test_ascii_prepend_noreply", false, true);
}

static enum test_return test_ascii_stat(void)
{
  execute(send_string("stats noreply\r\n"));
  execute(receive_error_response());
  execute(send_string("stats\r\n"));
  char buffer[1024];
  do {
    execute(receive_line(buffer, sizeof(buffer)));
  } while (strcmp(buffer, "END\r\n") != 0);

  return TEST_PASS_RECONNECT;
}

typedef enum test_return(*TEST_FUNC)(void);

struct testcase
{
  const char *description;
  TEST_FUNC function;
};

struct testcase testcases[]= {
  { "ascii quit", test_ascii_quit },
  { "ascii version", test_ascii_version },
  { "ascii verbosity", test_ascii_verbosity },
  { "ascii set", test_ascii_set },
  { "ascii set noreply", test_ascii_set_noreply },
  { "ascii get", test_ascii_get },
  { "ascii gets", test_ascii_gets },
  { "ascii mget", test_ascii_mget },
  { "ascii flush", test_ascii_flush },
  { "ascii flush noreply", test_ascii_flush_noreply },
  { "ascii add", test_ascii_add },
  { "ascii add noreply", test_ascii_add_noreply },
  { "ascii replace", test_ascii_replace },
  { "ascii replace noreply", test_ascii_replace_noreply },
  { "ascii cas", test_ascii_cas },
  { "ascii cas noreply", test_ascii_cas_noreply },
  { "ascii delete", test_ascii_delete },
  { "ascii delete noreply", test_ascii_delete_noreply },
  { "ascii incr", test_ascii_incr },
  { "ascii incr noreply", test_ascii_incr_noreply },
  { "ascii decr", test_ascii_decr },
  { "ascii decr noreply", test_ascii_decr_noreply },
  { "ascii append", test_ascii_append },
  { "ascii append noreply", test_ascii_append_noreply },
  { "ascii prepend", test_ascii_prepend },
  { "ascii prepend noreply", test_ascii_prepend_noreply },
  { "ascii stat", test_ascii_stat },
  { "binary noop", test_binary_noop },
  { "binary quit", test_binary_quit },
  { "binary quitq", test_binary_quitq },
  { "binary set", test_binary_set },
  { "binary setq", test_binary_setq },
  { "binary flush", test_binary_flush },
  { "binary flushq", test_binary_flushq },
  { "binary add", test_binary_add },
  { "binary addq", test_binary_addq },
  { "binary replace", test_binary_replace },
  { "binary replaceq", test_binary_replaceq },
  { "binary delete", test_binary_delete },
  { "binary deleteq", test_binary_deleteq },
  { "binary get", test_binary_get },
  { "binary getq", test_binary_getq },
  { "binary getk", test_binary_getk },
  { "binary getkq", test_binary_getkq },
  { "binary incr", test_binary_incr },
  { "binary incrq", test_binary_incrq },
  { "binary decr", test_binary_decr },
  { "binary decrq", test_binary_decrq },
  { "binary version", test_binary_version },
  { "binary append", test_binary_append },
  { "binary appendq", test_binary_appendq },
  { "binary prepend", test_binary_prepend },
  { "binary prependq", test_binary_prependq },
  { "binary stat", test_binary_stat },
  { NULL, NULL}
};

int main(int argc, char **argv)
{
  static const char * const status_msg[]= {"[skip]", "[pass]", "[pass]", "[FAIL]"};
  int total= 0;
  int failed= 0;
  const char *hostname= "localhost";
  const char *port= "11211";
  int cmd;
  bool prompt= false;
  const char *testname= NULL;

  while ((cmd= getopt(argc, argv, "t:vch:p:PT:?")) != EOF)
  {
    switch (cmd) {
    case 't':
      timeout= atoi(optarg);
      if (timeout == 0)
      {
        fprintf(stderr, "Invalid timeout. Please specify a number for -t\n");
        return 1;
      }
      break;
    case 'v': verbose= true;
      break;
    case 'c': do_core= true;
      break;
    case 'h': hostname= optarg;
      break;
    case 'p': port= optarg;
      break;
    case 'P': prompt= true;
      break;
    case 'T': testname= optarg;
       break;
    default:
      fprintf(stderr, "Usage: %s [-h hostname] [-p port] [-c] [-v] [-t n]"
              " [-P] [-T testname]'\n"
              "\t-c\tGenerate coredump if a test fails\n"
              "\t-v\tVerbose test output (print out the assertion)\n"
              "\t-t n\tSet the timeout for io-operations to n seconds\n"
              "\t-P\tPrompt the user before starting a test.\n"
              "\t\t\t\"skip\" will skip the test\n"
              "\t\t\t\"quit\" will terminate memcapable\n"
              "\t\t\tEverything else will start the test\n"
              "\t-T n\tJust run the test named n\n",
              argv[0]);
      return 1;
    }
  }

  initialize_sockets();
  sock= connect_server(hostname, port);
  if (sock == INVALID_SOCKET)
  {
    fprintf(stderr, "Failed to connect to <%s:%s>: %s\n",
            hostname, port, strerror(get_socket_errno()));
    return 1;
  }

  for (int ii= 0; testcases[ii].description != NULL; ++ii)
  {
    if (testname != NULL && strcmp(testcases[ii].description, testname) != 0)
       continue;

    ++total;
    fprintf(stdout, "%-40s", testcases[ii].description);
    fflush(stdout);

    if (prompt)
    {
      fprintf(stdout, "\nPress <return> when you are ready? ");
      char buffer[80] = {0};
      if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (strncmp(buffer, "skip", 4) == 0)
        {
          fprintf(stdout, "%-40s%s\n", testcases[ii].description,
                  status_msg[TEST_SKIP]);
          fflush(stdout);
          continue;
        }
        if (strncmp(buffer, "quit", 4) == 0)
          exit(0);
      }

      fprintf(stdout, "%-40s", testcases[ii].description);
      fflush(stdout);
    }

    bool reconnect= false;
    enum test_return ret= testcases[ii].function();
    if (ret == TEST_FAIL)
    {
      reconnect= true;
      ++failed;
      if (verbose)
        fprintf(stderr, "\n");
    }
    else if (ret == TEST_PASS_RECONNECT)
      reconnect= true;

    fprintf(stderr, "%s\n", status_msg[ret]);
    if (reconnect)
    {
      closesocket(sock);
      if ((sock= connect_server(hostname, port)) == INVALID_SOCKET)
      {
        fprintf(stderr, "Failed to connect to <%s:%s>: %s\n",
                hostname, port, strerror(get_socket_errno()));
        fprintf(stderr, "%d of %d tests failed\n", failed, total);
        return 1;
      }
    }
  }

  closesocket(sock);
  if (failed == 0)
    fprintf(stdout, "All tests passed\n");
  else
    fprintf(stderr, "%d of %d tests failed\n", failed, total);

  return (failed == 0) ? 0 : 1;
}
