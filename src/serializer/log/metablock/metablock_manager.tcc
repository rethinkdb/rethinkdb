#include "arch/arch.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/scoped_array.hpp>

/* head functions */

template<class metablock_t>
metablock_manager_t<metablock_t>::metablock_manager_t::head_t::head_t(metablock_manager_t *manager)
    : mb_slot(0), saved_mb_slot(-1), wraparound(false), mgr(manager) { }

template<class metablock_t>
void metablock_manager_t<metablock_t>::metablock_manager_t::head_t::operator++(int a) {
    mb_slot++;
    wraparound = false;
    
    if (mb_slot >= mgr->metablock_offsets.size()) {
        mb_slot = 0;
        wraparound = true;
    }
}

template<class metablock_t>
off64_t metablock_manager_t<metablock_t>::metablock_manager_t::head_t::offset() {
    
    return mgr->metablock_offsets[mb_slot];
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::metablock_manager_t::head_t::push() {
    saved_mb_slot = mb_slot;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::metablock_manager_t::head_t::pop() {
    guarantee(saved_mb_slot != (uint32_t) -1, "Popping without a saved state");
    mb_slot = saved_mb_slot;
    saved_mb_slot = -1;
}

template<class metablock_t>
metablock_manager_t<metablock_t>::metablock_manager_t(extent_manager_t *em)
    : head(this), extent_manager(em), state(state_unstarted), dbfile(NULL)
{    
    rassert(sizeof(crc_metablock_t) <= DEVICE_BLOCK_SIZE);
    mb_buffer = (crc_metablock_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    startup_values.mb_buffer_last = (crc_metablock_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    rassert(mb_buffer);
    mb_buffer_in_use = false;
    
    /* Build the list of metablock locations in the file */
    
    for (off64_t i = 0; i < MB_NEXTENTS; i++) {
        off64_t extent = i * extent_manager->extent_size * MB_EXTENT_SEPARATION;
        
        /* The reason why we don't reserve extent 0 is that it has already been reserved for the
        static header. We can share the first extent with the static header if and only if we don't
        overwrite the first DEVICE_BLOCK_SIZE of it, but we musn't reserve it again. */
        if (extent != 0) extent_manager->reserve_extent(extent);
    }

    initialize_metablock_offsets(extent_manager->extent_size, &metablock_offsets);
}

template<class metablock_t>
metablock_manager_t<metablock_t>::~metablock_manager_t() {
    
    rassert(state == state_unstarted || state == state_shut_down);
    
    rassert(!mb_buffer_in_use);
    free(mb_buffer);

    free(startup_values.mb_buffer_last); 
    startup_values.mb_buffer_last = NULL;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::create(direct_file_t *dbfile, off64_t extent_size, metablock_t *initial) {

    std::vector<off64_t> metablock_offsets;
    initialize_metablock_offsets(extent_size, &metablock_offsets);

    dbfile->set_size_at_least(metablock_offsets[metablock_offsets.size() - 1] + DEVICE_BLOCK_SIZE);

    /* Allocate a buffer for doing our writes */
    crc_metablock_t *buffer = reinterpret_cast<crc_metablock_t*>(malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE));
    bzero(buffer, DEVICE_BLOCK_SIZE);

    /* Wipe the metablock slots so we don't mistake something left by a previous database for a
    valid metablock. */
    for (unsigned i = 0; i < metablock_offsets.size(); i++) {
        co_write(dbfile, metablock_offsets[i], DEVICE_BLOCK_SIZE, buffer);
    }

    /* Write the first metablock */
    buffer->prepare(initial, MB_START_VERSION);
    co_write(dbfile, metablock_offsets[0], DEVICE_BLOCK_SIZE, buffer);

    free(buffer);
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::co_start_existing(direct_file_t *file, bool *mb_found, metablock_t *mb_out) {
    rassert(state == state_unstarted);
    dbfile = file;
    rassert(dbfile != NULL);
    
    rassert(!mb_buffer_in_use);
    mb_buffer_in_use = true;

    startup_values.version = MB_BAD_VERSION;
    
    dbfile->set_size_at_least(metablock_offsets[metablock_offsets.size() - 1] + DEVICE_BLOCK_SIZE);
    
    co_read(dbfile, head.offset(), DEVICE_BLOCK_SIZE, mb_buffer);
    
    state = state_reading;
    bool done_looking = false;
    while (!done_looking) {
        if (mb_buffer->check_crc()) {
            if (mb_buffer->version > startup_values.version) {
                /* this metablock is good, maybe there are more? */
                startup_values.version = mb_buffer->version;
                head.push();
                head++;
                /* mb_buffer_last = mb_buffer and give mb_buffer mb_buffer_last's space so no realloc */
                swap_buffers();
                done_looking = head.wraparound;
            } else {
                /* version smaller than the one we just had, yahtzee */
                done_looking = true;
            }
        } else {
            head++;
            done_looking = head.wraparound;
        }

        if (done_looking) {
            if (startup_values.version == MB_BAD_VERSION) {
                
                /* no metablock found anywhere -- the DB is toast */
                
                next_version_number = MB_START_VERSION;
                *mb_found = false;
                
                /* The log serializer will catastrophically fail when it sees that mb_found is
                false. We could catastrophically fail here, but it's a bit nicer to have the
                metablock manager as a standalone component that doesn't know how to behave if there
                is no metablock. */
                
            } else {
                /* we found a metablock */
                swap_buffers();

                /* set everything up */
                next_version_number = startup_values.version + 1;
                startup_values.version = MB_BAD_VERSION; /* version is now useless */
                head.pop();
                *mb_found = true;
                memcpy(mb_out, &(mb_buffer->metablock), sizeof(metablock_t));
            }
            mb_buffer_in_use = false;
            state = state_ready;
        } else {
            co_read(dbfile, head.offset(), DEVICE_BLOCK_SIZE, mb_buffer);
        }
    }
}

//The following two functions will go away in favor of the preceding one
template<class metablock_t>
void metablock_manager_t<metablock_t>::start_existing_callback(direct_file_t *file, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb) {
    co_start_existing(file, mb_found, mb_out);
    cb->on_metablock_read();
}

template<class metablock_t>
bool metablock_manager_t<metablock_t>::start_existing(direct_file_t *file, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb) {
    coro_t::spawn(boost::bind(&metablock_manager_t<metablock_t>::start_existing_callback, this, file, mb_found, mb_out, cb));
    return false;
}
template<class metablock_t>
void metablock_manager_t<metablock_t>::co_write_metablock(metablock_t *mb) {
    mutex_acquisition_t hold(&write_lock);

    rassert(state == state_ready);
    rassert(!mb_buffer_in_use);
    
    mb_buffer->prepare(mb, next_version_number++);
    rassert(mb_buffer->check_crc());

    mb_buffer_in_use = true;
    
    state = state_writing;
    co_write(dbfile, head.offset(), DEVICE_BLOCK_SIZE, mb_buffer);

    head++;

    state = state_ready;
    mb_buffer_in_use = false;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::write_metablock_callback(metablock_t *mb, metablock_write_callback_t *cb) {
    co_write_metablock(mb);
    cb->on_metablock_write();
}

template<class metablock_t>
bool metablock_manager_t<metablock_t>::write_metablock(metablock_t *mb, metablock_write_callback_t *cb) {
    coro_t::spawn(boost::bind(&metablock_manager_t<metablock_t>::write_metablock_callback, this, mb, cb));
    return false;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::shutdown() {
    
    rassert(state == state_ready);
    rassert(!mb_buffer_in_use);
    state = state_shut_down;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::swap_buffers() {
    crc_metablock_t *tmp = startup_values.mb_buffer_last;
    startup_values.mb_buffer_last = mb_buffer;
    mb_buffer = tmp;
}
