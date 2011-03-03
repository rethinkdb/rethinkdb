#include "arch/linux/disk/reordering_io_queue.hpp"

#include <limits>
#include "errors.hpp"
#include "config/args.hpp"
#include "reordering_io_queue.hpp"

reordering_io_queue_t::reordering_io_queue_t() {
    // Force recalculation on next pull
    batch_outstanding_reads = 0;
    batch_outstanding_writes = 0;
    batch_outstanding_conflicting = 0;
    queued_read_requests_size = 0;
    queued_write_requests_size = 0;
    queued_conflicting_requests_size = 0;
}

reordering_io_queue_t::~reordering_io_queue_t() {
    rassert(empty());
}

void reordering_io_queue_t::push(iocb *request) {
    if(request->aio_lio_opcode == IO_CMD_PREAD) {
        push_read(request);
    } else {
        rassert(request->aio_lio_opcode == IO_CMD_PWRITE);
        push_write(request);
    }
}

bool reordering_io_queue_t::empty() {
    return queued_read_requests.empty() && queued_write_requests.empty() && queued_conflicting_requests.empty();
}

iocb *reordering_io_queue_t::peek() {
    rassert(!empty());

    switch (pick_next_queue(false)) {
        case READQ:
            return queued_read_requests.front();
        case WRITEQ:
            return queued_write_requests.front();
        case CONFLICTINGQ:
            return queued_conflicting_requests.front();
        default:
            crash("illegal queue_type");
    }
}

iocb *reordering_io_queue_t::pull() {
    switch (pick_next_queue(true)) {
        case READQ:
            return pull_read();
        case WRITEQ:
            return pull_write();
        case CONFLICTINGQ:
            return pull_conflicting();
        default:
            crash("illegal queue_type");
    }
}

void reordering_io_queue_t::push_read(iocb *request) {
    const std::pair<size_t, size_t> affected_blocks = calculate_overlapping_blocks(request);

    // Find out if we have a conflict (and if we have: change previously non-conflicting requests into conflicting ones)
    bool conflicting = false;
    for (size_t block_address = affected_blocks.first, i = 0; i < affected_blocks.second; ++i) {
        // Please note: don't break in this loop! We still have to move all conflicting writes into the conflicting queue...

        std::map<size_t, size_t>::const_iterator conflicting_counter = in_conflicting_state[request->aio_fildes].find(block_address);
        if (conflicting_counter != in_conflicting_state[request->aio_fildes].end()) {
            rassert(conflicting_counter->second > 0);

            conflicting = true;
        }

        std::map<size_t, std::list<iocb *>::iterator>::iterator conflicting_write = non_conflicting_write_request[request->aio_fildes].find(block_address);
        if (conflicting_write != non_conflicting_write_request[request->aio_fildes].end()) {
            // Move the now conflicting write to the conflicting queue
            queued_conflicting_requests.push_back(*conflicting_write->second);
            ++queued_conflicting_requests_size;
            queued_write_requests.erase(conflicting_write->second);
            --queued_write_requests_size;
            ++in_conflicting_state[request->aio_fildes][block_address];
            non_conflicting_write_request[request->aio_fildes].erase(conflicting_write);

            conflicting = true;
        }

        block_address += DEVICE_BLOCK_SIZE;
    }

    if (conflicting) {
        queued_conflicting_requests.push_back(request);
        ++queued_conflicting_requests_size;

        // Book keeping...
        for (size_t block_address = affected_blocks.first, i = 0; i < affected_blocks.second; ++i) {
            ++in_conflicting_state[request->aio_fildes][block_address];
            block_address += DEVICE_BLOCK_SIZE;
        }
    } else {
        queued_read_requests.push_back(request);
        ++queued_read_requests_size;
        std::list<iocb *>::iterator queued_request_iterator = --queued_read_requests.end();

        // Book keeping...
        for (size_t block_address = affected_blocks.first, i = 0; i < affected_blocks.second; ++i) {
            non_conflicting_read_requests[request->aio_fildes][block_address].push_back(queued_request_iterator);
            block_address += DEVICE_BLOCK_SIZE;
        }
    }
}

