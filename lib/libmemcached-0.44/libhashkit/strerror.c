/* HashKit
 * Copyright (C) 2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "common.h"

const char *hashkit_strerror(hashkit_st *ptr __attribute__((unused)), hashkit_return_t rc)
{
  switch (rc)
  {
  case HASHKIT_SUCCESS:
    return "SUCCESS";
  case HASHKIT_FAILURE:
    return "FAILURE";
  case HASHKIT_MEMORY_ALLOCATION_FAILURE:
    return "MEMORY ALLOCATION FAILURE";
  case HASHKIT_MAXIMUM_RETURN:
    return "Gibberish returned!";
  default:
    return "Gibberish returned!";
  }
}
