
#ifndef __ARCH_MOCK_IO_HPP__
#define __ARCH_MOCK_IO_HPP__

#include "containers/segmented_vector.hpp"
#include "utils2.hpp"
#include <stdlib.h>

struct mock_iocallback_t {
    virtual void on_io_complete(event_t *event) = 0;
    virtual ~mock_iocallback_t() {}
};

template<class inner_io_config_t>
class mock_direct_file_t
{
    
public:
    enum mode_t {
        mode_read = 1 << 0,
        mode_write = 1 << 1
    };
    
    mock_direct_file_t(const char *path, int mode) {
        
        int mode2 = 0;
        if (mode & mode_read) mode2 |= inner_io_config_t::direct_file_t::mode_read;
        if (mode & mode_write) mode2 |= inner_io_config_t::direct_file_t::mode_write;
        inner_file = new typename inner_io_config_t::direct_file_t(path, mode2);
        
        if (inner_file->is_block_device()) {
            fail_due_to_user_error(
                "Using mock_direct_file_t with a block device is a really bad idea because it "
                "reads the entire contents of the underlying file into memory, which could be "
                "a lot for a block device.");
        }
        
        set_size(inner_file->get_size());
        for (unsigned i = 0; i < get_size() / DEVICE_BLOCK_SIZE; i++) {
            inner_file->read_blocking(i*DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE, blocks[i].data);
        }
    }
    
    bool is_block_device() {
        return false;
    }
    
    uint64_t get_size() {
        return blocks.get_size() * DEVICE_BLOCK_SIZE;
    }
    
    void set_size(size_t size) {
        assert(size % DEVICE_BLOCK_SIZE == 0);
        blocks.set_size(size / DEVICE_BLOCK_SIZE);
    }
    
    void set_size_at_least(size_t size) {
        if (get_size() < size) {
            size_t actual_size = size + randint(10) * DEVICE_BLOCK_SIZE;
            set_size(actual_size);
        }
    }
    
    /* These always return 'false'; the reason they return bool instead of void
    is for consistency with other asynchronous-callback methods */
    bool read_async(size_t offset, size_t length, void *buf, mock_iocallback_t *cb) {
        
        read_blocking(offset, length, buf);
        random_delay(cb, &mock_iocallback_t::on_io_complete, (event_t*)NULL);
        return false;
    }
    
    bool write_async(size_t offset, size_t length, void *buf, mock_iocallback_t *cb) {
        
        write_blocking(offset, length, buf);
        random_delay(cb, &mock_iocallback_t::on_io_complete, (event_t*)NULL);
        return false;
    }
    
    void read_blocking(size_t offset, size_t length, void *buf) {
        
        verify(offset, length, buf);
        for (unsigned i = 0; i < length / DEVICE_BLOCK_SIZE; i += 1) {
            memcpy((char*)buf + i*DEVICE_BLOCK_SIZE, blocks[offset/DEVICE_BLOCK_SIZE + i].data, DEVICE_BLOCK_SIZE);
        }
    }
    
    void write_blocking(size_t offset, size_t length, void *buf) {
        
        verify(offset, length, buf);
        for (unsigned i = 0; i < length / DEVICE_BLOCK_SIZE; i += 1) {
            memcpy(blocks[offset/DEVICE_BLOCK_SIZE + i].data, (char*)buf + i*DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
        }
    }
    
    ~mock_direct_file_t() {
        
        inner_file->set_size(get_size());
        for (unsigned i = 0; i < get_size() / DEVICE_BLOCK_SIZE; i++) {
            inner_file->write_blocking(i*DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE, blocks[i].data);
        }
        
        delete inner_file;
    }

private:
    typename inner_io_config_t::direct_file_t *inner_file;
    
    struct block_t {
        char *data;
        
        block_t() {
            data = (char*)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
            
            // Initialize to either random data or zeroes, choosing at random
            char d = randint(2) ? 0 : randint(0x100);
            memset(data, d, DEVICE_BLOCK_SIZE);
        }
        
        ~block_t() {
            free((void*)data);
        }
    };
    segmented_vector_t<block_t, 10*GIGABYTE/DEVICE_BLOCK_SIZE> blocks;
    
    void verify(size_t offset, size_t length, void *buf) {
        
        assert(buf);
        assert(offset + length <= get_size());
        assert((intptr_t)buf % DEVICE_BLOCK_SIZE == 0);
        assert(offset % DEVICE_BLOCK_SIZE == 0);
        assert(length % DEVICE_BLOCK_SIZE == 0);
    }
};

#endif /* __ARCH_MOCK_IO_HPP__ */
