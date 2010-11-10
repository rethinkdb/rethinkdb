// Copyright (c) 2007, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
// Author: Arun Sharma
//
// A tcmalloc system allocator that uses a memory based filesystem such as
// tmpfs or hugetlbfs
//
// Since these only exist on linux, we only register this allocator there.

#ifdef __linux

#include <config.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/vfs.h>           // for statfs
#include <string>

#include "base/basictypes.h"
#include "base/googleinit.h"
#include "base/sysinfo.h"
#include "system-alloc.h"
#include "internal_logging.h"

using std::string;

DEFINE_string(memfs_malloc_path, EnvToString("TCMALLOC_MEMFS_MALLOC_PATH", ""),
              "Path where hugetlbfs or tmpfs is mounted. The caller is "
              "responsible for ensuring that the path is unique and does "
              "not conflict with another process");
DEFINE_int64(memfs_malloc_limit_mb,
             EnvToInt("TCMALLOC_MEMFS_LIMIT_MB", 0),
             "Limit total allocation size to the "
             "specified number of MiB.  0 == no limit.");
DEFINE_bool(memfs_malloc_abort_on_fail,
            EnvToBool("TCMALLOC_MEMFS_ABORT_ON_FAIL", false),
            "abort() whenever memfs_malloc fails to satisfy an allocation "
            "for any reason.");
DEFINE_bool(memfs_malloc_ignore_mmap_fail,
            EnvToBool("TCMALLOC_MEMFS_IGNORE_MMAP_FAIL", false),
            "Ignore failures from mmap");

// Hugetlbfs based allocator for tcmalloc
class HugetlbSysAllocator: public SysAllocator {
public:
  HugetlbSysAllocator(int fd, int page_size)
    : big_page_size_(page_size),
      hugetlb_fd_(fd),
      hugetlb_base_(0) {
  }

  void* Alloc(size_t size, size_t *actual_size, size_t alignment);

  void DumpStats(TCMalloc_Printer* printer);

private:
  int64 big_page_size_;
  int hugetlb_fd_;      // file descriptor for hugetlb
  off_t hugetlb_base_;
};

void HugetlbSysAllocator::DumpStats(TCMalloc_Printer* printer) {
  printer->printf("HugetlbSysAllocator: failed_=%d allocated=%"PRId64"\n",
                  failed_, static_cast<int64_t>(hugetlb_base_));
}

