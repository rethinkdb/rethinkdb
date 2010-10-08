/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <embedded_innodb-1.0/innodb.h>

#include "storage.h"

const char *tablename= "memcached/items";

#define key_col_idx 0
#define data_col_idx 1
#define flags_col_idx 2
#define cas_col_idx 3
#define exp_col_idx 4

static uint64_t cas;

/**
 * To avoid cluttering down all the code with error checking I use the
 * following macro. It will execute the statement and verify that the
 * result of the operation is DB_SUCCESS. If any other error code is
 * returned it will print an "assert-like" output and jump to the
 * label error_exit. There I release resources before returning out of
 * the function.
 *
 * @param a the expression to execute
 *
 */
#define checked(expression)                                    \
do {                                                           \
  ib_err_t checked_err= expression;                            \
  if (checked_err != DB_SUCCESS)                               \
  {                                                            \
    fprintf(stderr, "ERROR: %s at %u: Failed: <%s>\n\t%s\n",   \
            __FILE__, __LINE__, #expression,                   \
            ib_strerror(checked_err));                         \
    goto error_exit;                                           \
  }                                                            \
} while (0);

/**
 * Create the database schema.
 * @return true if the database schema was created without any problems
 *         false otherwise.
 */
static bool create_schema(void) 
{
  ib_tbl_sch_t schema= NULL;
  ib_idx_sch_t dbindex= NULL;

  if (ib_database_create("memcached") != IB_TRUE)
  {
    fprintf(stderr, "Failed to create database\n");
    return false;
  }

  ib_trx_t transaction= ib_trx_begin(IB_TRX_SERIALIZABLE);
  ib_id_t table_id;

  checked(ib_table_schema_create(tablename, &schema, IB_TBL_COMPACT, 0));
  checked(ib_table_schema_add_col(schema, "key", IB_BLOB,
                                  IB_COL_NOT_NULL, 0, 32767));
  checked(ib_table_schema_add_col(schema, "data", IB_BLOB,
                                  IB_COL_NONE, 0, 1024*1024));
  checked(ib_table_schema_add_col(schema, "flags", IB_INT,
                                  IB_COL_UNSIGNED, 0, 4));
  checked(ib_table_schema_add_col(schema, "cas", IB_INT,
                                  IB_COL_UNSIGNED, 0, 8));
  checked(ib_table_schema_add_col(schema, "exp", IB_INT,
                                  IB_COL_UNSIGNED, 0, 4));
  checked(ib_table_schema_add_index(schema, "PRIMARY_KEY", &dbindex));
  checked(ib_index_schema_add_col(dbindex, "key", 0));
  checked(ib_index_schema_set_clustered(dbindex));
  checked(ib_schema_lock_exclusive(transaction));
  checked(ib_table_create(transaction, schema, &table_id));
  checked(ib_trx_commit(transaction));
  ib_table_schema_delete(schema);

  return true;

 error_exit:
  /* @todo release resources! */
  {
    ib_err_t error= ib_trx_rollback(transaction);
    if (error != DB_SUCCESS)
      fprintf(stderr, "Failed to roll back the transaction:\n\t%s\n",
              ib_strerror(error));
  }
  return false;
}

/**
 * Store an item into the database. Update the CAS id on the item before
 * storing it in the database.
 *
 * @param trx the transaction to use
 * @param item the item to store
 * @return true if we can go ahead and commit the transaction, false otherwise
 */
static bool do_put_item(ib_trx_t trx, struct item* item) 
{
  update_cas(item);

  ib_crsr_t cursor= NULL;
  ib_tpl_t tuple= NULL;
  bool retval= false;

  checked(ib_cursor_open_table(tablename, trx, &cursor));
  checked(ib_cursor_lock(cursor, IB_LOCK_X));
  tuple= ib_clust_read_tuple_create(cursor);

  checked(ib_col_set_value(tuple, key_col_idx, item->key, item->nkey));
  checked(ib_col_set_value(tuple, data_col_idx, item->data, item->size));
  checked(ib_tuple_write_u32(tuple, flags_col_idx, item->flags));
  checked(ib_tuple_write_u64(tuple, cas_col_idx, item->cas));
  checked(ib_tuple_write_u32(tuple, exp_col_idx, (ib_u32_t)item->exp));
  checked(ib_cursor_insert_row(cursor, tuple));

  retval= true;
  /* Release resources: */
  /* FALLTHROUGH */

 error_exit:
  if (tuple != NULL)
    ib_tuple_delete(tuple);

  ib_err_t currsor_error;
  if (cursor != NULL)
    currsor_error= ib_cursor_close(cursor);

  return retval;
}

/**
 * Try to locate an item in the database. Return a cursor and the tuple to
 * the item if I found it in the database.
 *
 * @param trx the transaction to use
 * @param key the key of the item to look up
 * @param nkey the size of the key
 * @param cursor where to store the cursor (OUT)
 * @param tuple where to store the tuple (OUT)
 * @return true if I found the object, false otherwise
 */
