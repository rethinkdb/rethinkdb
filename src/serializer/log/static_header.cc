#include "static_header.hpp"
#include "config/args.hpp"
#include "utils.hpp"

struct static_header_t {
    char software_name[16];
    char version[16];
    byte_t data[0];
};

struct static_header_write_fsm_t :
    public iocallback_t
{
    static_header_write_callback_t *callback;
    static_header_t *buffer;
    static_header_write_fsm_t() {
        buffer = (static_header_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    }
    ~static_header_write_fsm_t() {
        free(buffer);
    }
    void on_io_complete(event_t *e) {
        if (callback) callback->on_static_header_write();
        gdelete(this);
    }
};

bool static_header_write(direct_file_t *file, void *data, size_t data_size, static_header_write_callback_t *cb) {
    
    static_header_write_fsm_t *fsm = gnew<static_header_write_fsm_t>();
    fsm->callback = cb;
    
    assert(sizeof(static_header_t) + data_size < DEVICE_BLOCK_SIZE);
        
    file->set_size_at_least(DEVICE_BLOCK_SIZE);
    
    bzero(fsm->buffer, DEVICE_BLOCK_SIZE);
    
    assert(sizeof(SOFTWARE_NAME_STRING) < 16);
    memcpy(fsm->buffer->software_name, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING));
    
    assert(sizeof(VERSION_STRING) < 16);
    memcpy(fsm->buffer->version, VERSION_STRING, sizeof(VERSION_STRING));
    
    memcpy(fsm->buffer->data, data, data_size);
    
    bool done __attribute__((unused)) = file->write_async(0, DEVICE_BLOCK_SIZE, fsm->buffer, fsm);
    assert(!done);
    
    return false;
}

struct static_header_read_fsm_t :
    public iocallback_t
{
    static_header_read_callback_t *callback;
    static_header_t *buffer;
    void *data_out;
    size_t data_size;
    static_header_read_fsm_t() {
        buffer = (static_header_t*)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    }
    ~static_header_read_fsm_t() {
        free(buffer);
    }
    void on_io_complete(event_t *e) {
        
        if (memcmp(buffer->software_name, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING)) != 0) {
            fail("This doesn't appear to be a RethinkDB data file.");
        }
        
        if (memcmp(buffer->version, VERSION_STRING, sizeof(VERSION_STRING)) != 0) {
            fail("File version is incorrect. This file was created with version %s of RethinkDB, "
                "but you are trying to read it with version %s.", buffer->version, VERSION_STRING);
        }
        
        memcpy(data_out, buffer->data, data_size);
        
        if (callback) callback->on_static_header_read();
        gdelete(this);
    }
};

bool static_header_read(direct_file_t *file, void *data_out, size_t data_size, static_header_read_callback_t *cb) {
        
    assert(sizeof(static_header_t) + data_size < DEVICE_BLOCK_SIZE);
    
    static_header_read_fsm_t *fsm = gnew<static_header_read_fsm_t>();
    fsm->data_out = data_out;
    fsm->data_size = data_size;

    fsm->callback = cb;
    file->read_async(0, DEVICE_BLOCK_SIZE, fsm->buffer, fsm);
    
    return false;
}