void reordering_io_queue_t::push_write(iocb *request) {
    const std::pair<size_t, size_t> affected_blocks = calculate_overlapping_blocks(request);

    // Find out if we have a conflict (and if we have: change previously non-conflicting requests into conflicting ones)
    bool conflicting = false;
    for (size_t block_address = affected_blocks.first, i = 0; i < affected_blocks.second; ++i) {
        // Please note: don't break in this loop! We still have to move all conflicting writes/reads into the conflicting queue...

        std::map<size_t, size_t>::const_iterator conflicting_counter = in_conflicting_state[request->aio_fildes].find(block_address);
        if (conflicting_counter != in_conflicting_state[request->aio_fildes].end()) {
            rassert(conflicting_counter->second > 0);

            conflicting = true;
        }

        std::map<size_t, std::list<iocb *>::iterator>::iterator conflicting_write = non_conflicting_write_request[request->aio_fildes].find(block_address);
        if (conflicting_write != non_conflicting_write_request[request->aio_fildes].end()) {
            // Move the now conflicting write to the conflicting queue
            queued_conflicting_requests.push_back(*conflicting_write->second);
            ++queued_conflicting_requests_size;
            queued_write_requests.erase(conflicting_write->second);
            --queued_write_requests_size;
            ++in_conflicting_state[request->aio_fildes][block_address];
            non_conflicting_write_request[request->aio_fildes].erase(conflicting_write);

            conflicting = true;
        }

        std::map<size_t, std::list<std::list<iocb *>::iterator> >::iterator conflicting_reads = non_conflicting_read_requests[request->aio_fildes].find(block_address);
        if (conflicting_reads != non_conflicting_read_requests[request->aio_fildes].end()) {
            // Move the now conflicting reads to the conflicting queue
            for (std::list<std::list<iocb *>::iterator>::iterator conflicting_read = conflicting_reads->second.begin(); conflicting_read != conflicting_reads->second.end(); ++conflicting_read) {
                queued_conflicting_requests.push_back(**conflicting_read);
                ++queued_conflicting_requests_size;
                queued_read_requests.erase(*conflicting_read);
                --queued_read_requests_size;
                ++in_conflicting_state[request->aio_fildes][block_address];
            }
            non_conflicting_read_requests[request->aio_fildes].erase(conflicting_reads);

            conflicting = true;
        }

        block_address += DEVICE_BLOCK_SIZE;
    }

    if (conflicting) {
        queued_conflicting_requests.push_back(request);
        ++queued_conflicting_requests_size;

        // Book keeping...
        for (size_t block_address = affected_blocks.first, i = 0; i < affected_blocks.second; ++i) {
            ++in_conflicting_state[request->aio_fildes][block_address];
            block_address += DEVICE_BLOCK_SIZE;
        }
    } else {
        queued_write_requests.push_back(request);
        ++queued_write_requests_size;
        std::list<iocb *>::iterator queued_request_iterator = --queued_write_requests.end();

        // Book keeping...
        for (size_t block_address = affected_blocks.first, i = 0; i < affected_blocks.second; ++i) {
            rassert(non_conflicting_write_request[request->aio_fildes].find(block_address) == non_conflicting_write_request[request->aio_fildes].end());
            non_conflicting_write_request[request->aio_fildes][block_address] = queued_request_iterator;
            block_address += DEVICE_BLOCK_SIZE;
        }
    }
}

iocb *reordering_io_queue_t::pull_read() {
    iocb *request = queued_read_requests.front();
    const std::pair<size_t, size_t> affected_blocks = calculate_overlapping_blocks(request);

    // Book keeping...
    for (size_t block_address = affected_blocks.first, i = 0; i < affected_blocks.second; ++i) {
        rassert(non_conflicting_read_requests[request->aio_fildes][block_address].front() == queued_read_requests.begin());
        non_conflicting_read_requests[request->aio_fildes][block_address].erase(non_conflicting_read_requests[request->aio_fildes][block_address].begin());
        if (non_conflicting_read_requests[request->aio_fildes][block_address].empty()) {
            non_conflicting_read_requests[request->aio_fildes].erase(block_address);
        }
        block_address += DEVICE_BLOCK_SIZE;
    }

    queued_read_requests.erase(queued_read_requests.begin());
    --queued_read_requests_size;
    return request;
}