static bool do_locate_item(ib_trx_t trx,
                           const void* key,
                           size_t nkey,
                           ib_crsr_t *cursor)
{
  int res;
  ib_tpl_t tuple= NULL;

  *cursor= NULL;

  checked(ib_cursor_open_table(tablename, trx, cursor));
  tuple= ib_clust_search_tuple_create(*cursor);
  if (tuple == NULL)
  {
    fprintf(stderr, "Failed to allocate tuple object\n");
    goto error_exit;
  }

  checked(ib_col_set_value(tuple, key_col_idx, key, nkey));
  ib_err_t err= ib_cursor_moveto(*cursor, tuple, IB_CUR_GE, &res);

  if (err == DB_SUCCESS && res == 0)
  {
    ib_tuple_delete(tuple);
    return true;
  }
  else if (err != DB_SUCCESS &&
           err != DB_RECORD_NOT_FOUND &&
           err != DB_END_OF_INDEX)
  {
    fprintf(stderr, "ERROR: ib_cursor_moveto(): %s\n", ib_strerror(err));
  }
  /* FALLTHROUGH */
 error_exit:
  if (tuple != NULL)
    ib_tuple_delete(tuple);

  ib_err_t cursor_error;
  if (*cursor != NULL)
    cursor_error= ib_cursor_close(*cursor);

  *cursor= NULL;

  return false;
}

/**
 * Try to get an item from the database
 *
 * @param trx the transaction to use
 * @param key the key to get
 * @param nkey the lenght of the key
 * @return a pointer to the item if I found it in the database
 */
static struct item* do_get_item(ib_trx_t trx, const void* key, size_t nkey) 
{
  ib_crsr_t cursor= NULL;
  ib_tpl_t tuple= NULL;
  struct item* retval= NULL;

  if (do_locate_item(trx, key, nkey, &cursor)) 
  {
    tuple= ib_clust_read_tuple_create(cursor);
    if (tuple == NULL)
    {
      fprintf(stderr, "Failed to create read tuple\n");
      goto error_exit;
    }
    checked(ib_cursor_read_row(cursor, tuple));
    ib_col_meta_t meta;
    ib_ulint_t datalen= ib_col_get_meta(tuple, data_col_idx, &meta);
    ib_ulint_t flaglen= ib_col_get_meta(tuple, flags_col_idx, &meta);
    ib_ulint_t caslen= ib_col_get_meta(tuple, cas_col_idx, &meta);
    ib_ulint_t explen= ib_col_get_meta(tuple, exp_col_idx, &meta);
    const void *dataptr= ib_col_get_value(tuple, data_col_idx);

    retval= create_item(key, nkey, dataptr, datalen, 0, 0);
    if (retval == NULL) 
    {
      fprintf(stderr, "Failed to allocate memory\n");
      goto error_exit;
    }

    if (flaglen != 0) 
    {
      ib_u32_t val;
      checked(ib_tuple_read_u32(tuple, flags_col_idx, &val));
      retval->flags= (uint32_t)val;
    }
    if (caslen != 0) 
    {
      ib_u64_t val;
      checked(ib_tuple_read_u64(tuple, cas_col_idx, &val));
      retval->cas= (uint64_t)val;
    }
    if (explen != 0) 
    {
      ib_u32_t val;
      checked(ib_tuple_read_u32(tuple, exp_col_idx, &val));
      retval->exp= (time_t)val;
    }
  }

  /* Release resources */
  /* FALLTHROUGH */

 error_exit:
  if (tuple != NULL)
    ib_tuple_delete(tuple);

  ib_err_t cursor_error;
  if (cursor != NULL)
    cursor_error= ib_cursor_close(cursor);

  return retval;
}

/**
 * Delete an item from the cache
 * @param trx the transaction to use
 * @param key the key of the item to delete
 * @param nkey the length of the key
 * @return true if we should go ahead and commit the transaction
 *         or false if we should roll back (if the key didn't exists)
 */
static bool do_delete_item(ib_trx_t trx, const void* key, size_t nkey) {
  ib_crsr_t cursor= NULL;
  bool retval= false;

  if (do_locate_item(trx, key, nkey, &cursor))
  {
    checked(ib_cursor_lock(cursor, IB_LOCK_X));
    checked(ib_cursor_delete_row(cursor));
    retval= true;
  }
  /* Release resources */
  /* FALLTHROUGH */

 error_exit:
  if (cursor != NULL)
  {
    ib_err_t cursor_error;
    cursor_error= ib_cursor_close(cursor);
  }

  return retval;
}


/****************************************************************************
 * External interface
 ***************************************************************************/

/**
 * Initialize the database storage
 * @return true if the database was initialized successfully, false otherwise
 */
bool initialize_storage(void) 
{
  ib_err_t error;
  ib_id_t tid;

  checked(ib_init());
  checked(ib_cfg_set_text("data_home_dir", "/tmp/memcached_light"));
  checked(ib_cfg_set_text("log_group_home_dir", "/tmp/memcached_light"));
  checked(ib_cfg_set_bool_on("file_per_table"));
  checked(ib_startup("barracuda"));

  /* check to see if the table exists or if we should create the schema */
  error= ib_table_get_id(tablename, &tid);
  if (error == DB_TABLE_NOT_FOUND) 
  {
    if (!create_schema()) 
    {
      return false;
    }
  } 
  else if (error != DB_SUCCESS) 
  {
    fprintf(stderr, "Failed to get table id: %s\n", ib_strerror(error));
    return false;
  }

  return true;

 error_exit:

  return false;
}

