/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#include "libmemcached/protocol/common.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

/**
 * Try to parse a key from the string.
 * @pointer start pointer to a pointer to the string (IN and OUT)
 * @return length of the string of -1 if this was an illegal key (invalid
 *         characters or invalid length)
 * @todo add length!
 */
static uint16_t parse_ascii_key(char **start)
{
  uint16_t len= 0;
  char *c= *start;
  /* Strip leading whitespaces */
  while (isspace(*c))
  {
    ++c;
  }

  *start= c;

  while (*c != '\0' && !isspace(*c) && !iscntrl(*c))
  {
    ++c;
    ++len;
  }


  if (len == 0 || len > 240 || (*c != '\0' && *c != '\r' && iscntrl(*c)))
  {
    return 0;
  }

  return len;
}

/**
 * Spool a zero-terminated string
 * @param client destination
 * @param text the text to spool
 * @return status of the spool operation
 */
static protocol_binary_response_status
spool_string(memcached_protocol_client_st *client, const char *text)
{
  return client->root->spool(client, text, strlen(text));
}

/**
 * Send a "CLIENT_ERROR" message back to the client with the correct
 * format of the command being sent
 * @param client the client to send the message to
 */
static void send_command_usage(memcached_protocol_client_st *client)
{
  const char *errmsg[]= {
    [GET_CMD]= "CLIENT_ERROR: Syntax error: get <key>*\r\n",
    [GETS_CMD]= "CLIENT_ERROR: Syntax error: gets <key>*\r\n",
    [SET_CMD]= "CLIENT_ERROR: Syntax error: set <key> <flags> <exptime> <bytes> [noreply]\r\n",
    [ADD_CMD]= "CLIENT_ERROR: Syntax error: add <key> <flags> <exptime> <bytes> [noreply]\r\n",
    [REPLACE_CMD]= "CLIENT_ERROR: Syntax error: replace <key> <flags> <exptime> <bytes> [noreply]\r\n",
    [CAS_CMD]= "CLIENT_ERROR: Syntax error: cas <key> <flags> <exptime> <bytes> <casid> [noreply]\r\n",
    [APPEND_CMD]= "CLIENT_ERROR: Syntax error: append <key> <flags> <exptime> <bytes> [noreply]\r\n",
    [PREPEND_CMD]= "CLIENT_ERROR: Syntax error: prepend <key> <flags> <exptime> <bytes> [noreply]\r\n",
    [DELETE_CMD]= "CLIENT_ERROR: Syntax error: delete <key> [noreply]\r\n",
    [INCR_CMD]= "CLIENT_ERROR: Syntax error: incr <key> <value> [noreply]\r\n",
    [DECR_CMD]= "CLIENT_ERROR: Syntax error: decr <key> <value> [noreply]\r\n",
    [STATS_CMD]= "CLIENT_ERROR: Syntax error: stats [key]\r\n",
    [FLUSH_ALL_CMD]= "CLIENT_ERROR: Syntax error: flush_all [timeout] [noreply]\r\n",
    [VERSION_CMD]= "CLIENT_ERROR: Syntax error: version\r\n",
    [QUIT_CMD]="CLIENT_ERROR: Syntax error: quit\r\n",

    [VERBOSITY_CMD]= "CLIENT_ERROR: Syntax error: verbosity <num>\r\n",
    [UNKNOWN_CMD]= "CLIENT_ERROR: Unknown command\r\n",
  };

  client->mute = false;
  spool_string(client, errmsg[client->ascii_command]);
}

/**
 * Callback for the VERSION responses
 * @param cookie client identifier
 * @param text the length of the body
 * @param textlen the length of the body
 */
static protocol_binary_response_status
ascii_version_response_handler(const void *cookie,
                         const void *text,
                         uint32_t textlen)
{
  memcached_protocol_client_st *client= (void*)cookie;
  spool_string(client, "VERSION ");
  client->root->spool(client, text, textlen);
  spool_string(client, "\r\n");
  return PROTOCOL_BINARY_RESPONSE_SUCCESS;
}

/**
 * Callback for the GET/GETQ/GETK and GETKQ responses
 * @param cookie client identifier
 * @param key the key for the item
 * @param keylen the length of the key
 * @param body the length of the body
 * @param bodylen the length of the body
 * @param flags the flags for the item
 * @param cas the CAS id for the item
 */
