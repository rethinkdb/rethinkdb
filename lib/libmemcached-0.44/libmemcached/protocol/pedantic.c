/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#include "libmemcached/protocol/common.h"

#include <sys/types.h>
#include <inttypes.h>

#define ensure(a) if (!(a)) { return false; }

bool memcached_binary_protocol_pedantic_check_request(const protocol_binary_request_header *request)
{
  ensure(request->request.magic == PROTOCOL_BINARY_REQ);
  ensure(request->request.datatype == PROTOCOL_BINARY_RAW_BYTES);

  ensure(request->bytes[6] == 0);
  ensure(request->bytes[7] == 0);

  uint8_t opcode= request->request.opcode;
  uint16_t keylen= ntohs(request->request.keylen);
  uint8_t extlen= request->request.extlen;
  uint32_t bodylen= ntohl(request->request.bodylen);

  ensure(bodylen >= (keylen + extlen));

  switch (opcode) {
  case PROTOCOL_BINARY_CMD_GET:
  case PROTOCOL_BINARY_CMD_GETK:
  case PROTOCOL_BINARY_CMD_GETKQ:
  case PROTOCOL_BINARY_CMD_GETQ:
    ensure(extlen == 0);
    ensure(keylen > 0);
    ensure(keylen == bodylen);
    ensure(request->request.cas == 0);
    break;

  case PROTOCOL_BINARY_CMD_ADD:
  case PROTOCOL_BINARY_CMD_ADDQ:
    /* it makes no sense to run add with a cas value */
    ensure(request->request.cas == 0);
    /* FALLTHROUGH */
  case PROTOCOL_BINARY_CMD_SET:
  case PROTOCOL_BINARY_CMD_SETQ:
  case PROTOCOL_BINARY_CMD_REPLACE:
  case PROTOCOL_BINARY_CMD_REPLACEQ:
    ensure(keylen > 0);
    ensure(extlen == 8);
    break;

  case PROTOCOL_BINARY_CMD_DELETE:
  case PROTOCOL_BINARY_CMD_DELETEQ:
    ensure(extlen == 0);
    ensure(keylen > 0);
    ensure(keylen == bodylen);
    break;

  case PROTOCOL_BINARY_CMD_INCREMENT:
  case PROTOCOL_BINARY_CMD_INCREMENTQ:
  case PROTOCOL_BINARY_CMD_DECREMENT:
  case PROTOCOL_BINARY_CMD_DECREMENTQ:
    ensure(extlen == 20);
    ensure(keylen > 0);
    ensure(keylen + extlen == bodylen);
    break;

  case PROTOCOL_BINARY_CMD_QUIT:
  case PROTOCOL_BINARY_CMD_QUITQ:
  case PROTOCOL_BINARY_CMD_NOOP:
  case PROTOCOL_BINARY_CMD_VERSION:
    ensure(extlen == 0);
    ensure(keylen == 0);
    ensure(bodylen == 0);
    break;

  case PROTOCOL_BINARY_CMD_FLUSH:
  case PROTOCOL_BINARY_CMD_FLUSHQ:
    ensure(extlen == 0 || extlen == 4);
    ensure(keylen == 0);
    ensure(bodylen == extlen);
    break;

  case PROTOCOL_BINARY_CMD_STAT:
    ensure(extlen == 0);
    /* May have key, but not value */
    ensure(keylen == bodylen);
    break;

  case PROTOCOL_BINARY_CMD_APPEND:
  case PROTOCOL_BINARY_CMD_APPENDQ:
  case PROTOCOL_BINARY_CMD_PREPEND:
  case PROTOCOL_BINARY_CMD_PREPENDQ:
    ensure(extlen == 0);
    ensure(keylen > 0);
    break;
  default:
    /* Unknown command */
    ;
  }

  return true;
}

bool memcached_binary_protocol_pedantic_check_response(const protocol_binary_request_header *request,
                                                       const protocol_binary_response_header *response)
{
  ensure(response->response.magic == PROTOCOL_BINARY_RES);
  ensure(response->response.datatype == PROTOCOL_BINARY_RAW_BYTES);
  ensure(response->response.opaque == request->request.opaque);

  uint16_t status= ntohs(response->response.status);
  uint8_t opcode= response->response.opcode;

  if (status == PROTOCOL_BINARY_RESPONSE_SUCCESS)
  {
    switch (opcode) {
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
      /* Quiet command shouldn't return on success */
      return false;
    default:
      break;
    }

    switch (opcode) {
    case PROTOCOL_BINARY_CMD_ADD:
    case PROTOCOL_BINARY_CMD_REPLACE:
    case PROTOCOL_BINARY_CMD_SET:
    case PROTOCOL_BINARY_CMD_APPEND:
    case PROTOCOL_BINARY_CMD_PREPEND:
      ensure(response->response.keylen == 0);
      ensure(response->response.extlen == 0);
      ensure(response->response.bodylen == 0);
      ensure(response->response.cas != 0);
      break;
    case PROTOCOL_BINARY_CMD_FLUSH:
    case PROTOCOL_BINARY_CMD_NOOP:
    case PROTOCOL_BINARY_CMD_QUIT:
    case PROTOCOL_BINARY_CMD_DELETE:
      ensure(response->response.keylen == 0);
      ensure(response->response.extlen == 0);
      ensure(response->response.bodylen == 0);
      ensure(response->response.cas == 0);
      break;

    case PROTOCOL_BINARY_CMD_DECREMENT:
    case PROTOCOL_BINARY_CMD_INCREMENT:
      ensure(response->response.keylen == 0);
      ensure(response->response.extlen == 0);
      ensure(ntohl(response->response.bodylen) == 8);
      ensure(response->response.cas != 0);
      break;

    case PROTOCOL_BINARY_CMD_STAT:
      ensure(response->response.extlen == 0);
      /* key and value exists in all packets except in the terminating */
      ensure(response->response.cas == 0);
      break;

    case PROTOCOL_BINARY_CMD_VERSION:
      ensure(response->response.keylen == 0);
      ensure(response->response.extlen == 0);
      ensure(response->response.bodylen != 0);
      ensure(response->response.cas == 0);
      break;

    case PROTOCOL_BINARY_CMD_GET:
    case PROTOCOL_BINARY_CMD_GETQ:
      ensure(response->response.keylen == 0);
      ensure(response->response.extlen == 4);
      ensure(response->response.cas != 0);
      break;

    case PROTOCOL_BINARY_CMD_GETK:
    case PROTOCOL_BINARY_CMD_GETKQ:
      ensure(response->response.keylen != 0);
      ensure(response->response.extlen == 4);
      ensure(response->response.cas != 0);
      break;

    default:
      /* Undefined command code */
      break;
    }
  }
  else
  {
    ensure(response->response.cas == 0);
    ensure(response->response.extlen == 0);
    if (opcode != PROTOCOL_BINARY_CMD_GETK)
    {
      ensure(response->response.keylen == 0);
    }
  }

  return true;
}
