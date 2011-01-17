#ifndef __CMD_ARGS_HPP__
#define __CMD_ARGS_HPP__

#include "config/args.hpp"
#include <stdint.h>
#include <sys/types.h>
#include <string>
#include <vector>

#include "serializer/types.hpp"

#define NEVER_FLUSH -1

/* Private serializer dynamic configuration values */

struct log_serializer_private_dynamic_config_t {
    std::string db_filename;
#ifdef SEMANTIC_SERIALIZER_CHECK
    std::string semantic_filename;
#endif
};

/* Configuration for the serializer that can change from run to run */

struct log_serializer_dynamic_config_t {
    /* When the proportion of garbage blocks hits gc_high_ratio, then the serializer will collect
    garbage until it reaches gc_low_ratio. */
    float gc_low_ratio, gc_high_ratio;
    
    /* How many data block extents the serializer will be writing to at once */
    unsigned num_active_data_extents;

    /* If file_size is nonzero and the serializer is not running on a block device, then it will
    pretend to be running on a block device by immediately resizing the file to file_size and then
    zoning it like a block device. */
    size_t file_size;
    
    /* How big to make each zone if the database is on a block device or if file_size is given */
    size_t file_zone_size;
};

/* Configuration for the serializer that is set when the database is created */
struct log_serializer_static_config_t {
    uint64_t block_size_;
    uint64_t extent_size_;

    uint64_t blocks_per_extent() const { return extent_size_ / block_size_; }
    int block_index(off64_t offset) const { return (offset % extent_size_) / block_size_; }
    int extent_index(off64_t offset) const { return offset / extent_size_; }

    // Minimize calls to these.
    block_size_t block_size() const { return block_size_t::unsafe_make(block_size_); }
    uint64_t extent_size() const { return extent_size_; }

    // Avoid calls to these.
    uint64_t& unsafe_block_size() { return block_size_; }
    uint64_t& unsafe_extent_size() { return extent_size_; }
};

/* Configuration for the cache (it can all change from run to run) */

struct mirrored_cache_config_t {
    // Max amount of memory that will be used for the cache, in bytes.
    long long max_size;
    
    // If wait_for_flush is true, then write operations will not return until after the data is
    // safely sitting on the disk.
    bool wait_for_flush;
    
    // flush_timer_ms is how long (in milliseconds) the cache will allow modified data to sit in
    // memory before flushing it to disk. If it is NEVER_FLUSH, then data will be allowed to sit in
    // memory indefinitely.
    int flush_timer_ms;
    
    // max_dirty_size is the most unsaved data that is allowed in memory before the cache will
    // throttle write transactions. It's in bytes.
    long long max_dirty_size;
    
    // flush_dirty_size is the amount of unsaved data that will trigger an immediate flush. It
    // should be much less than max_dirty_size. It's in bytes.
    long long flush_dirty_size;
};

/* Configuration for the btree that is set when the database is created and serialized in the
serializer */

struct btree_config_t {
    int32_t n_slices;
};

/* Configuration for the store (btree, cache, and serializers) that can be changed from run
to run */

struct btree_key_value_store_dynamic_config_t {
    log_serializer_dynamic_config_t serializer;

    /* Vector of per-serializer database information structures */
    std::vector<log_serializer_private_dynamic_config_t> serializer_private;

    mirrored_cache_config_t cache;
};

/* Configuration for the store (btree, cache, and serializers) that is set at database
creation time */

struct btree_key_value_store_static_config_t {
    log_serializer_static_config_t serializer;
    btree_config_t btree;
};

/* Configuration for replication */
struct replication_config_t {
    char hostname[MAX_HOSTNAME_LEN];
    int port;
};

/* All the configuration together */

struct cmd_config_t {
    /* Methods */

    cmd_config_t(); // Automatically initializes to default values

    void print();
    void print_runtime_flags();
    void print_database_flags();
    void print_system_spec();

    
    /* Attributes */
    
    int port;
    int n_workers;
    
    char log_file_name[MAX_LOG_FILE_NAME];
    // Log messages below this level aren't printed
    //log_level min_log_level;
    
    // Configuration information for the btree
    btree_key_value_store_dynamic_config_t store_dynamic_config;
    btree_key_value_store_static_config_t store_static_config;
    bool create_store, force_create, shutdown_after_creation;

    //replication configuration
    replication_config_t replication_config;

    bool verbose;
};

// This variant adds parsing functionality and also validates the values given
class parsing_cmd_config_t : public cmd_config_t
{
public:
    parsing_cmd_config_t();
    
    void set_cores(const char* value);
    void set_port(const char* value);
    void set_log_file(const char* value);
    void set_slices(const char* value);
    void set_max_cache_size(const char* value);
    void set_wait_for_flush(const char* value);
    void set_unsaved_data_limit(const char* value);
    void set_flush_timer(const char* value);
    void set_gc_range(const char* value);
    void set_active_data_extents(const char* value);
    void set_block_size(const char* value);
    void set_extent_size(const char* value);
    void set_coroutine_stack_size(const char* value);
#ifdef SEMANTIC_SERIALIZER_CHECK
    void set_last_semantic_file(const char* value);
#endif
    void set_master_addr(const char *value);
    void push_private_config(const char* value);
    
private:
    bool parsing_failed;
    int parse_int(const char* value);
    long long int parse_longlong(const char* value);
    
    template<typename T> bool is_positive(const T value) const;
    template<typename T> bool is_in_range(const T value, const T minimum_value, const T maximum_value) const;
    template<typename T> bool is_at_least(const T value, const T minimum_value) const;
    template<typename T> bool is_at_most(const T value, const T maximum_value) const;
};

cmd_config_t parse_cmd_args(int argc, char *argv[]);


#endif // __CMD_ARGS_HPP__