static protocol_binary_response_status
ascii_get_response_handler(const void *cookie,
                           const void *key,
                           uint16_t keylen,
                           const void *body,
                           uint32_t bodylen,
                           uint32_t flags,
                           uint64_t cas)
{
  memcached_protocol_client_st *client= (void*)cookie;
  char buffer[300];
  strcpy(buffer, "VALUE ");
  const char *source= key;
  char *dest= buffer + 6;

  for (int x= 0; x < keylen; ++x)
  {
    if (*source != '\0' && !isspace(*source) && !iscntrl(*source))
    {
      *dest= *source;
    }
    else
    {
      return PROTOCOL_BINARY_RESPONSE_EINVAL; /* key constraints in ascii */
    }

    ++dest;
    ++source;
  }

  size_t used= (size_t)(dest - buffer);

  if (client->ascii_command == GETS_CMD)
  {
    snprintf(dest, sizeof(buffer) - used, " %u %u %" PRIu64 "\r\n", flags,
             bodylen, cas);
  }
  else
  {
    snprintf(dest, sizeof(buffer) - used, " %u %u\r\n", flags, bodylen);
  }

  client->root->spool(client, buffer, strlen(buffer));
  client->root->spool(client, body, bodylen);
  client->root->spool(client, "\r\n", 2);

  return PROTOCOL_BINARY_RESPONSE_SUCCESS;
}

/**
 * Callback for the STAT responses
 * @param cookie client identifier
 * @param key the key for the item
 * @param keylen the length of the key
 * @param body the length of the body
 * @param bodylen the length of the body
 */
static protocol_binary_response_status
ascii_stat_response_handler(const void *cookie,
                     const void *key,
                     uint16_t keylen,
                     const void *body,
                     uint32_t bodylen)
{

  memcached_protocol_client_st *client= (void*)cookie;

  if (key != NULL)
  {
    spool_string(client, "STAT ");
    client->root->spool(client, key, keylen);
    spool_string(client, " ");
    client->root->spool(client, body, bodylen);
    spool_string(client, "\r\n");
  }
  else
  {
    spool_string(client, "END\r\n");
  }

  return PROTOCOL_BINARY_RESPONSE_SUCCESS;
}

/**
 * Process a get or a gets request.
 * @param client the client handle
 * @param buffer the complete get(s) command
 * @param end the last character in the command
 */
static void ascii_process_gets(memcached_protocol_client_st *client,
                               char *buffer, char *end)
{
  char *key= buffer;

  /* Skip command */
  key += (client->ascii_command == GETS_CMD) ? 5 : 4;

  int num_keys= 0;
  while (key < end)
  {
    uint16_t nkey= parse_ascii_key(&key);
    if (nkey == 0) /* Invalid key... stop processing this line */
    {
      break;
    }

    (void)client->root->callback->interface.v1.get(client, key, nkey,
                                                   ascii_get_response_handler);
    key += nkey;
    ++num_keys;
  }

  if (num_keys == 0)
  {
    send_command_usage(client);
  }
  else
    client->root->spool(client, "END\r\n", 5);
}

/**
 * Try to split up the command line "asdf asdf asdf asdf\n" into an
 * argument vector for easier parsing.
 * @param start the first character in the command line
 * @param end the last character in the command line ("\n")
 * @param vec the vector to insert the pointers into
 * @size the number of elements in the vector
 * @return the number of tokens in the vector
 */
static int ascii_tokenize_command(char *str, char *end, char **vec, int size)
{
  int elem= 0;

  while (str < end)
  {
    /* Skip leading blanks */
    while (str < end && isspace(*str))
    {
      ++str;
    }

    if (str == end)
    {
      return elem;
    }

    vec[elem++]= str;
    /* find the next non-blank field */
    while (str < end && !isspace(*str))
    {
      ++str;
    }

    /* zero-terminate it for easier parsing later on */
    *str= '\0';
    ++str;

    /* Is the vector full? */
    if (elem == size)
    {
      break;
    }
  }

  return elem;
}

/**
 * If we for some reasons needs to push the line back to read more
 * data we have to reverse the tokenization. Just do the brain-dead replace
 * of all '\0' to ' ' and set the last character to '\n'. We could have used
 * the vector we created, but then we would have to search for all of the
 * spaces we ignored...
 * @param start pointer to the first character in the buffer to recover
 * @param end pointer to the last character in the buffer to recover
 */