iocb *reordering_io_queue_t::pull_write() {
    iocb *request = queued_write_requests.front();
    const std::pair<size_t, size_t> affected_blocks = calculate_overlapping_blocks(request);

    // Book keeping...
    for (size_t block_address = affected_blocks.first, i = 0; i < affected_blocks.second; ++i) {
        rassert(non_conflicting_write_request[request->aio_fildes][block_address] == queued_write_requests.begin());
        non_conflicting_write_request[request->aio_fildes].erase(block_address);
        block_address += DEVICE_BLOCK_SIZE;
    }

    queued_write_requests.erase(queued_write_requests.begin());
    --queued_write_requests_size;
    return request;
}

iocb *reordering_io_queue_t::pull_conflicting() {
    iocb *request = queued_conflicting_requests.front();
    const std::pair<size_t, size_t> affected_blocks = calculate_overlapping_blocks(request);

    // Book keeping...
    for (size_t block_address = affected_blocks.first, i = 0; i < affected_blocks.second; ++i) {
        rassert(in_conflicting_state[request->aio_fildes][block_address] > 0);
        --in_conflicting_state[request->aio_fildes][block_address];
        if (in_conflicting_state[request->aio_fildes][block_address] == 0) {
            in_conflicting_state[request->aio_fildes].erase(block_address);
        }
        block_address += DEVICE_BLOCK_SIZE;
    }

    queued_conflicting_requests.erase(queued_conflicting_requests.begin());
    --queued_conflicting_requests_size;
    return request;
}

reordering_io_queue_t::queue_types_t reordering_io_queue_t::pick_next_queue(bool remove_from_outstanding) {
    rassert(!empty());

    if (batch_outstanding_reads > 0 && queued_read_requests_size > 0) {
        if (remove_from_outstanding) {
            --batch_outstanding_reads;
        }
        return READQ;
    }
    else if (batch_outstanding_writes > 0 && queued_write_requests_size > 0) {
        if (remove_from_outstanding) {
            --batch_outstanding_writes;
        }
        return WRITEQ;
    }
    else if (batch_outstanding_conflicting > 0 && queued_conflicting_requests_size > 0) {
        if (remove_from_outstanding) {
            --batch_outstanding_conflicting;
        }
        return CONFLICTINGQ;
    }
    else {
        // Start next batch of (approximately) size 32
        // (we use a smaller batch size to get a finer blend of operations)
        const size_t batch_size = 32;
        const size_t total_n_requests = queued_read_requests_size + queued_write_requests_size + queued_conflicting_requests_size;
        batch_outstanding_reads = std::max((size_t)(batch_size / 2), queued_read_requests_size * batch_size / total_n_requests);
        batch_outstanding_reads = std::max((size_t)1, queued_read_requests_size * batch_size / total_n_requests);
        batch_outstanding_writes = std::max((size_t)1, queued_write_requests_size * batch_size / total_n_requests);
        batch_outstanding_conflicting = std::max((size_t)1, queued_conflicting_requests_size * batch_size / total_n_requests);

        return pick_next_queue(remove_from_outstanding);
    }
}

// Returns the offset of the first overlapping block and the number of blocks overlapped
std::pair<size_t, size_t> reordering_io_queue_t::calculate_overlapping_blocks(iocb* request) const {
    const size_t first_block = (request->u.c.offset / DEVICE_BLOCK_SIZE) * DEVICE_BLOCK_SIZE;
    size_t bytes_left = request->u.c.nbytes - std::min((size_t)request->u.c.nbytes, ((size_t)DEVICE_BLOCK_SIZE - ((size_t)request->u.c.offset - first_block)));
    size_t n_blocks = 1;
    while (bytes_left > 0) {
        ++n_blocks;
        bytes_left -= std::min((size_t)DEVICE_BLOCK_SIZE, bytes_left);
    }
    return std::pair<size_t, size_t>(first_block, n_blocks);
}
