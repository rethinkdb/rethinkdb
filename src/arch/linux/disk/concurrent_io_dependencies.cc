#include <set>

#include "arch/linux/disk/concurrent_io_dependencies.hpp"

#include "errors.hpp"
#include "config/args.hpp"


bool concurrent_io_dependencies_t::is_conflicting(iocb *request) {
    const std::vector<size_t> affected_blocks = calculate_overlapping_blocks(request);

    if(request->aio_lio_opcode == IO_CMD_PREAD) {
        for (size_t i = 0; i < affected_blocks.size(); ++i) {
            // Check that no writer is active on that block
            if (active_writers[request->aio_fildes].find(affected_blocks[i]) != active_writers[request->aio_fildes].end()) {
                return true;
            }
        }
    } else if(request->aio_lio_opcode == IO_CMD_PWRITE) {
        for (size_t i = 0; i < affected_blocks.size(); ++i) {
            // Check that no writer is active on that block
            if (active_writers[request->aio_fildes].find(affected_blocks[i]) != active_writers[request->aio_fildes].end()) {
                return true;
            }
            // Check that no reader is active on that block
            if (active_readers[request->aio_fildes].find(affected_blocks[i]) != active_readers[request->aio_fildes].end()) {
                return true;
            }
        }
    } else {
        crash("Unhandled aio operation code");
    }
    return false;
}

void concurrent_io_dependencies_t::register_active_request(iocb *request) {
    assert(!is_conflicting(request));

    const std::vector<size_t> affected_blocks = calculate_overlapping_blocks(request);

    if(request->aio_lio_opcode == IO_CMD_PREAD) {
        for (size_t i = 0; i < affected_blocks.size(); ++i) {
            ++active_readers[request->aio_fildes][affected_blocks[i]];
        }
    } else if(request->aio_lio_opcode == IO_CMD_PWRITE) {
        for (size_t i = 0; i < affected_blocks.size(); ++i) {
            active_writers[request->aio_fildes].insert(affected_blocks[i]);
        }
    } else {
        crash("Unhandled aio operation code");
    }
}

void concurrent_io_dependencies_t::unregister_request(iocb *request) {
    const std::vector<size_t> affected_blocks = calculate_overlapping_blocks(request);

    if(request->aio_lio_opcode == IO_CMD_PREAD) {
        for (size_t i = 0; i < affected_blocks.size(); ++i) {
            --active_readers[request->aio_fildes][affected_blocks[i]];
            if (active_readers[request->aio_fildes][affected_blocks[i]] == 0)
                active_readers[request->aio_fildes].erase(affected_blocks[i]);
        }
    } else if(request->aio_lio_opcode == IO_CMD_PWRITE) {
        for (size_t i = 0; i < affected_blocks.size(); ++i) {
            active_writers[request->aio_fildes].erase(affected_blocks[i]);
        }
    } else {
        crash("Unhandled aio operation code");
    }
}

std::vector<size_t> concurrent_io_dependencies_t::calculate_overlapping_blocks(iocb* request) const {
    std::vector<size_t> result;
    const size_t first_block = (request->u.c.offset / DEVICE_BLOCK_SIZE) * DEVICE_BLOCK_SIZE;
    result.push_back(first_block);

    size_t bytes_left = request->u.c.nbytes - std::min((size_t)request->u.c.nbytes, ((size_t)DEVICE_BLOCK_SIZE - ((size_t)request->u.c.offset - first_block)));
    while (bytes_left > 0) {
        result.push_back(result[result.size()-1] + DEVICE_BLOCK_SIZE);
        bytes_left -= std::min((size_t)DEVICE_BLOCK_SIZE, bytes_left);
    }

    return result;
}