static void recover_tokenize_command(char *start, char *end)
{
  while (start < end)
  {
    if (*start == '\0')
      *start= ' ';
    ++start;
  }

  *end= '\n';
}

/**
 * Convert the textual command into a comcode
 */
static enum ascii_cmd ascii_to_cmd(char *start, size_t length)
{
  struct {
    const char *cmd;
    size_t len;
    enum ascii_cmd cc;
  } commands[]= {
    { .cmd= "get", .len= 3, .cc= GET_CMD },
    { .cmd= "gets", .len= 4, .cc= GETS_CMD },
    { .cmd= "set", .len= 3, .cc= SET_CMD },
    { .cmd= "add", .len= 3, .cc= ADD_CMD },
    { .cmd= "replace", .len= 7, .cc= REPLACE_CMD },
    { .cmd= "cas", .len= 3, .cc= CAS_CMD },
    { .cmd= "append", .len= 6, .cc= APPEND_CMD },
    { .cmd= "prepend", .len= 7, .cc= PREPEND_CMD },
    { .cmd= "delete", .len= 6, .cc= DELETE_CMD },
    { .cmd= "incr", .len= 4, .cc= INCR_CMD },
    { .cmd= "decr", .len= 4, .cc= DECR_CMD },
    { .cmd= "stats", .len= 5, .cc= STATS_CMD },
    { .cmd= "flush_all", .len= 9, .cc= FLUSH_ALL_CMD },
    { .cmd= "version", .len= 7, .cc= VERSION_CMD },
    { .cmd= "quit", .len= 4, .cc= QUIT_CMD },
    { .cmd= "verbosity", .len= 9, .cc= VERBOSITY_CMD },
    { .cmd= NULL, .len= 0, .cc= UNKNOWN_CMD }};

  int x= 0;
  while (commands[x].len > 0) {
    if (length >= commands[x].len)
    {
      if (strncmp(start, commands[x].cmd, commands[x].len) == 0)
      {
        /* Potential hit */
        if (length == commands[x].len || isspace(*(start + commands[x].len)))
        {
          return commands[x].cc;
        }
      }
    }
    ++x;
  }

  return UNKNOWN_CMD;
}

/**
 * Perform a delete operation.
 *
 * @param client client requesting the deletion
 * @param tokens the command as a vector
 * @param ntokens the number of items in the vector
 */
static void process_delete(memcached_protocol_client_st *client,
                           char **tokens, int ntokens)
{
  char *key= tokens[1];
  uint16_t nkey;

  if (ntokens != 2 || (nkey= parse_ascii_key(&key)) == 0)
  {
    send_command_usage(client);
    return;
  }

  if (client->root->callback->interface.v1.delete == NULL)
  {
    spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
    return;
  }

  protocol_binary_response_status rval;
  rval= client->root->callback->interface.v1.delete(client, key, nkey, 0);

  if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS)
  {
    spool_string(client, "DELETED\r\n");
  }
  else if (rval == PROTOCOL_BINARY_RESPONSE_KEY_ENOENT)
  {
    spool_string(client, "NOT_FOUND\r\n");
  }
  else
  {
    char msg[80];
    snprintf(msg, sizeof(msg), "SERVER_ERROR: delete failed %u\r\n",(uint32_t)rval);
    spool_string(client, msg);
  }
}

static void process_arithmetic(memcached_protocol_client_st *client,
                               char **tokens, int ntokens)
{
  char *key= tokens[1];
  uint16_t nkey;

  if (ntokens != 3 || (nkey= parse_ascii_key(&key)) == 0)
  {
    send_command_usage(client);
    return;
  }

  uint64_t cas;
  uint64_t result;
  uint64_t delta= strtoull(tokens[2], NULL, 10);

  protocol_binary_response_status rval;
  if (client->ascii_command == INCR_CMD)
  {
    if (client->root->callback->interface.v1.increment == NULL)
    {
      spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
      return;
    }
    rval= client->root->callback->interface.v1.increment(client,
                                                         key, nkey,
                                                         delta, 0,
                                                         0,
                                                         &result,
                                                         &cas);
  }
  else
  {
    if (client->root->callback->interface.v1.decrement == NULL)
    {
      spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
      return;
    }
    rval= client->root->callback->interface.v1.decrement(client,
                                                         key, nkey,
                                                         delta, 0,
                                                         0,
                                                         &result,
                                                         &cas);
  }

  if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS)
  {
    char buffer[80];
    snprintf(buffer, sizeof(buffer), "%"PRIu64"\r\n", result);
    spool_string(client, buffer);
  }
  else
  {
    spool_string(client, "NOT_FOUND\r\n");
  }
}