/**
 * Shut down this storage engine
 */
void shutdown_storage(void) 
{
  checked(ib_shutdown(IB_SHUTDOWN_NORMAL));
 error_exit:
  ;
}

/**
 * Store an item in the databse
 *
 * @param item the item to store
 */
void put_item(struct item* item) 
{
  ib_trx_t transaction= ib_trx_begin(IB_TRX_SERIALIZABLE);
  if (do_put_item(transaction, item)) 
  {
    ib_err_t error= ib_trx_commit(transaction);
    if (error != DB_SUCCESS) 
    {
      fprintf(stderr, "Failed to store key:\n\t%s\n",
              ib_strerror(error));
    }
  } 
  else 
  {
    ib_err_t error= ib_trx_rollback(transaction);
    if (error != DB_SUCCESS)
      fprintf(stderr, "Failed to roll back the transaction:\n\t%s\n",
              ib_strerror(error));
  }
}

/**
 * Get an item from the engine
 * @param key the key to grab
 * @param nkey number of bytes in the key
 * @return pointer to the item if found
 */
struct item* get_item(const void* key, size_t nkey) 
{
  ib_trx_t transaction= ib_trx_begin(IB_TRX_SERIALIZABLE);
  struct item* ret= do_get_item(transaction, key, nkey);
  ib_err_t error= ib_trx_rollback(transaction);

  if (error != DB_SUCCESS)
    fprintf(stderr, "Failed to roll back the transaction:\n\t%s\n",
            ib_strerror(error));

  return ret;
}

/**
 * Create an item structure and initialize it with the content
 *
 * @param key the key for the item
 * @param nkey the number of bytes in the key
 * @param data pointer to the value for the item (may be NULL)
 * @param size the size of the data
 * @param flags the flags to store with the data
 * @param exp the expiry time for the item
 * @return pointer to an initialized item object or NULL if allocation failed
 */
struct item* create_item(const void* key, size_t nkey, const void* data,
                         size_t size, uint32_t flags, time_t exp)
{
  struct item* ret= calloc(1, sizeof(*ret));
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

/**
 * Delete an item from the cache
 * @param key the key of the item to delete
 * @param nkey the length of the key
 * @return true if the item was deleted from the cache
 */
bool delete_item(const void* key, size_t nkey) {
  ib_trx_t transaction= ib_trx_begin(IB_TRX_REPEATABLE_READ);

  bool ret= do_delete_item(transaction, key, nkey);

  if (ret)
  {
    /* object found. commit transaction */
    ib_err_t error= ib_trx_commit(transaction);
    if (error != DB_SUCCESS)
    {
      fprintf(stderr, "Failed to delete key:\n\t%s\n",
              ib_strerror(error));
      ret= false;
    }
  }
  else
  {
    ib_err_t error= ib_trx_rollback(transaction);
    if (error != DB_SUCCESS)
      fprintf(stderr, "Failed to roll back the transaction:\n\t%s\n",
              ib_strerror(error));
  }

  return ret;
}

/**
 * Flush the entire cache
 * @param when when the cache should be flushed (0 == immediately)
 */
void flush(uint32_t when __attribute__((unused))) 
{
  /* @TODO implement support for when != 0 */
  ib_trx_t transaction= ib_trx_begin(IB_TRX_REPEATABLE_READ);
  ib_crsr_t cursor= NULL;
	ib_err_t err= DB_SUCCESS;

  checked(ib_cursor_open_table(tablename, transaction, &cursor));
  checked(ib_cursor_first(cursor));
  checked(ib_cursor_lock(cursor, IB_LOCK_X));

  do 
  {
    checked(ib_cursor_delete_row(cursor));
  } while ((err= ib_cursor_next(cursor)) == DB_SUCCESS);

  if (err != DB_END_OF_INDEX)
  {
    fprintf(stderr, "Failed to flush the cache: %s\n", ib_strerror(err));
    goto error_exit;
  }
  ib_err_t cursor_error;
  cursor_error= ib_cursor_close(cursor);
  cursor= NULL;
  checked(ib_trx_commit(transaction));
  return;

 error_exit:
  if (cursor != NULL)
  {
    cursor_error= ib_cursor_close(cursor);
  }

  ib_err_t error= ib_trx_rollback(transaction);
  if (error != DB_SUCCESS)
    fprintf(stderr, "Failed to roll back the transaction:\n\t%s\n",
            ib_strerror(error));
}

/**
 * Update the cas ID in the item structure
 * @param item the item to update
 */
void update_cas(struct item* item) 
{
  item->cas= ++cas;
}

/**
 * Release all the resources allocated by the item
 * @param item the item to release
 */
void release_item(struct item* item) 
{
  free(item->key);
  free(item->data);
  free(item);
}
