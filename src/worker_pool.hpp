
#ifndef __WORKER_POOL_HPP__
#define __WORKER_POOL_HPP__

#include <vector>
#include "event_queue.hpp"
#include "arch/resource.hpp"
#include "config/cmd_args.hpp"
#include "config/code.hpp"
#include "corefwd.hpp"
#include "buffer_cache/callbacks.hpp"
#include "buffer_cache/page_map/array.hpp"
#include "message_hub.hpp"
#include "perfmon.hpp"
#include "logger.hpp"
#include "concurrency/rwi_lock.hpp"

struct base_worker_t : public sync_callback<code_config_t> {
    public:
        base_worker_t(int workerid, int _nworkers,
                  worker_pool_t *parent_pool, cmd_config_t *cmd_config);

        ~base_worker_t();
    public:
        //functions for the event_queue
        virtual void start_worker() = 0;
        virtual void shutdown() = 0;

    public:
        //functions for the outside world
        virtual void event_handler(event_t *event) = 0;

    public:
        event_queue_t *event_queue;
        int nworkers;
        int workerid;

    public:
        virtual void on_sync() = 0;
};

struct worker_t : public sync_callback<code_config_t> {
    public:
        typedef code_config_t::cache_t cache_t;
        typedef code_config_t::conn_fsm_t conn_fsm_t;
        typedef code_config_t::fsm_list_t fsm_list_t;
        typedef std::vector<conn_fsm_t*, gnew_alloc<conn_fsm_t*> > shutdown_fsms_t;
    public:
        worker_t(int workerid, int _nworkers,
                  worker_pool_t *parent_pool, cmd_config_t *cmd_config);
        ~worker_t();
    public:
        //functions for the event_queue to call
        void start_worker();
        void start_slices();
        void shutdown();
        void shutdown_slices();
        void delete_slices();
        cache_t *slice(btree_key *key) {
            return slices[key_to_slice(key, nworkers, nslices)];
        }

        void new_fsm(int data, int &resource, void **source);

        /*! \brief degister a specific fsm, used for when an fsm closes a connection
         */
        void deregister_fsm(void *fsm, int &resource);
        /*! \brief degister an active fsm (order not significant), used
         *  for shutdown, returns true if there is an fsm to be shutdown
         */
        bool deregister_fsm(int &resource);
        /*! \brief delete the conn_fsms which have been deregistered
         *  the fsms sources should have been forgotten by the event_queue
         */
        void clean_fsms();
    public:
        //functions for the outside world to call
        void initiate_conn_fsm_transition(event_t *event);
        static void on_btree_completed(code_config_t::btree_fsm_t *btree_fsm);
        void process_btree_msg(code_config_t::btree_fsm_t *btree_fsm);
        void process_perfmon_msg(perfmon_msg_t *msg);
        void process_lock_msg(event_t *event, rwi_lock_t::lock_request_t *lr);
        void process_log_msg(log_msg_t *msg);
        void event_handler(event_t *event);
    public:
        worker_pool_t *parent_pool;
        event_queue_t *event_queue;
        cache_t *slices[MAX_SLICES];

        int nworkers;
        int nslices;

        int workerid;

        log_writer_t log_writer;
    public:
        virtual void on_sync();
    public:
        perfmon_t perfmon;
        int total_connections, curr_connections;
        int curr_items, total_items;
        int cmd_get, cmd_set;
        int get_hits, get_misses;
        int bytes_read, bytes_written;
        logger_t logger;
        //static float uptime();

        btree_value::cas_t gen_cas();
    private:
        bool active_slices;

        fsm_list_t live_fsms;
        shutdown_fsms_t shutdown_fsms;

    private:
        /*! \brief are we trying to shutdown?
         */
        bool shutting_down;

        /*! \brief reference_count is the number of things we need to get back
         *  before we can shut down
         *  this includes sync_callbacks and log_messages
         */
        int ref_count;

        uint32_t cas_counter;
    public:
        void incr_ref_count();
        void decr_ref_count();
};

// Worker pool
struct worker_pool_t {
public:
    worker_pool_t(pthread_t main_thread,
                  cmd_config_t *_cmd_config);
    worker_pool_t(pthread_t main_thread, int _nworkers, int _nslices,
                  cmd_config_t *_cmd_config);
    ~worker_pool_t();
    
    worker_t* next_active_worker();
    
    worker_t *workers[MAX_CPUS];
    int nworkers;
    int nslices;
    int active_worker;
    pthread_t main_thread;
    cmd_config_t *cmd_config;

    // Collects thread local allocators for delete after shutdown
    std::vector<void*, gnew_alloc<void*> > all_allocs;

public:
    /*! global values needed for stats, should these go here?
     */

    //! pid: the pid of the server
    int pid;

    //! startime: the time the server started
    time_t starttime;
    
private:
    void create_worker_pool(pthread_t main_thread,
                            int _nworkers, int _nslices);
};

#endif // __WORKER_POOL_HPP__