/**
 * Process the stats command (with or without a key specified)
 * @param key pointer to the first character after "stats"
 * @param end pointer to the "\n"
 */
static void process_stats(memcached_protocol_client_st *client,
                          char *key, char *end)
{
  if (client->root->callback->interface.v1.stat == NULL)
  {
    spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
    return;
  }

  while (isspace(*key))
    key++;

  uint16_t nkey= (uint16_t)(end - key);
  (void)client->root->callback->interface.v1.stat(client, key, nkey,
                                                  ascii_stat_response_handler);
}

static void process_version(memcached_protocol_client_st *client,
                            char **tokens, int ntokens)
{
  (void)tokens;
  if (ntokens != 1)
  {
    send_command_usage(client);
    return;
  }

  if (client->root->callback->interface.v1.version == NULL)
  {
    spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
    return;
  }

 client->root->callback->interface.v1.version(client,
                                              ascii_version_response_handler);
}

static void process_flush(memcached_protocol_client_st *client,
                          char **tokens, int ntokens)
{
  if (ntokens > 2)
  {
    send_command_usage(client);
    return;
  }

  if (client->root->callback->interface.v1.flush == NULL)
  {
    spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
    return;
  }

  uint32_t timeout= 0;
  if (ntokens == 2)
  {
    timeout= (uint32_t)strtoul(tokens[1], NULL, 10);
  }

  protocol_binary_response_status rval;
  rval= client->root->callback->interface.v1.flush(client, timeout);
  if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS)
    spool_string(client, "OK\r\n");
  else
    spool_string(client, "SERVER_ERROR: internal error\r\n");
}

/**
 * Process one of the storage commands
 * @param client the client performing the operation
 * @param tokens the command tokens
 * @param ntokens the number of tokens
 * @param start pointer to the first character in the line
 * @param end pointer to the pointer where the last character of this
 *            command is (IN and OUT)
 * @param length the number of bytes available
 * @return -1 if an error occurs (and we should just terminate the connection
 *            because we are out of sync)
 *         0 storage command completed, continue processing
 *         1 We need more data, so just go ahead and wait for more!
 */
