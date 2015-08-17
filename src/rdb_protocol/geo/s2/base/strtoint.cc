// Copyright 2008 Google Inc. All Rights Reserved.
//
// Architecture-neutral plug compatible replacements for strtol() friends.
// See strtoint.h for details on how to use this component.
//

#include <errno.h>

#include "rdb_protocol/geo/s2/base/port.h"
#include "rdb_protocol/geo/s2/base/basictypes.h"
#include "rdb_protocol/geo/s2/base/strtoint.h"

namespace geo {

// Replacement strto[u]l functions that have identical overflow and underflow
// characteristics for both ILP-32 and LP-64 platforms, including errno
// preservation for error-free calls.
int32 strto32_adapter(const char *nptr, char **endptr, int base) {
  const int saved_errno = errno;
  errno = 0;
  const long result = strtol(nptr, endptr, base);
  if (errno == ERANGE && result == LONG_MIN) {
    return kint32min;
  } else if (errno == ERANGE && result == LONG_MAX) {
    return kint32max;
  } else if (errno == 0 && result < kint32min) {
    errno = ERANGE;
    return kint32min;
  } else if (errno == 0 && result > kint32max) {
    errno = ERANGE;
    return kint32max;
  }
  if (errno == 0)
    errno = saved_errno;
  return static_cast<int32>(result);
}

uint32 strtou32_adapter(const char *nptr, char **endptr, int base) {
  const int saved_errno = errno;
  errno = 0;
  const unsigned long result = strtoul(nptr, endptr, base);
  if (errno == ERANGE && result == ULONG_MAX) {
    return kuint32max;
  } else if (errno == 0 && result > kuint32max) {
    errno = ERANGE;
    return kuint32max;
  }
  if (errno == 0)
    errno = saved_errno;
  return static_cast<uint32>(result);
}

}  // namespace geo
