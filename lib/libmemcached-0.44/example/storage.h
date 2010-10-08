/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#ifndef STORAGE_H
#define STORAGE_H

struct item {
  uint64_t cas;
  void* key;
  size_t nkey;
  void* data;
  size_t size;
  uint32_t flags;
  time_t exp;
};

bool initialize_storage(void);
void shutdown_storage(void);

void update_cas(struct item* item);
void put_item(struct item* item);
struct item* get_item(const void* key, size_t nkey);
struct item* create_item(const void* key, size_t nkey, const void *data,
                         size_t size, uint32_t flags, time_t exp);
bool delete_item(const void* key, size_t nkey);
void flush(uint32_t when);
void release_item(struct item* item);

#endif