static inline int process_storage_command(memcached_protocol_client_st *client,
                                     char **tokens, int ntokens, char *start,
                                     char **end, ssize_t length)
{
  (void)ntokens; /* already checked */
  char *key= tokens[1];
  uint16_t nkey= parse_ascii_key(&key);
  if (nkey == 0)
  {
    /* return error */
    spool_string(client, "CLIENT_ERROR: bad key\r\n");
    return -1;
  }

  uint32_t flags= (uint32_t)strtoul(tokens[2], NULL, 10);
  uint32_t timeout= (uint32_t)strtoul(tokens[3], NULL, 10);
  unsigned long nbytes= strtoul(tokens[4], NULL, 10);

  /* Do we have all data? */
  unsigned long need= nbytes + (unsigned long)((*end - start) + 1) + 2; /* \n\r\n */
  if ((ssize_t)need > length)
  {
    /* Keep on reading */
    recover_tokenize_command(start, *end);
    return 1;
  }

  void *data= (*end) + 1;
  uint64_t cas= 0;
  uint64_t result_cas;
  protocol_binary_response_status rval;
  switch (client->ascii_command)
  {
  case SET_CMD:
    rval= client->root->callback->interface.v1.set(client, key,
                                                   (uint16_t)nkey,
                                                   data,
                                                   (uint32_t)nbytes,
                                                   flags,
                                                   timeout, cas,
                                                   &result_cas);
    break;
  case ADD_CMD:
    rval= client->root->callback->interface.v1.add(client, key,
                                                   (uint16_t)nkey,
                                                   data,
                                                   (uint32_t)nbytes,
                                                   flags,
                                                   timeout, &result_cas);
    break;
  case CAS_CMD:
    cas= strtoull(tokens[5], NULL, 10);
    /* FALLTHROUGH */
  case REPLACE_CMD:
    rval= client->root->callback->interface.v1.replace(client, key,
                                                       (uint16_t)nkey,
                                                       data,
                                                       (uint32_t)nbytes,
                                                       flags,
                                                       timeout, cas,
                                                       &result_cas);
    break;
  case APPEND_CMD:
    rval= client->root->callback->interface.v1.append(client, key,
                                                      (uint16_t)nkey,
                                                      data,
                                                      (uint32_t)nbytes,
                                                      cas,
                                                      &result_cas);
    break;
  case PREPEND_CMD:
    rval= client->root->callback->interface.v1.prepend(client, key,
                                                       (uint16_t)nkey,
                                                       data,
                                                       (uint32_t)nbytes,
                                                       cas,
                                                       &result_cas);
    break;

    /* gcc complains if I don't put all of the enums in here.. */
  case GET_CMD:
  case GETS_CMD:
  case DELETE_CMD:
  case DECR_CMD:
  case INCR_CMD:
  case STATS_CMD:
  case FLUSH_ALL_CMD:
  case VERSION_CMD:
  case QUIT_CMD:
  case VERBOSITY_CMD:
  case UNKNOWN_CMD:
  default:
    abort(); /* impossible */
  }

  if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS)
  {
    spool_string(client, "STORED\r\n");
  }
  else
  {
    if (client->ascii_command == CAS_CMD)
    {
      if (rval == PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS)
      {
        spool_string(client, "EXISTS\r\n");
      }
      else if (rval == PROTOCOL_BINARY_RESPONSE_KEY_ENOENT)
      {
        spool_string(client, "NOT_FOUND\r\n");
      }
      else
      {
        spool_string(client, "NOT_STORED\r\n");
      }
    }
    else
    {
      spool_string(client, "NOT_STORED\r\n");
    }
  }

  *end += nbytes + 2;

  return 0;
}

static int process_cas_command(memcached_protocol_client_st *client,
                                char **tokens, int ntokens, char *start,
                                char **end, ssize_t length)
{
  if (ntokens != 6)
  {
    send_command_usage(client);
    return false;
  }

  if (client->root->callback->interface.v1.replace == NULL)
  {
    spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
    return false;
  }

  return process_storage_command(client, tokens, ntokens, start, end, length);
}

static int process_set_command(memcached_protocol_client_st *client,
                                char **tokens, int ntokens, char *start,
                                char **end, ssize_t length)
{
  if (ntokens != 5)
  {
    send_command_usage(client);
    return false;
  }

  if (client->root->callback->interface.v1.set == NULL)
  {
    spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
    return false;
  }

  return process_storage_command(client, tokens, ntokens, start, end, length);
}

static int process_add_command(memcached_protocol_client_st *client,
                                char **tokens, int ntokens, char *start,
                                char **end, ssize_t length)
{
  if (ntokens != 5)
  {
    send_command_usage(client);
    return false;
  }

  if (client->root->callback->interface.v1.add == NULL)
  {
    spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
    return false;
  }

  return process_storage_command(client, tokens, ntokens, start, end, length);
}

static int process_replace_command(memcached_protocol_client_st *client,
                                    char **tokens, int ntokens, char *start,
                                    char **end, ssize_t length)
{
  if (ntokens != 5)
  {
    send_command_usage(client);
    return false;
  }

  if (client->root->callback->interface.v1.replace == NULL)
  {
    spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
    return false;
  }

  return process_storage_command(client, tokens, ntokens, start, end, length);
}

static int process_append_command(memcached_protocol_client_st *client,
                                   char **tokens, int ntokens, char *start,
                                   char **end, ssize_t length)
{
  if (ntokens != 5)
  {
    send_command_usage(client);
    return false;
  }

  if (client->root->callback->interface.v1.append == NULL)
  {
    spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
    return false;
  }

  return process_storage_command(client, tokens, ntokens, start, end, length);
}

static int process_prepend_command(memcached_protocol_client_st *client,
                                    char **tokens, int ntokens, char *start,
                                    char **end, ssize_t length)
{
  if (ntokens != 5)
  {
    send_command_usage(client);
    return false;
  }

  if (client->root->callback->interface.v1.prepend == NULL)
  {
    spool_string(client, "SERVER_ERROR: callback not implemented\r\n");
    return false;
  }

  return process_storage_command(client, tokens, ntokens, start, end, length);
}

