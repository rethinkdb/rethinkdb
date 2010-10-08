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

#include "config.h"

#include <stdio.h>
#include <string.h>
#include "server.h"

int main(void)
{
  server_startup_st construct;

  memset(&construct, 0, sizeof(server_startup_st));

  construct.count= 4;

  server_startup(&construct);

  return 0;
}
