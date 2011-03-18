#ifndef __ARCH_LINUX_DISK_DISKMGR_HPP__
#define __ARCH_LINUX_DISK_DISKMGR_HPP__

#include "arch/linux/disk.hpp"   /* For linux_disk_op_t */

struct linux_diskmgr_t {

    virtual ~linux_diskmgr_t() { }

    virtual void pread(fd_t fd, void *buf, size_t count, off_t offset, linux_disk_op_t *cb) = 0;
    virtual void pwrite(fd_t fd, const void *buf, size_t count, off_t offset, linux_disk_op_t *cb) = 0;
};

#endif /* __ARCH_LINUX_DISK_DISKMGR_HPP__ */
