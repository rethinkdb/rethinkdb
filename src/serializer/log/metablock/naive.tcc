#include "cpu_context.hpp"
#include "event_queue.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

template<class metablock_t>
naive_metablock_manager_t<metablock_t>::naive_metablock_manager_t(extent_manager_t *em)
    : mb_written(0), extent(0), extent_manager(em), state(state_unstarted), dbfd(INVALID_FD)
{
    assert(sizeof(static_header) <= DEVICE_BLOCK_SIZE);
    hdr = (static_header *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
#ifndef NDEBUG
    memset(hdr, 0xBD, DEVICE_BLOCK_SIZE);
#endif

    assert(sizeof(crc_metablock_t) <= DEVICE_BLOCK_SIZE);
    mb_buffer = (crc_metablock_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    mb_buffer_last = (crc_metablock_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    assert(mb_buffer);
#ifndef NDEBUG
    memset(mb_buffer, 0xBD, DEVICE_BLOCK_SIZE);   // Happify Valgrind
    memset(mb_buffer_last, 0xBD, DEVICE_BLOCK_SIZE);   // Happify Valgrind
#endif
#ifdef SERIALIZER_MARKERS
    memcpy(mb_buffer->magic_marker, mb_marker_magic, strlen(mb_marker_magic));
    memcpy(mb_buffer->crc_marker, mb_marker_crc, strlen(mb_marker_crc));
    memcpy(mb_buffer->version_marker, mb_marker_version, strlen(mb_marker_version));
#endif
    mb_buffer->set_crc();
    mb_buffer_in_use = false;
}

template<class metablock_t>
naive_metablock_manager_t<metablock_t>::~naive_metablock_manager_t() {

    assert(state == state_unstarted || state == state_shut_down);
    
    assert(!mb_buffer_in_use);
    free(mb_buffer);
}

template<class metablock_t>
bool naive_metablock_manager_t<metablock_t>::start(fd_t fd, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb) {
    
    assert(state == state_unstarted);
    dbfd = fd;
    assert(dbfd != INVALID_FD);

    struct stat file_stat;
    int res = fstat(fd, &file_stat);
    check("Stat request on file failed", res != 0);

    // Determine if we need to create a new database
    bool create_new_database;
    if (S_ISBLK(file_stat.st_mode)) {
        /* with a block device we don't need to do creation */
        create_new_database = false;
    } else if (S_ISREG(file_stat.st_mode)) {
        off64_t dbsize = lseek64(dbfd, 0, SEEK_END);
        check("Could not determine database file size", dbsize == -1);
        off64_t res = lseek64(dbfd, 0, SEEK_SET);
        check("Could not reset database file position", res == -1);
        create_new_database = (dbsize == 0);
    } else {
        fail("Unsupported file descriptor type");
    }
    
    if (create_new_database) {
        
        *mb_found = false;
        mb_buffer->version = 0;
        for (int i = 0; i < MB_NEXTENTS; i++) {
            /* reserve the ith extent */
            extent_manager->reserve_extent(i * MB_EXTENT_SEPERATION * extent_manager->extent_size, false);
        }
        state = state_writing_header;
        write_headers();
        return true;
    
    } else {
        
        this->mb_found = mb_found;
        this->mb_out = mb_out;
        
        assert(!mb_buffer_in_use);
        mb_buffer_in_use = true;

        version = -1;
        for (int i = 0; i < MB_NEXTENTS; i++) {
            /* reserve the ith extent */
            extent_manager->reserve_extent(i * MB_EXTENT_SEPERATION * extent_manager->extent_size, false);
        }

        event_queue_t *queue = get_cpu_context()->event_queue;
        queue->iosys.schedule_aio_read(
                dbfd,
                DEVICE_BLOCK_SIZE * mb_written + extent * extent_manager->extent_size + DEVICE_BLOCK_SIZE, 
                DEVICE_BLOCK_SIZE, 
                mb_buffer, 
                queue, 
                this);
        
        read_callback = cb;
        state = state_reading;
        return false;
    }
}

template<class metablock_t>
bool naive_metablock_manager_t<metablock_t>::incr_mb_location() {
    mb_written++;
    if (mb_written >= (extent_manager->extent_size - DEVICE_BLOCK_SIZE) / DEVICE_BLOCK_SIZE  ) {
        mb_written = 0;
        extent += MB_EXTENT_SEPERATION;
        if (extent >= MB_NEXTENTS * MB_EXTENT_SEPERATION) {
            extent = 0;
            return true;
        }
    }
    return false;
}

template<class metablock_t>
bool naive_metablock_manager_t<metablock_t>::write_metablock(metablock_t *mb, metablock_write_callback_t *cb) {
    
    assert(state == state_ready);
    
    assert(!mb_buffer_in_use);
    memcpy(&(mb_buffer->metablock), mb, sizeof(metablock_t));
    mb_buffer->set_crc();
    assert(mb_buffer->check_crc());
    mb_buffer_in_use = true;
    
    event_queue_t *queue = get_cpu_context()->event_queue;
    queue->iosys.schedule_aio_write(
        dbfd,
        DEVICE_BLOCK_SIZE * mb_written + extent * extent_manager->extent_size + DEVICE_BLOCK_SIZE,// Offset of beginning of write
        DEVICE_BLOCK_SIZE,                                                    // Length of write
        mb_buffer,
        queue,
        this);

    mb_buffer->version++;
    incr_mb_location();

    state = state_writing;
    write_callback = cb;
    return false;
}

template<class metablock_t>
void naive_metablock_manager_t<metablock_t>::shutdown() {
    
    assert(state == state_ready);
    state = state_shut_down;
}

template<class metablock_t>
void naive_metablock_manager_t<metablock_t>::on_io_complete(event_t *e) {
    bool done_looking = false; /* whether or not the value in mb_buffer_last is the real metablock */
    switch(state) {
 
    case state_reading:
        if (mb_buffer->check_crc()) {
            if (mb_buffer->version > version) {
                /* this metablock is good, maybe there are more? */
                version = mb_buffer->version;
                last_mb_written = mb_written;
                last_mb_extent = extent;
                bool extent_wraparound = incr_mb_location();
                /* mb_buffer_last = mb_buffer and give mb_buffer mb_buffer_last's space so no realloc */
                swap((void **) &mb_buffer_last, (void **) &mb_buffer);
                if (extent_wraparound) {
                    done_looking = true;
                } else {
                    done_looking = false;
                }
            } else {
                /* version smaller than the one we just had, yahtzee */
                done_looking = true;
            }
        } else {
            bool extent_wraparound = incr_mb_location();
            if (extent_wraparound) {
                done_looking = true;
            } else {
                done_looking = false;
            }
        }

        if (done_looking) {
            if (version == -1) {
                /* no metablock found anywhere (either the db is toast, or we're on a block device) */
                state = state_writing_header;
                write_headers();
                *mb_found = false;
            } else {
                /* we found a metablock */
                *mb_found = true;
                swap((void **) &mb_buffer_last, (void **) &mb_buffer);
                free(mb_buffer_last); 
                mb_buffer_last = NULL;

                /* set everything up */
                version = -1; /* version is now useless */
                mb_written = last_mb_written;
                extent = last_mb_extent;
                memcpy(mb_out, &(mb_buffer->metablock), sizeof(metablock_t));
                state = state_ready;
            }
            mb_buffer_in_use = false;
            if (read_callback) read_callback->on_metablock_read();
            read_callback = NULL;
        } else {
            event_queue_t *queue = get_cpu_context()->event_queue;
            queue->iosys.schedule_aio_read(
                    dbfd,
                    DEVICE_BLOCK_SIZE * mb_written + extent * extent_manager->extent_size + DEVICE_BLOCK_SIZE, 
                    DEVICE_BLOCK_SIZE, 
                    mb_buffer, 
                    queue, 
                    this);
        }
        break;

    case state_reading_header:
        //I'm betting at some point there will be more to do here
        state = state_ready;
        break;

    case state_writing_header:
        hdr_ref_count--;
        if (hdr_ref_count == 0)
            state = state_ready;
        break;
        
    case state_writing:
        state = state_ready;
        mb_buffer_in_use = false;
        if (write_callback) write_callback->on_metablock_write();
        write_callback = NULL;
        break;
        
    default:
        fail("Unexpected state.");
    }
}

template<class metablock_t>
void naive_metablock_manager_t<metablock_t>::write_headers() {
    assert(state == state_writing_header);
    static_header hdr(BTREE_BLOCK_SIZE, EXTENT_SIZE);
    memcpy(this->hdr, &hdr, sizeof(static_header));

    event_queue_t *queue = get_cpu_context()->event_queue;
    for (int i = 0; i < MB_NEXTENTS; i++) {
        hdr_ref_count++;
        queue->iosys.schedule_aio_write(
                dbfd,
                i * MB_EXTENT_SEPERATION * extent_manager->extent_size,
                DEVICE_BLOCK_SIZE,
                this->hdr,
                queue,
                this);
    }
}
