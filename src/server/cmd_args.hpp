#ifndef __CMD_ARGS_HPP__
#define __CMD_ARGS_HPP__

#include <string>
#include <vector>

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include "serializer/types.hpp"
#include "arch/runtime/runtime.hpp"

#include "serializer/log/config.hpp"
#include "buffer_cache/mirrored/config.hpp"
#include "server/key_value_store_config.hpp"

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>

/* Configuration for replication */
struct replication_config_t {
    std::string hostname;
    int     port;
    bool    active;
    /* Terminate the connection if no heartbeat is received within this many milliseconds */
    int     heartbeat_timeout;
};

/* Configuration for failover */
struct failover_config_t {
    std::string    failover_script_path; /* !< script to be called when the other server goes down */
    bool    active;
    bool    no_rogue; /* whether to go rogue when the master is struggling to stay up */

    failover_config_t() : active(false), no_rogue(false) { }
};

/* Configuration for import */

struct import_config_t {
    bool do_import;
    std::vector<std::string> file;

    import_config_t() : do_import(false) { }
    void add_import_file(std::string s) {
        // See if we can open the file at this path with read permissions
        FILE* import_file = fopen(s.c_str(), "r");
        if (import_file == NULL)
            fail_due_to_user_error("Inaccessible or invalid import file: \"%s\": %s", s.c_str(), strerror(errno));
        else
            fclose(import_file);

        file.push_back(s);
    }
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

    std::string log_file_name;
    // Log messages below this level aren't printed
    //log_level min_log_level;

    // Configuration information for the btree
    btree_key_value_store_dynamic_config_t store_dynamic_config;
    btree_key_value_store_dynamic_config_t metadata_store_dynamic_config;
    btree_key_value_store_static_config_t store_static_config;
    bool create_store, force_create, shutdown_after_creation;

    //replication configuration
    replication_config_t replication_config;
    int replication_master_listen_port;
    bool replication_master_active;
    bool force_unslavify;

    // Configuration for failover
    failover_config_t failover_config;

    // Configuration for import
    import_config_t import_config;

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
    void set_flush_waiting_threshold(const char* value);
    void set_max_concurrent_flushes(const char* value);
    void set_gc_range(const char* value);
    void set_active_data_extents(const char* value);
    void set_block_size(const char* value);
    void set_extent_size(const char* value);
    void set_read_ahead(const char* value);
    void set_coroutine_stack_size(const char* value);
#ifdef SEMANTIC_SERIALIZER_CHECK
    void set_last_semantic_file(const char* value);
#endif
    void set_master_listen_port(const char *value);
    void set_master_addr(const char *value);
    void set_heartbeat_timeout(const char *value);
    void set_failover_file(const char* value);
    void set_io_backend(const char* value);
    void set_io_batch_factor(const char* value);
    void push_private_config(const char* value);
    void set_metadata_file(const char *value);

    long long parse_diff_log_size(const char* value);

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
void usage_serve();
void usage_create();
void usage_import();

#endif // __CMD_ARGS_HPP__
