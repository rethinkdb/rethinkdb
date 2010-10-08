/* 
  I debated about putting this in the client library since it does an 
  action I don't really believe belongs in the library.

  Frankly its too damn useful not to be here though.
*/

#include "common.h"

memcached_server_list_st memcached_servers_parse(const char *server_strings)
{
  char *string;
  in_port_t port;
  uint32_t weight;
  const char *begin_ptr;
  const char *end_ptr;
  memcached_server_st *servers= NULL;
  memcached_return_t rc;

  WATCHPOINT_ASSERT(server_strings);

  end_ptr= server_strings + strlen(server_strings);

  for (begin_ptr= server_strings, string= index(server_strings, ','); 
       begin_ptr != end_ptr; 
       string= index(begin_ptr, ','))
  {
    char buffer[HUGE_STRING_LEN];
    char *ptr, *ptr2;
    port= 0;
    weight= 0;

    if (string)
    {
      memcpy(buffer, begin_ptr, (size_t) (string - begin_ptr));
      buffer[(unsigned int)(string - begin_ptr)]= 0;
      begin_ptr= string+1;
    }
    else
    {
      size_t length= strlen(begin_ptr);
      memcpy(buffer, begin_ptr, length);
      buffer[length]= 0;
      begin_ptr= end_ptr;
    }

    ptr= index(buffer, ':');

    if (ptr)
    {
      ptr[0]= 0;

      ptr++;

      port= (in_port_t) strtoul(ptr, (char **)NULL, 10);

      ptr2= index(ptr, ' ');
      if (! ptr2)
        ptr2= index(ptr, ':');
      if (ptr2)
      {
        ptr2++;
        weight = (uint32_t) strtoul(ptr2, (char **)NULL, 10);
      }
    }

    servers= memcached_server_list_append_with_weight(servers, buffer, port, weight, &rc);

    if (isspace(*begin_ptr))
      begin_ptr++;
  }

  return servers;
}
