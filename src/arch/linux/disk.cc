#include <algorithm>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "arch/linux/arch.hpp"
#include "config/args.hpp"
#include "utils2.hpp"
#include "logger.hpp"

// #define DEBUG_DUMP_WRITES 1

/* Disk file object */

perfmon_counter_t
    pm_io_reads_started("io_reads_started[ioreads]"),
    pm_io_writes_started("io_writes_started[iowrites]");

linux_direct_file_t::linux_direct_file_t(const char *path, int mode, bool is_really_direct)
    : fd(INVALID_FD)
{
    int res;
    
    // Determine if it is a block device
    
    struct stat64 file_stat;
    bzero((void*)&file_stat, sizeof(file_stat)); // make valgrind happy
    res = stat64(path, &file_stat);
    guarantee_err(res == 0 || errno == ENOENT, "Could not stat file '%s'", path);
    
    if (res == -1 && errno == ENOENT) {
        if (!(mode & mode_create)) {
            file_exists = false;
            return;
        }
        is_block = false;
    } else {
        is_block = S_ISBLK(file_stat.st_mode);
    }
    
    // Construct file flags
    
    int flags = O_CREAT | (is_really_direct ? O_DIRECT : 0) | O_LARGEFILE;
    
    if ((mode & mode_write) && (mode & mode_read)) flags |= O_RDWR;
    else if (mode & mode_write) flags |= O_WRONLY;
    else if (mode & mode_read) flags |= O_RDONLY;
    else crash("Bad file access mode.");

    
    // O_NOATIME requires owner or root privileges. This is a bit of a hack; we assume that
    // if we are opening a regular file, we are the owner, but if we are opening a block device,
    // we are not.
    if (!is_block) flags |= O_NOATIME;
    
    // Open the file
    
    fd = open(path, flags, 0644);
    if (fd == INVALID_FD)
        fail_due_to_user_error("Inaccessible database file: \"%s\": %s", path, strerror(errno));

    file_exists = true;
    
    // Determine the file size
    
    if (is_block) {
        res = ioctl(fd, BLKGETSIZE64, &file_size);
        guarantee_err(res != -1, "Could not determine block device size");
    } else {
        off64_t size = lseek64(fd, 0, SEEK_END);
        guarantee_err(size != -1, "Could not determine file size");
        res = lseek64(fd, 0, SEEK_SET);
        guarantee_err(res != -1, "Could not reset file position");
        
        file_size = size;
    }
}

bool linux_direct_file_t::exists() {
    return file_exists;
}

bool linux_direct_file_t::is_block_device() {
    return is_block;
}

size_t linux_direct_file_t::get_size() {
    return file_size;
}

void linux_direct_file_t::set_size(size_t size) {
    assert(!is_block);
    int res = ftruncate(fd, size);
    guarantee_err(res == 0, "Could not ftruncate()");
    file_size = size;
}

void linux_direct_file_t::set_size_at_least(size_t size) {
    if (is_block) {
        assert(file_size >= size);
    } else {
        /* Grow in large chunks at a time */
        if (file_size < size) {
            // TODO: we should make the growth rate of a db file
            // configurable.
            set_size(ceil_aligned(size, DEVICE_BLOCK_SIZE * 128));
        }
    }
}

bool linux_direct_file_t::read_async(size_t offset, size_t length, void *buf, linux_iocallback_t *callback) {
    verify(offset, length, buf);
    
    linux_io_calls_t *iosys = &linux_thread_pool_t::thread->iosys;
    
    // Prepare the request
    iocb *request = new iocb;
    io_prep_pread(request, fd, buf, length, offset);
#ifndef NO_EVENTFD
    io_set_eventfd(request, iosys->aio_notify_fd);
#endif
    request->data = callback;

    // Add it to a list of outstanding read requests
    iosys->io_requests.queue.push_back(request);
    
    pm_io_reads_started++;

    // Process whatever is left
    iosys->process_requests();
    
    return false;
}

bool linux_direct_file_t::write_async(size_t offset, size_t length, void *buf, linux_iocallback_t *callback) {
#ifdef DEBUG_DUMP_WRITES
    printf("--- WRITE BEGIN ---\n");
    print_backtrace(stdout);
    printf("\n");
    print_hd(buf, offset, length);
    printf("---- WRITE END ----\n\n");
#endif
    
    verify(offset, length, buf);
    
    linux_io_calls_t *iosys = &linux_thread_pool_t::thread->iosys;
    
    // Prepare the request
    iocb *request = new iocb;
    io_prep_pwrite(request, fd, buf, length, offset);
#ifndef NO_EVENTFD
    io_set_eventfd(request, iosys->aio_notify_fd);
#endif
    request->data = callback;

    // Add it to a list of outstanding write requests
    iosys->io_requests.queue.push_back(request);
    
    pm_io_writes_started++;
    
    // Process whatever is left
    iosys->process_requests();
    
    return false;
}

void linux_direct_file_t::read_blocking(size_t offset, size_t length, void *buf) {
    verify(offset, length, buf);
 tryagain:
    ssize_t res = pread(fd, buf, length, offset);
    if (res == -1 && errno == EINTR) {
        goto tryagain;
    }
    assert(size_t(res) == length, "Blocking read failed");
    (void)res;
}

void linux_direct_file_t::write_blocking(size_t offset, size_t length, void *buf) {
    verify(offset, length, buf);
 tryagain:
    ssize_t res = pwrite(fd, buf, length, offset);
    if (res == -1 && errno == EINTR) {
        goto tryagain;
    }
    assert(size_t(res) == length, "Blocking write failed");
    (void)res;
}

linux_direct_file_t::~linux_direct_file_t() {
    if (fd != INVALID_FD) close(fd);
}

void linux_direct_file_t::verify(size_t offset, size_t length, void *buf) {
    assert(buf);
    assert(offset + length <= file_size);
    assert((intptr_t)buf % DEVICE_BLOCK_SIZE == 0);
    assert(offset % DEVICE_BLOCK_SIZE == 0);
    assert(length % DEVICE_BLOCK_SIZE == 0);
}

