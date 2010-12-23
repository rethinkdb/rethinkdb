#include "arch/arch.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


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
    : head(this), extent_manager(em), state(state_unstarted), dbfile(NULL) {
    assert(sizeof(crc_metablock_t) <= DEVICE_BLOCK_SIZE);
    mb_buffer = (crc_metablock_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    startup_values.mb_buffer_last = (crc_metablock_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    assert(mb_buffer);
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
    assert(state == state_unstarted || state == state_shut_down);

    assert(!mb_buffer_in_use);
    free(mb_buffer);

    free(startup_values.mb_buffer_last);
    startup_values.mb_buffer_last = NULL;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::start_new(direct_file_t *file) {
    assert(state == state_unstarted);
    dbfile = file;
    assert(dbfile != NULL);

    dbfile->set_size_at_least(metablock_offsets[metablock_offsets.size() - 1] + DEVICE_BLOCK_SIZE);

    /* Wipe the metablock slots so we don't mistake something left by a previous database as a
    valid metablock. */
    bzero(mb_buffer, DEVICE_BLOCK_SIZE);
    for (unsigned i = 0; i < metablock_offsets.size(); i++) {
        dbfile->write_blocking(metablock_offsets[i], DEVICE_BLOCK_SIZE, mb_buffer);
    }

    next_version_number = MB_START_VERSION;

    state = state_ready;
}

template<class metablock_t>
bool metablock_manager_t<metablock_t>::start_existing(direct_file_t *file, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb) {
    assert(state == state_unstarted);
    dbfile = file;
    assert(dbfile != NULL);

    this->mb_found = mb_found;
    this->mb_out = mb_out;

    assert(!mb_buffer_in_use);
    mb_buffer_in_use = true;

    startup_values.version = MB_BAD_VERSION;

    dbfile->set_size_at_least(metablock_offsets[metablock_offsets.size() - 1] + DEVICE_BLOCK_SIZE);

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

        mb_buffer->metablock = *mb;
        memcpy(mb_buffer->magic_marker, MB_MARKER_MAGIC, sizeof(MB_MARKER_MAGIC));
        memcpy(mb_buffer->crc_marker, MB_MARKER_CRC, sizeof(MB_MARKER_CRC));
        memcpy(mb_buffer->version_marker, MB_MARKER_VERSION, sizeof(MB_MARKER_VERSION));
        mb_buffer->version = next_version_number++;

        mb_buffer->set_crc();
        assert(mb_buffer->check_crc());
        mb_buffer_in_use = true;

        dbfile->write_async(head.offset(), DEVICE_BLOCK_SIZE, mb_buffer, this);

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
    assert(!mb_buffer_in_use);
    state = state_shut_down;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::swap_buffers() {
    crc_metablock_t *tmp = startup_values.mb_buffer_last;
    startup_values.mb_buffer_last = mb_buffer;
    mb_buffer = tmp;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::on_io_complete(event_t *e) {
    bool done_looking = false; /* whether or not the value in mb_buffer_last is the real metablock */
    switch(state) {
        case state_reading:
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
                if (read_callback) read_callback->on_metablock_read();
            } else {
                dbfile->read_async(
                    head.offset(),
                    DEVICE_BLOCK_SIZE,
                    mb_buffer,
                    this);
            }
            break;
        case state_writing: {
            state = state_ready;
            mb_buffer_in_use = false;

            /* Store the write callback because when we call it, it might destroy us */
            metablock_write_callback_t *write_cb = write_callback;
            write_callback = NULL;

            /* Start the next write if there are more in the queue */
            if (!outstanding_writes.empty()) {
                metablock_write_req_t req = outstanding_writes.front();
                outstanding_writes.pop_front();
                write_metablock(req.mb, req.cb);
            }

            /* Call the write callback as the last step, because it might destroy us */
            if (write_cb) write_cb->on_metablock_write();

            break;
        }
        case state_unstarted:
        case state_ready:
        case state_shut_down:
        default:
            unreachable("Unexpected state.");
    }
}
