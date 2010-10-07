#include "arch/arch.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* head functions */

template<class metablock_t>
metablock_manager_t<metablock_t>::metablock_manager_t::head_t::head_t()
    : mb_slot(0), extent(0), saved_mb_slot(-1), saved_extent(-1), extent_size(-1), wraparound(false)
{}

template<class metablock_t>
void metablock_manager_t<metablock_t>::metablock_manager_t::head_t::operator++(int a) {
    check("Head ++ called with setting extent_size", extent_size == 0);
    mb_slot++;
    wraparound = false;
    if (mb_slot >= (extent_size - DEVICE_BLOCK_SIZE) / DEVICE_BLOCK_SIZE  ) {
        mb_slot = 0;
        extent += MB_EXTENT_SEPERATION;
        if (extent >= MB_NEXTENTS * MB_EXTENT_SEPERATION) {
            extent = 0;
            wraparound = true;
        }
    }
}

template<class metablock_t>
off64_t metablock_manager_t<metablock_t>::metablock_manager_t::head_t::offset() {
    check("Head offset called with setting extent_size", extent_size == 0);
    return DEVICE_BLOCK_SIZE * mb_slot + extent * extent_size + DEVICE_BLOCK_SIZE;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::metablock_manager_t::head_t::push() {
    saved_mb_slot = mb_slot;
    saved_extent = extent;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::metablock_manager_t::head_t::pop() {
    check("Popping without a saved state", saved_mb_slot == (uint32_t) -1 || saved_extent == (uint32_t) -1);
    mb_slot = saved_mb_slot;
    extent = saved_extent;
    
    saved_mb_slot = -1;
    saved_extent = -1;
}

template<class metablock_t>
metablock_manager_t<metablock_t>::metablock_manager_t(extent_manager_t *em)
    : extent_manager(em), state(state_unstarted), dbfile(NULL), hdr(NULL), hdr_ref_count(0)
{
    head.extent_size = extent_manager->extent_size;
    assert(sizeof(static_header) <= DEVICE_BLOCK_SIZE);
    hdr = (static_header *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
#ifdef VALGRIND
    memset(hdr, 0xBD, DEVICE_BLOCK_SIZE);              // Happify Valgrind
#endif

    assert(sizeof(crc_metablock_t) <= DEVICE_BLOCK_SIZE);
    mb_buffer = (crc_metablock_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    mb_buffer_last = (crc_metablock_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    assert(mb_buffer);
#ifdef VALGRIND
    memset(mb_buffer, 0xBD, DEVICE_BLOCK_SIZE);        // Happify Valgrind
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
metablock_manager_t<metablock_t>::~metablock_manager_t() {

    assert(state == state_unstarted || state == state_shut_down);

    free(hdr);
    
    assert(!mb_buffer_in_use);
    free(mb_buffer);

    free(mb_buffer_last); 
    mb_buffer_last = NULL;
}

template<class metablock_t>
bool metablock_manager_t<metablock_t>::start(direct_file_t *file, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb) {
    
    assert(state == state_unstarted);
    dbfile = file;
    assert(dbfile != NULL);
    
    this->mb_found = mb_found;
    this->mb_out = mb_out;
    
    assert(!mb_buffer_in_use);
    mb_buffer_in_use = true;

    version = -1;
    
    dbfile->set_size_at_least(((MB_NEXTENTS - 1) * MB_EXTENT_SEPERATION + 1) * extent_manager->extent_size);
    for (int i = 0; i < MB_NEXTENTS; i++) {
        /* reserve the ith extent */
        extent_manager->reserve_extent(i * MB_EXTENT_SEPERATION * extent_manager->extent_size);
    }
    
    dbfile->read_async(head.offset(), DEVICE_BLOCK_SIZE, mb_buffer, this);
    
    read_callback = cb;
    state = state_reading;
    return false;
}


template<class metablock_t>
bool metablock_manager_t<metablock_t>::write_metablock(metablock_t *mb, metablock_write_callback_t *cb) {
    if (state != state_ready) {
        /* TODO right now we only queue writes when we're presently writing
           this is basically the only use case... but we could easily queue
           in other places by modifying on_io_complete
         */
        //printf("Enqueuing write\n");
        assert(state == state_writing);
        outstanding_writes.push_back(metablock_write_req_t(mb, cb));
    } else {
        assert(!mb_buffer_in_use);
        memcpy(&(mb_buffer->metablock), mb, sizeof(metablock_t));
        mb_buffer->set_crc();
        assert(mb_buffer->check_crc());
        mb_buffer_in_use = true;
        
        dbfile->write_async(head.offset(), DEVICE_BLOCK_SIZE, mb_buffer, this);

        mb_buffer->version++;
        head++;

        state = state_writing;
        write_callback = cb;
    }
    return false;
}

template <class metablock_t>
metablock_manager_t<metablock_t>::metablock_write_req_t::metablock_write_req_t(metablock_t *mb, metablock_write_callback_t *cb)
    :mb(mb), cb(cb)
{}

template<class metablock_t>
void metablock_manager_t<metablock_t>::shutdown() {
    
    assert(state == state_ready);
    state = state_shut_down;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::on_io_complete(event_t *e) {
    bool done_looking = false; /* whether or not the value in mb_buffer_last is the real metablock */
    switch(state) {
 
    case state_reading:
        if (mb_buffer->check_crc()) {
            if (mb_buffer->version > version) {
                /* this metablock is good, maybe there are more? */
                version = mb_buffer->version;
                head.push();
                head++;
                /* mb_buffer_last = mb_buffer and give mb_buffer mb_buffer_last's space so no realloc */
                swap((void **) &mb_buffer_last, (void **) &mb_buffer);
                if (head.wraparound) {
                    done_looking = true;
                } else {
                    done_looking = false;
                }
            } else {
                /* version smaller than the one we just had, yahtzee */
                done_looking = true;
            }
        } else {
            head++;
            if (head.wraparound) {
                done_looking = true;
            } else {
                done_looking = false;
            }
        }

        if (done_looking) {
            if (version == -1) {
                
                /* no metablock found anywhere (either the db is toast, or we're on a block device) */
                
                // Start versions from 0, not from whatever garbage was in the last thing we read
                mb_buffer->version = 0;
                
                state = state_writing_header;
                write_headers();
                *mb_found = false;
                
            } else {
                /* we found a metablock */
                *mb_found = true;
                swap((void **) &mb_buffer_last, (void **) &mb_buffer);

                /* set everything up */
                version = -1; /* version is now useless */
                head.pop();
                memcpy(mb_out, &(mb_buffer->metablock), sizeof(metablock_t));
                state = state_reading_header;
                read_headers();
            }
            mb_buffer_in_use = false;
        } else {
            dbfile->read_async(
                head.offset(),
                DEVICE_BLOCK_SIZE,
                mb_buffer,
                this);
        }
        break;

    case state_reading_header:
        //TODO, we should actually do something with the header once we have it
        state = state_ready;
        if (read_callback) read_callback->on_metablock_read();
            read_callback = NULL;
        break;

    case state_writing_header:
        hdr_ref_count--;
        if (hdr_ref_count == 0) {
            state = state_ready;
            if (read_callback) {
                read_callback->on_metablock_read();
            }
            read_callback = NULL;
        }
        break;
        
    case state_writing:
        state = state_ready;
        mb_buffer_in_use = false;
        if (write_callback) write_callback->on_metablock_write();
        write_callback = NULL;
        if (!outstanding_writes.empty()) {
            metablock_write_req_t req = outstanding_writes.front();
            outstanding_writes.pop_front();
            write_metablock(req.mb, req.cb);
        }
        break;
        
    default:
        fail("Unexpected state.");
    }
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::write_headers() {
    assert(state == state_writing_header);
    static_header hdr(BTREE_BLOCK_SIZE, EXTENT_SIZE);
    memcpy(this->hdr, &hdr, sizeof(static_header));

    for (int i = 0; i < MB_NEXTENTS; i++) {
        hdr_ref_count++;
        dbfile->write_async(
            i * MB_EXTENT_SEPERATION * extent_manager->extent_size,
            DEVICE_BLOCK_SIZE,
            this->hdr,
            this);
    }
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::read_headers() {
    assert(state == state_reading_header);
    
    dbfile->read_async(0, DEVICE_BLOCK_SIZE, hdr, this);
}