// No locking needed here since we assume that tcmalloc calls
// us with an internal lock held (see tcmalloc/system-alloc.cc).
void* HugetlbSysAllocator::Alloc(size_t size, size_t *actual_size,
                                 size_t alignment) {

  // don't go any further if we haven't opened the backing file
  if (hugetlb_fd_ == -1) {
    return NULL;
  }

  // We don't respond to allocation requests smaller than big_page_size_ unless
  // the caller is willing to take more than they asked for.
  if (actual_size == NULL && size < big_page_size_) {
    return NULL;
  }

  // Enforce huge page alignment.  Be careful to deal with overflow.
  if (alignment < big_page_size_) alignment = big_page_size_;
  size_t aligned_size = ((size + alignment - 1) / alignment) * alignment;
  if (aligned_size < size) {
    return NULL;
  }
  size = aligned_size;

  // Ask for extra memory if alignment > pagesize
  size_t extra = 0;
  if (alignment > big_page_size_) {
    extra = alignment - big_page_size_;
  }

  // Test if this allocation would put us over the limit.
  off_t limit = FLAGS_memfs_malloc_limit_mb*1024*1024;
  if (limit > 0 && hugetlb_base_ + size + extra > limit) {
    // Disable the allocator when there's less than one page left.
    if (limit - hugetlb_base_ < big_page_size_) {
      TCMalloc_MESSAGE(__FILE__, __LINE__, "reached memfs_malloc_limit_mb\n");
      failed_ = true;
    }
    else {
      TCMalloc_MESSAGE(__FILE__, __LINE__, "alloc size=%"PRIuS
                       " too large while %"PRId64" bytes remain\n",
                       size, static_cast<int64_t>(limit - hugetlb_base_));
    }
    if (FLAGS_memfs_malloc_abort_on_fail) {
      CRASH("memfs_malloc_abort_on_fail is set\n");
    }
    return NULL;
  }

  // This is not needed for hugetlbfs, but needed for tmpfs.  Annoyingly
  // hugetlbfs returns EINVAL for ftruncate.
  int ret = ftruncate(hugetlb_fd_, hugetlb_base_ + size + extra);
  if (ret != 0 && errno != EINVAL) {
    TCMalloc_MESSAGE(__FILE__, __LINE__, "ftruncate failed: %s\n",
                     strerror(errno));
    failed_ = true;
    if (FLAGS_memfs_malloc_abort_on_fail) {
      CRASH("memfs_malloc_abort_on_fail is set\n");
    }
    return NULL;
  }

  // Note: size + extra does not overflow since:
  //            size + alignment < (1<<NBITS).
  // and        extra <= alignment
  // therefore  size + extra < (1<<NBITS)
  void *result = mmap(0, size + extra, PROT_WRITE|PROT_READ,
                      MAP_SHARED, hugetlb_fd_, hugetlb_base_);
  if (result == reinterpret_cast<void*>(MAP_FAILED)) {
    if (!FLAGS_memfs_malloc_ignore_mmap_fail) {
      TCMalloc_MESSAGE(__FILE__, __LINE__, "mmap of size %"PRIuS" failed: %s\n",
                       size + extra, strerror(errno));
      failed_ = true;
      if (FLAGS_memfs_malloc_abort_on_fail) {
        CRASH("memfs_malloc_abort_on_fail is set\n");
      }
    }
    return NULL;
  }
  uintptr_t ptr = reinterpret_cast<uintptr_t>(result);

  // Adjust the return memory so it is aligned
  size_t adjust = 0;
  if ((ptr & (alignment - 1)) != 0) {
    adjust = alignment - (ptr & (alignment - 1));
  }
  ptr += adjust;
  hugetlb_base_ += (size + extra);

  if (actual_size) {
    *actual_size = size + extra - adjust;
  }

  return reinterpret_cast<void*>(ptr);
}

static void InitSystemAllocator() {
  if (FLAGS_memfs_malloc_path.length()) {
    char path[PATH_MAX];
    int rc = snprintf(path, sizeof(path), "%s.XXXXXX",
                      FLAGS_memfs_malloc_path.c_str());
    if (rc < 0 || rc >= sizeof(path)) {
      CRASH("XX fatal: memfs_malloc_path too long\n");
    }

    int hugetlb_fd = mkstemp(path);
    if (hugetlb_fd == -1) {
      TCMalloc_MESSAGE(__FILE__, __LINE__,
                       "warning: unable to create memfs_malloc_path %s: %s\n",
                       path, strerror(errno));
      return;
    }

    // Cleanup memory on process exit
    if (unlink(path) == -1) {
      CRASH("fatal: error unlinking memfs_malloc_path %s: %s\n",
            path, strerror(errno));
    }

    // Use fstatfs to figure out the default page size for memfs
    struct statfs sfs;
    if (fstatfs(hugetlb_fd, &sfs) == -1) {
      CRASH("fatal: error fstatfs of memfs_malloc_path: %s\n",
            strerror(errno));
    }
    int64 page_size = sfs.f_bsize;

    SysAllocator *alloc = new HugetlbSysAllocator(hugetlb_fd, page_size);
    // Register ourselves with tcmalloc
    RegisterSystemAllocator(alloc, 0);
  }
}

REGISTER_MODULE_INITIALIZER(memfs_malloc, { InitSystemAllocator(); });

#endif   /* ifdef __linux */
