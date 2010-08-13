
#ifndef __SERIALIZER_LOG_METABLOCK_NAIVE_HPP__
#define __SERIALIZER_LOG_METABLOCK_NAIVE_HPP__

#include "../extents/extent_manager.hpp"
#include "arch/io.hpp"

/* TODO support multiple concurrent writes */

template<class metablock_t>
class naive_metablock_manager_t : private iocallback_t {

public:
    naive_metablock_manager_t(extent_manager_t *em);
    ~naive_metablock_manager_t();

public:
    struct metablock_read_callback_t {
        virtual void on_metablock_read() = 0;
    };
    bool start(fd_t dbfd, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb);
private:
    metablock_read_callback_t *read_callback;
    metablock_t *mb_out;

public:
    struct metablock_write_callback_t {
        virtual void on_metablock_write() = 0;
    };
    bool write_metablock(metablock_t *mb, metablock_write_callback_t *cb);
private:
    metablock_write_callback_t *write_callback;

public:
    void shutdown();

private:
    void on_io_complete(event_t *e);
    
    metablock_t *mb_buffer;
    bool mb_buffer_in_use;
    
    extent_manager_t *extent_manager;
    
    enum state_t {
        state_unstarted,
        state_reading,
        state_ready,
        state_writing,
        state_shut_down
    } state;
    
    fd_t dbfd;
};

#include "naive.tcc"

#endif /* __SERIALIZER_LOG_METABLOCK_NAIVE_HPP__ */