/**
 * The ASCII protocol support is just one giant big hack. Instead of adding
 * a optimal ascii support, I just convert the ASCII commands to the binary
 * protocol and calls back into the command handlers for the binary protocol ;)
 */
memcached_protocol_event_t memcached_ascii_protocol_process_data(memcached_protocol_client_st *client, ssize_t *length, void **endptr)
{
  char *ptr= (char*)client->root->input_buffer;
  *endptr= ptr;

  do {
    /* Do we have \n (indicating the command preamble)*/
    char *end= memchr(ptr, '\n', (size_t)*length);
    if (end == NULL)
    {
      *endptr= ptr;
      return MEMCACHED_PROTOCOL_READ_EVENT;
    }

    client->ascii_command= ascii_to_cmd(ptr, (size_t)(*length));

    /* A multiget lists all of the keys, and I don't want to have an
     * avector of let's say 512 pointers to tokenize all of them, so let's
     * just handle them immediately
     */
    if (client->ascii_command == GET_CMD ||
        client->ascii_command == GETS_CMD) {
      if (client->root->callback->interface.v1.get != NULL)
        ascii_process_gets(client, ptr, end);
      else
        spool_string(client, "SERVER_ERROR: Command not implemented\n");
    } else {
      /* None of the defined commands takes 10 parameters, so lets just use
       * that as a maximum limit.
       */
      char *tokens[10];
      int ntokens= ascii_tokenize_command(ptr, end, tokens, 10);

      if (ntokens < 10)
      {
        client->mute= strcmp(tokens[ntokens - 1], "noreply") == 0;
        if (client->mute)
          --ntokens; /* processed noreply token*/
      }

      int error= 0;

      switch (client->ascii_command) {
      case SET_CMD:
        error= process_set_command(client, tokens, ntokens, ptr, &end, *length);
        break;
      case ADD_CMD:
        error= process_add_command(client, tokens, ntokens, ptr, &end, *length);
        break;
      case REPLACE_CMD:
        error= process_replace_command(client, tokens, ntokens,
                                       ptr, &end, *length);
        break;
      case CAS_CMD:
        error= process_cas_command(client, tokens, ntokens, ptr, &end, *length);
        break;
      case APPEND_CMD:
        error= process_append_command(client, tokens, ntokens,
                                      ptr, &end, *length);
        break;
      case PREPEND_CMD:
        error= process_prepend_command(client, tokens, ntokens,
                                       ptr, &end, *length);
       break;
      case DELETE_CMD:
        process_delete(client, tokens, ntokens);
        break;

      case INCR_CMD: /* FALLTHROUGH */
      case DECR_CMD:
        process_arithmetic(client, tokens, ntokens);
        break;
      case STATS_CMD:
        if (client->mute)
        {
          send_command_usage(client);
        }
        else
        {
          recover_tokenize_command(ptr, end);
          process_stats(client, ptr + 6, end);
        }
        break;
      case FLUSH_ALL_CMD:
        process_flush(client, tokens, ntokens);
        break;
      case VERSION_CMD:
        if (client->mute)
        {
          send_command_usage(client);
        }
        else
        {
          process_version(client, tokens, ntokens);
        }
        break;
      case QUIT_CMD:
        if (ntokens != 1 || client->mute)
        {
          send_command_usage(client);
        }
        else
        {
          if (client->root->callback->interface.v1.quit != NULL)
            client->root->callback->interface.v1.quit(client);

          return MEMCACHED_PROTOCOL_ERROR_EVENT;
        }
        break;

      case VERBOSITY_CMD:
        if (ntokens != 2)
          send_command_usage(client);
        else
          spool_string(client, "OK\r\n");
        break;

      case UNKNOWN_CMD:
        send_command_usage(client);
        break;

      case GET_CMD:
      case GETS_CMD:
      default:
        /* Should already be handled */
        abort();
      }

      if (error == -1)
        return MEMCACHED_PROTOCOL_ERROR_EVENT;
      else if (error == 1)
        return MEMCACHED_PROTOCOL_READ_EVENT;
    }

    /* Move past \n */
    ++end;
    *length -= end - ptr;
    ptr= end;
  } while (*length > 0);

  *endptr= ptr;
  return MEMCACHED_PROTOCOL_READ_EVENT;
}
