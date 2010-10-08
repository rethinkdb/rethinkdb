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

#include "byteorder.h"

/* Byte swap a 64-bit number. */
#ifndef swap64
static inline uint64_t swap64(uint64_t in)
{
#ifndef WORDS_BIGENDIAN
  /* Little endian, flip the bytes around until someone makes a faster/better
   * way to do this. */
  uint64_t rv= 0;
  for (uint8_t x= 0; x < 8; x++)
  {
    rv= (rv << 8) | (in & 0xff);
    in >>= 8;
  }
  return rv;
#else
  /* big-endian machines don't need byte swapping */
  return in;
#endif // WORDS_BIGENDIAN
}
#endif

uint64_t memcached_ntohll(uint64_t value)
{
  return swap64(value);
}

uint64_t memcached_htonll(uint64_t value)
{
  return swap64(value);
}
