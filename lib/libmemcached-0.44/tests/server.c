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

/*
  Startup, and shutdown the memcached servers.
*/

#define TEST_PORT_BASE MEMCACHED_DEFAULT_PORT+10

#define PID_FILE_BASE "/tmp/%ulibmemcached_memc.pid"

#include "config.h"

#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <libmemcached/memcached.h>
#include <libmemcached/util.h>

#include "server.h"

static struct timespec global_sleep_value= { .tv_sec= 0, .tv_nsec= 50000 };

static void global_sleep(void)
{
#ifdef WIN32
  sleep(1);
#else
  nanosleep(&global_sleep_value, NULL);
#endif
}

static void kill_file(const char *file_buffer)
{
  FILE *fp= fopen(file_buffer, "r");

  while ((fp= fopen(file_buffer, "r")))
  {
    char pid_buffer[1024];

    if (fgets(pid_buffer, sizeof(pid_buffer), fp) != NULL)
    {
      pid_t pid= (pid_t)atoi(pid_buffer);
      if (pid != 0)
      {
        if (kill(pid, SIGTERM) == -1)
        {
          remove(file_buffer); // If this happens we may be dealing with a dead server that left its pid file.
        }
        else
        {
          uint32_t counter= 3;
          while ((kill(pid, 0) == 0) && --counter)
          {
            global_sleep();
          }
        }
      }
    }

    global_sleep();

    fclose(fp);
  }
}

void server_startup(server_startup_st *construct)
{
  if ((construct->server_list= getenv("MEMCACHED_SERVERS")))
  {
    printf("servers %s\n", construct->server_list);
    construct->servers= memcached_servers_parse(construct->server_list);
    construct->server_list= NULL;
    construct->count= 0;
  }
  else
  {
    {
      char server_string_buffer[8096];
      char *end_ptr;
      end_ptr= server_string_buffer;

      for (uint32_t x= 0; x < construct->count; x++)
      {
        int count;
        int status;
        in_port_t port;

        {
          char *var;
          char variable_buffer[1024];

          snprintf(variable_buffer, sizeof(variable_buffer), "LIBMEMCACHED_PORT_%u", x);

          if ((var= getenv(variable_buffer)))
          {
            port= (in_port_t)atoi(var);
          }
          else
          {
            port= (in_port_t)(x + TEST_PORT_BASE);
          }
        }

        char buffer[PATH_MAX];
        snprintf(buffer, sizeof(buffer), PID_FILE_BASE, x);
        kill_file(buffer);

        if (x == 0)
        {
          snprintf(buffer, sizeof(buffer), "%s -d -u root -P "PID_FILE_BASE" -t 1 -p %u -U %u -m 128",
                   MEMCACHED_BINARY, x, port, port);
        }
        else
        {
          snprintf(buffer, sizeof(buffer), "%s -d -u root -P "PID_FILE_BASE" -t 1 -p %u -U %u",
                   MEMCACHED_BINARY, x, port, port);
        }
	if (libmemcached_util_ping("localhost", port, NULL))
	{
	  fprintf(stderr, "Server on port %u already exists\n", port);
	}
	else
	{
	  status= system(buffer);
	  fprintf(stderr, "STARTING SERVER: %s  status:%d\n", buffer, status);
	}
        count= sprintf(end_ptr, "localhost:%u,", port);
        end_ptr+= count;
      }
      *end_ptr= 0;


      int *pids= calloc(construct->count, sizeof(int));
      for (uint32_t x= 0; x < construct->count; x++)
      {
        char buffer[PATH_MAX]; /* Nothing special for number */

        snprintf(buffer, sizeof(buffer), PID_FILE_BASE, x);

        uint32_t counter= 3000; // Absurd, just to catch run away process
        while (pids[x] <= 0  && --counter)
        {
          FILE *file= fopen(buffer, "r");
          if (file)
          {
            char pid_buffer[1024];
            char *found= fgets(pid_buffer, sizeof(pid_buffer), file);

            if (found)
            {
              pids[x]= atoi(pid_buffer);
              fclose(file);

              if (pids[x] > 0)
                break;
            }
            fclose(file);
          }
          global_sleep();
        }

        bool was_started= false;
        if (pids[x] > 0)
        {
          counter= 30;
          while (--counter)
          {
            if (kill(pids[x], 0) == 0)
            {
              was_started= true;
              break;
            }
            global_sleep();
          }
        }

        if (was_started == false)
        {
          fprintf(stderr, "Failed to open buffer %s(%d)\n", buffer, pids[x]);
          for (uint32_t y= 0; y < construct->count; y++)
          {
            if (pids[y] > 0)
              kill(pids[y], SIGTERM);
          }
          abort();
        }
      }
      free(pids);

      construct->server_list= strdup(server_string_buffer);
    }
    printf("servers %s\n", construct->server_list);
    construct->servers= memcached_servers_parse(construct->server_list);
  }

  assert(construct->servers);

  srandom((unsigned int)time(NULL));

  for (uint32_t x= 0; x < memcached_server_list_count(construct->servers); x++)
  {
    printf("\t%s : %d\n", memcached_server_name(&construct->servers[x]), memcached_server_port(&construct->servers[x]));
    assert(construct->servers[x].fd == -1);
    assert(construct->servers[x].cursor_active == 0);
  }

  printf("\n");
}

void server_shutdown(server_startup_st *construct)
{
  if (construct->server_list)
  {
    for (uint32_t x= 0; x < construct->count; x++)
    {
      char file_buffer[PATH_MAX]; /* Nothing special for number */
      snprintf(file_buffer, sizeof(file_buffer), PID_FILE_BASE, x);
      kill_file(file_buffer);
    }

    free(construct->server_list);
  }
}
