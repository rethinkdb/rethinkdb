/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#include "config.h"
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include "storage.h"

struct list_entry {
  struct item item;
  struct list_entry *next;
  struct list_entry *prev;
};

static struct list_entry *root;
static uint64_t cas;

bool initialize_storage(void)
{
  return true;
}

void shutdown_storage(void)
{
  /* Do nothing */
}

void put_item(struct item* item)
{
  struct list_entry* entry= (void*)item;

  update_cas(item);

  if (root == NULL)
  {
    entry->next= entry->prev= entry;
  }
  else
  {
    entry->prev= root->prev;
    entry->next= root;
    entry->prev->next= entry;
    entry->next->prev= entry;
  }

  root= entry;
}

struct item* get_item(const void* key, size_t nkey)
{
  struct list_entry *walker= root;

  if (root == NULL)
  {
    return NULL;
  }

  do
  {
    if (((struct item*)walker)->nkey == nkey &&
        memcmp(((struct item*)walker)->key, key, nkey) == 0)
    {
      return (struct item*)walker;
    }
    walker= walker->next;
  } while (walker != root);

  return NULL;
}

struct item* create_item(const void* key, size_t nkey, const void* data,
                         size_t size, uint32_t flags, time_t exp)
{
  struct item* ret= calloc(1, sizeof(struct list_entry));

  if (ret != NULL)
  {
    ret->key= malloc(nkey);
    if (size > 0)
    {
      ret->data= malloc(size);
    }

    if (ret->key == NULL || (size > 0 && ret->data == NULL))
    {
      free(ret->key);
      free(ret->data);
      free(ret);
      return NULL;
    }

    memcpy(ret->key, key, nkey);
    if (data != NULL)
    {
      memcpy(ret->data, data, size);
    }

    ret->nkey= nkey;
    ret->size= size;
    ret->flags= flags;
    ret->exp= exp;
  }

  return ret;
}

bool delete_item(const void* key, size_t nkey)
{
  struct item* item= get_item(key, nkey);
  bool ret= false;

  if (item)
  {
    /* remove from linked list */
    struct list_entry *entry= (void*)item;

    if (entry->next == entry)
    {
      /* Only one object in the list */
      root= NULL;
    }
    else
    {
      /* ensure that we don't loose track of the root, and this will
       * change the start position for the next search ;-) */
      root= entry->next;
      entry->prev->next= entry->next;
      entry->next->prev= entry->prev;
    }

    free(item->key);
    free(item->data);
    free(item);
    ret= true;
  }

  return ret;
}

void flush(uint32_t when)
{
  /* FIXME */
  (void)when;
  /* remove the complete linked list */
  if (root == NULL)
  {
    return;
  }

  root->prev->next= NULL;
  while (root != NULL)
  {
    struct item* tmp= (void*)root;
    root= root->next;

    free(tmp->key);
    free(tmp->data);
    free(tmp);
  }
}

void update_cas(struct item* item)
{
  item->cas= ++cas;
}

void release_item(struct item* item __attribute__((unused)))
{
  /* EMPTY */
}
