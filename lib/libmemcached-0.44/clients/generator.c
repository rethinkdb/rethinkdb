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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "generator.h"

/* Use this for string generation */
static const char ALPHANUMERICS[]=
  "0123456789ABCDEFGHIJKLMNOPQRSTWXYZabcdefghijklmnopqrstuvwxyz";

#define ALPHANUMERICS_SIZE (sizeof(ALPHANUMERICS)-1)

static size_t get_alpha_num(void)
{
  return (size_t)random() % ALPHANUMERICS_SIZE;
}

static void get_random_string(char *buffer, size_t size)
{
  char *buffer_ptr= buffer;

  while (--size)
    *buffer_ptr++= ALPHANUMERICS[get_alpha_num()];
  *buffer_ptr++= ALPHANUMERICS[get_alpha_num()];
}

void pairs_free(pairs_st *pairs)
{
  uint32_t x;

  if (! pairs)
    return;

  /* We free until we hit the null pair we stores during creation */
  for (x= 0; pairs[x].key; x++)
  {
    free(pairs[x].key);
    if (pairs[x].value)
      free(pairs[x].value);
  }

  free(pairs);
}

pairs_st *pairs_generate(uint64_t number_of, size_t value_length)
{
  unsigned int x;
  pairs_st *pairs;

  pairs= (pairs_st*)calloc((size_t)number_of + 1, sizeof(pairs_st));

  if (!pairs)
    goto error;

  for (x= 0; x < number_of; x++)
  {
    pairs[x].key= (char *)calloc(100, sizeof(char));
    if (!pairs[x].key)
      goto error;
    get_random_string(pairs[x].key, 100);
    pairs[x].key_length= 100;

    if (value_length)
    {
      pairs[x].value= (char *)calloc(value_length, sizeof(char));
      if (!pairs[x].value)
        goto error;
      get_random_string(pairs[x].value, value_length);
      pairs[x].value_length= value_length;
    }
    else
    {
      pairs[x].value= NULL;
      pairs[x].value_length= 0;
    }
  }

  return pairs;
error:
    fprintf(stderr, "Memory Allocation failure in pairs_generate.\n");
    exit(0);
}
