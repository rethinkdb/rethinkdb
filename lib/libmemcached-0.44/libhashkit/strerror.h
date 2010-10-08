/* HashKit
 * Copyright (C) 2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#ifndef HASHKIT_STRERROR_H
#define HASHKIT_STRERROR_H

#ifdef __cplusplus
extern "C" {
#endif

HASHKIT_API
  const char *hashkit_strerror(hashkit_st *ptr, hashkit_return_t rc);

#ifdef __cplusplus
}
#endif

#endif /* HASHKIT_STRERROR_H */
