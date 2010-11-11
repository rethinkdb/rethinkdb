// Copyright (c) 2000, Google Inc.
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
// Author: Urs Holzle <opensource@google.com>

#include "config.h"
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <pthread.h>
#include <stdio.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <stdarg.h>
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>

#include "base/commandlineflags.h"
#include "base/googleinit.h"
#include "base/logging.h"
#include "google/malloc_extension.h"
#include "google/malloc_hook.h"
#include "google/stacktrace.h"
#include "addressmap-inl.h"
#include "malloc_hook-inl.h"
#include "symbolize.h"

#ifdef TCMALLOC_FOR_DEBUGALLOCATION
#include "tcmalloc.cc"
#else
#include "base/spinlock.h"
// Else we already have a SpinLock defined in tcmalloc/internal_spinlock.h
#endif

// __THROW is defined in glibc systems.  It means, counter-intuitively,
// "This function will never throw an exception."  It's an optional
// optimization tool, but we may need to use it to match glibc prototypes.
#ifndef __THROW    // I guess we're not on a glibc system
# define __THROW   // __THROW is just an optimization, so ok to make it ""
#endif

// On systems (like freebsd) that don't define MAP_ANONYMOUS, use the old
// form of the name instead.
#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

// ========================================================================= //

DEFINE_bool(malloctrace,
            EnvToBool("TCMALLOC_TRACE", false),
            "Enables memory (de)allocation tracing to /tmp/google.alloc.");
#ifdef HAVE_MMAP
DEFINE_bool(malloc_page_fence,
            EnvToBool("TCMALLOC_PAGE_FENCE", false),
            "Enables putting of memory allocations at page boundaries "
            "with a guard page following the allocation (to catch buffer "
            "overruns right when they happen).");
DEFINE_bool(malloc_page_fence_never_reclaim,
            EnvToBool("TCMALLOC_PAGE_FRANCE_NEVER_RECLAIM", false),
            "Enables making the virtual address space inaccessible "
            "upon a deallocation instead of returning it and reusing later.");
#else
DEFINE_bool(malloc_page_fence, false, "Not usable (requires mmap)");
DEFINE_bool(malloc_page_fence_never_reclaim, false, "Not usable (required mmap)");
#endif
DEFINE_bool(malloc_reclaim_memory,
            EnvToBool("TCMALLOC_RECLAIM_MEMORY", true),
            "If set to false, we never return memory to malloc "
            "when an object is deallocated. This ensures that all "
            "heap object addresses are unique.");
DEFINE_int32(max_free_queue_size,
             EnvToInt("TCMALLOC_MAX_FREE_QUEUE_SIZE", 10*1024*1024),
             "If greater than 0, keep freed blocks in a queue instead of "
             "releasing them to the allocator immediately.  Release them when "
             "the total size of all blocks in the queue would otherwise exceed "
             "this limit.");

DEFINE_bool(symbolize_stacktrace,
            EnvToBool("TCMALLOC_SYMBOLIZE_STACKTRACE", true),
            "Symbolize the stack trace when provided (on some error exits)");

// ========================================================================= //

// A safe version of printf() that does not do any allocation and
// uses very little stack space.
static void TracePrintf(int fd, const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));

//
// GNU has some weird "weak aliasing" thing that permits us to define our
// own malloc(), free(), and realloc() which can use the normal versions of
// of themselves by calling __libc_malloc(), __libc_free(), and
// __libc_realloc().
//
extern "C" {
  extern void* __libc_malloc(size_t size);
  extern void __libc_free(void* ptr);
  extern void* __libc_realloc(void* ptr, size_t size);
  extern void* __libc_calloc(size_t nmemb, size_t size);
  extern int __libc_mallopt(int cmd, int value);
#ifdef HAVE_STRUCT_MALLINFO
  extern struct mallinfo __libc_mallinfo(void);
#endif
}

// Define the malloc/free/mallopt/mallinfo implementations
// we will be working on top of.
// TODO(csilvers): provide debugallocation on top of libc alloc,
//                 so this #ifdef might sometimes be false.
#ifdef TCMALLOC_FOR_DEBUGALLOCATION

// The do_* functions are defined in tcmalloc.cc,
// which is included before this file
// when TCMALLOC_FOR_DEBUGALLOCATION is defined.
#define BASE_MALLOC_NEW(size)  cpp_alloc(size, false)
#define BASE_MALLOC      do_malloc_or_cpp_alloc
#define BASE_FREE        do_free
#define BASE_MALLOPT     do_mallopt
#define BASE_MALLINFO    do_mallinfo

#else

// We are working on top of standard libc's malloc library
#define BASE_MALLOC_NEW  __libc_malloc
#define BASE_MALLOC      __libc_malloc
#define BASE_FREE        __libc_free
#define BASE_MALLOPT     __libc_mallopt
#define BASE_MALLINFO    __libc_mallinfo

#endif

// ========================================================================= //

class MallocBlock;

// A circular buffer to hold freed blocks of memory.  MallocBlock::Deallocate
// (below) pushes blocks into this queue instead of returning them to the
// underlying allocator immediately.  See MallocBlock::Deallocate for more
// information.
//
// We can't use an STL class for this because we need to be careful not to
// perform any heap de-allocations in any of the code in this class, since the
// code in MallocBlock::Deallocate is not re-entrant.
template <typename QueueEntry>
class FreeQueue {
 public:
  FreeQueue() : q_front_(0), q_back_(0) {}

  bool Full() {
    return (q_front_ + 1) % kFreeQueueSize == q_back_;
  }

  void Push(QueueEntry block) {
    q_[q_front_] = block;
    q_front_ = (q_front_ + 1) % kFreeQueueSize;
  }

  QueueEntry Pop() {
    const QueueEntry& ret = q_[q_back_];
    q_back_ = (q_back_ + 1) % kFreeQueueSize;
    return ret;
  }

  size_t size() const {
    return (q_front_ - q_back_ + kFreeQueueSize) % kFreeQueueSize;
  }

 private:
  // Maximum number of blocks kept in the free queue before being freed.
  static const int kFreeQueueSize = 1024;

  QueueEntry q_[kFreeQueueSize];
  int q_front_;
  int q_back_;
};

struct MallocBlockQueueEntry {
  MallocBlockQueueEntry() : block(NULL), size(0),
                            num_deleter_pcs(0), deleter_threadid(0) {}
  MallocBlockQueueEntry(MallocBlock* b, size_t s) : block(b), size(s) {
    if (FLAGS_max_free_queue_size != 0) {
      // Adjust the number of frames to skip (4) if you change the
      // location of this call.
      num_deleter_pcs =
          GetStackTrace(deleter_pcs,
                        sizeof(deleter_pcs) / sizeof(deleter_pcs[0]),
                        4);
      deleter_threadid = pthread_self();
    } else {
      num_deleter_pcs = 0;
      // Zero is an illegal pthread id by my reading of the pthread
      // implementation:
      deleter_threadid = 0;
    }
  }

  MallocBlock* block;
  size_t size;

  // When deleted and put in the free queue, we (flag-controlled)
  // record the stack so that if corruption is later found, we can
  // print the deleter's stack.  (These three vars add 144 bytes of
  // overhead under the LP64 data model.)
  void* deleter_pcs[16];
  int num_deleter_pcs;
  pthread_t deleter_threadid;
};

class MallocBlock {
 public:  // allocation type constants

  // Different allocation types we distinguish.
  // Note: The lower 4 bits are not random: we index kAllocName array
  // by these values masked with kAllocTypeMask;
  // the rest are "random" magic bits to help catch memory corruption.
  static const int kMallocType = 0xEFCDAB90;
  static const int kNewType = 0xFEBADC81;
  static const int kArrayNewType = 0xBCEADF72;

 private:  // constants

  // A mask used on alloc types above to get to 0, 1, 2
  static const int kAllocTypeMask = 0x3;
  // An additional bit to set in AllocType constants
  // to mark now deallocated regions.
  static const int kDeallocatedTypeBit = 0x4;

  // For better memory debugging, we initialize all storage to known
  // values, and overwrite the storage when it's deallocated:
  // Byte that fills uninitialized storage.
  static const int kMagicUninitializedByte = 0xAB;
  // Byte that fills deallocated storage.
  // NOTE: tcmalloc.cc depends on the value of kMagicDeletedByte
  //       to work around a bug in the pthread library.
  static const int kMagicDeletedByte = 0xCD;
  // An int (type of alloc_type_ below) in a deallocated storage
  // filled with kMagicDeletedByte.
  static const int kMagicDeletedInt = 0xCDCDCDCD | ((0xCDCDCDCD << 16) << 16);
    // Initializer works for 32 and 64 bit ints;
    // "<< 16 << 16" is to fool gcc from issuing a warning
    // when ints are 32 bits.

  // NOTE: on Linux, you can enable malloc debugging support in libc by
  // setting the environment variable MALLOC_CHECK_ to 1 before you
  // start the program (see man malloc).

  // We use either BASE_MALLOC or mmap to make the actual allocation. In
  // order to remember which one of the two was used for any block, we store an
  // appropriate magic word next to the block.
  static const int kMagicMalloc = 0xDEADBEEF;
  static const int kMagicMMap = 0xABCDEFAB;

  // This array will be filled with 0xCD, for use with memcmp.
  static unsigned char kMagicDeletedBuffer[1024];
  static bool deleted_buffer_initialized_;

 private:  // data layout

                    // The four fields size1_,offset_,magic1_,alloc_type_
                    // should together occupy a multiple of 8 bytes.
  size_t size1_;
  size_t offset_;   // normally 0 unless memaligned memory
                    // see comments in memalign() and FromRawPointer().
  int magic1_;
  int alloc_type_;
  // here comes the actual data (variable length)
  // ...
  // then come the size2_ and magic2_, or a full page of mprotect-ed memory
  // if the malloc_page_fence feature is enabled.
  size_t size2_;
  int magic2_;

 private:  // static data and helpers

  // Allocation map: stores the allocation type for each allocated object,
  // or the type or'ed with kDeallocatedTypeBit
  // for each formerly allocated object.
  typedef AddressMap<int> AllocMap;
  static AllocMap* alloc_map_;
  // This protects alloc_map_ and consistent state of metadata
  // for each still-allocated object in it.
  // We use spin locks instead of pthread_mutex_t locks
  // to prevent crashes via calls to pthread_mutex_(un)lock
  // for the (de)allocations coming from pthreads initialization itself.
  static SpinLock alloc_map_lock_;

  // A queue of freed blocks.  Instead of releasing blocks to the allocator
  // immediately, we put them in a queue, freeing them only when necessary
  // to keep the total size of all the freed blocks below the limit set by
  // FLAGS_max_free_queue_size.
  static FreeQueue<MallocBlockQueueEntry>* free_queue_;

  static size_t free_queue_size_;  // total size of blocks in free_queue_
  // protects free_queue_ and free_queue_size_
  static SpinLock free_queue_lock_;

  // Names of allocation types (kMallocType, kNewType, kArrayNewType)
  static const char* const kAllocName[];
  // Names of corresponding deallocation types
  static const char* const kDeallocName[];

  static const char* AllocName(int type) {
    return kAllocName[type & kAllocTypeMask];
  }

  static const char* DeallocName(int type) {
    return kDeallocName[type & kAllocTypeMask];
  }

 private:  // helper accessors

  bool IsMMapped() const { return kMagicMMap == magic1_; }

  bool IsValidMagicValue(int value) const {
    return kMagicMMap == value  ||  kMagicMalloc == value;
  }

  static size_t real_malloced_size(size_t size) {
    return size + sizeof(MallocBlock);
  }
  static size_t real_mmapped_size(size_t size) {
    return size + MallocBlock::data_offset();
  }

  size_t real_size() {
    return IsMMapped() ? real_mmapped_size(size1_) : real_malloced_size(size1_);
  }

  // NOTE: if the block is mmapped (that is, we're using the
  // malloc_page_fence option) then there's no size2 or magic2
  // (instead, the guard page begins where size2 would be).

  size_t* size2_addr() { return (size_t*)((char*)&size2_ + size1_); }
  const size_t* size2_addr() const {
    return (const size_t*)((char*)&size2_ + size1_);
  }

  int* magic2_addr() { return (int*)(size2_addr() + 1); }
  const int* magic2_addr() const { return (const int*)(size2_addr() + 1); }

 private:  // other helpers

  void Initialize(size_t size, int type) {
    RAW_CHECK(IsValidMagicValue(magic1_), "");
    // record us as allocated in the map
    alloc_map_lock_.Lock();
    if (!alloc_map_) {
      void* p = BASE_MALLOC(sizeof(AllocMap));
      alloc_map_ = new(p) AllocMap(BASE_MALLOC, BASE_FREE);
    }
    alloc_map_->Insert(data_addr(), type);
    // initialize us
    size1_ = size;
    offset_ = 0;
    alloc_type_ = type;
    if (!IsMMapped()) {
      *magic2_addr() = magic1_;
      *size2_addr() = size;
    }
    alloc_map_lock_.Unlock();
    memset(data_addr(), kMagicUninitializedByte, size);
    if (!IsMMapped()) {
      RAW_CHECK(size1_ == *size2_addr(), "should hold");
      RAW_CHECK(magic1_ == *magic2_addr(), "should hold");
    }
  }

  size_t CheckAndClear(int type) {
    alloc_map_lock_.Lock();
    CheckLocked(type);
    if (!IsMMapped()) {
      RAW_CHECK(size1_ == *size2_addr(), "should hold");
    }
    // record us as deallocated in the map
    alloc_map_->Insert(data_addr(), type | kDeallocatedTypeBit);
    alloc_map_lock_.Unlock();
    // clear us
    const size_t size = real_size();
    memset(this, kMagicDeletedByte, size);
    return size;
  }

  void CheckLocked(int type) const {
    int map_type = 0;
    const int* found_type =
      alloc_map_ != NULL ? alloc_map_->Find(data_addr()) : NULL;
    if (found_type == NULL) {
      RAW_LOG(FATAL, "memory allocation bug: object at %p "
                     "has never been allocated", data_addr());
    } else {
      map_type = *found_type;
    }
    if ((map_type & kDeallocatedTypeBit) != 0) {
      RAW_LOG(FATAL, "memory allocation bug: object at %p "
                     "has been already deallocated (it was allocated with %s)",
                     data_addr(), AllocName(map_type & ~kDeallocatedTypeBit));
    }
    if (alloc_type_ == kMagicDeletedInt) {
      RAW_LOG(FATAL, "memory stomping bug: a word before object at %p "
                     "has been corrupted; or else the object has been already "
                     "deallocated and our memory map has been corrupted",
                     data_addr());
    }
    if (!IsValidMagicValue(magic1_)) {
      RAW_LOG(FATAL, "memory stomping bug: a word before object at %p "
                     "has been corrupted; "
                     "or else our memory map has been corrupted and this is a "
                     "deallocation for not (currently) heap-allocated object",
                     data_addr());
    }
    if (!IsMMapped()) {
      if (size1_ != *size2_addr()) {
        RAW_LOG(FATAL, "memory stomping bug: a word after object at %p "
                       "has been corrupted", data_addr());
      }
      if (!IsValidMagicValue(*magic2_addr())) {
        RAW_LOG(FATAL, "memory stomping bug: a word after object at %p "
                "has been corrupted", data_addr());
      }
    }
    if (alloc_type_ != type) {
      if ((alloc_type_ != MallocBlock::kMallocType) &&
          (alloc_type_ != MallocBlock::kNewType)    &&
          (alloc_type_ != MallocBlock::kArrayNewType)) {
        RAW_LOG(FATAL, "memory stomping bug: a word before object at %p "
                       "has been corrupted", data_addr());
      }
      RAW_LOG(FATAL, "memory allocation/deallocation mismatch at %p: "
                     "allocated with %s being deallocated with %s",
                     data_addr(), AllocName(alloc_type_), DeallocName(type));
    }
    if (alloc_type_ != map_type) {
      RAW_LOG(FATAL, "memory stomping bug: our memory map has been corrupted : "
                     "allocation at %p made with %s "
                     "is recorded in the map to be made with %s",
                     data_addr(), AllocName(alloc_type_),  AllocName(map_type));
    }
  }

 public:  // public accessors

  void* data_addr() { return (void*)&size2_; }
  const void* data_addr() const { return (const void*)&size2_; }

  static size_t data_offset() { return OFFSETOF_MEMBER(MallocBlock, size2_); }

  size_t data_size() const { return size1_; }

  void set_offset(int offset) { this->offset_ = offset; }

 public:  // our main interface

  static MallocBlock* Allocate(size_t size, int type) {
    // Prevent an integer overflow / crash with large allocation sizes.
    // TODO - Note that for a e.g. 64-bit size_t, max_size_t may not actually
    // be the maximum value, depending on how the compiler treats ~0. The worst
    // practical effect is that allocations are limited to 4Gb or so, even if
    // the address space could take more.
    static size_t max_size_t = ~0;
    if (size < 0 || size > max_size_t - sizeof(MallocBlock)) {
      RAW_LOG(ERROR, "Massive size passed to malloc: %"PRIuS"", size);
      return NULL;
    }
    MallocBlock* b = NULL;
    const bool use_malloc_page_fence = FLAGS_malloc_page_fence;
#ifdef HAVE_MMAP
    if (use_malloc_page_fence) {
      // Put the block towards the end of the page and make the next page
      // inaccessible. This will catch buffer overrun right when it happens.
      size_t sz = real_mmapped_size(size);
      int pagesize = getpagesize();
      int num_pages = (sz + pagesize - 1) / pagesize + 1;
      char* p = (char*) mmap(NULL, num_pages * pagesize, PROT_READ|PROT_WRITE,
                             MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
      if (p == MAP_FAILED) {
        // If the allocation fails, abort rather than returning NULL to
        // malloc. This is because in most cases, the program will run out
        // of memory in this mode due to tremendous amount of wastage. There
        // is no point in propagating the error elsewhere.
        RAW_LOG(FATAL, "Out of memory: possibly due to page fence overhead: %s",
                strerror(errno));
      }
      // Mark the page after the block inaccessible
      if (mprotect(p + (num_pages - 1) * pagesize, pagesize, PROT_NONE)) {
        RAW_LOG(FATAL, "Guard page setup failed: %s", strerror(errno));
      }
      b = (MallocBlock*) (p + (num_pages - 1) * pagesize - sz);
    } else {
      b = (MallocBlock*) (type == kMallocType ?
                          BASE_MALLOC(real_malloced_size(size)) :
                          BASE_MALLOC_NEW(real_malloced_size(size)));
    }
#else
    b = (MallocBlock*) (type == kMallocType ?
                        BASE_MALLOC(real_malloced_size(size)) :
                        BASE_MALLOC_NEW(real_malloced_size(size)));
#endif

    // It would be nice to output a diagnostic on allocation failure
    // here, but logging (other than FATAL) requires allocating
    // memory, which could trigger a nasty recursion. Instead, preserve
    // malloc semantics and return NULL on failure.
    if (b != NULL) {
      b->magic1_ = use_malloc_page_fence ? kMagicMMap : kMagicMalloc;
      b->Initialize(size, type);
    }
    return b;
  }

  void Deallocate(int type) {
    if (IsMMapped()) {  // have to do this before CheckAndClear
#ifdef HAVE_MMAP
      int size = CheckAndClear(type);
      int pagesize = getpagesize();
      int num_pages = (size + pagesize - 1) / pagesize + 1;
      char* p = (char*) this;
      if (FLAGS_malloc_page_fence_never_reclaim  ||
          !FLAGS_malloc_reclaim_memory) {
        mprotect(p - (num_pages - 1) * pagesize + size,
                 num_pages * pagesize, PROT_NONE);
      } else {
        munmap(p - (num_pages - 1) * pagesize + size, num_pages * pagesize);
      }
#endif
    } else {
      const size_t size = CheckAndClear(type);
      if (FLAGS_malloc_reclaim_memory) {
        // Instead of freeing the block immediately, push it onto a queue of
        // recently freed blocks.  Free only enough blocks to keep from
        // exceeding the capacity of the queue or causing the total amount of
        // un-released memory in the queue from exceeding
        // FLAGS_max_free_queue_size.
        ProcessFreeQueue(this, size, FLAGS_max_free_queue_size);
      }
    }
  }

  static size_t FreeQueueSize() {
    SpinLockHolder l(&free_queue_lock_);
    return free_queue_size_;
  }

  static void ProcessFreeQueue(MallocBlock* b, size_t size,
                               int max_free_queue_size) {
    SpinLockHolder l(&free_queue_lock_);
    if (free_queue_ == NULL)
      free_queue_ = new FreeQueue<MallocBlockQueueEntry>;
    RAW_CHECK(!free_queue_->Full(), "Free queue mustn't be full!");

    if (b != NULL) {
      free_queue_size_ += size + sizeof(MallocBlockQueueEntry);
      MallocBlockQueueEntry new_entry(b, size);
      free_queue_->Push(new_entry);
    }

    // Free blocks until the total size of unfreed blocks no longer exceeds
    // max_free_queue_size, and the free queue has at least one free
    // space in it.
    while (free_queue_size_ > max_free_queue_size || free_queue_->Full()) {
      MallocBlockQueueEntry cur = free_queue_->Pop();
      CheckForDanglingWrites(cur);
      free_queue_size_ -= cur.size + sizeof(MallocBlockQueueEntry);
      BASE_FREE(cur.block);
    }
    RAW_CHECK(free_queue_size_ >= 0, "Free queue size went negative!");
  }

  static void CheckForDanglingWrites(const MallocBlockQueueEntry& queue_entry) {
    // Initialize the buffer if necessary.
    if (!deleted_buffer_initialized_) {
      // This is threadsafe.  We hold free_queue_lock_.
      memset(kMagicDeletedBuffer, 0xcd, sizeof(kMagicDeletedBuffer));
      deleted_buffer_initialized_ = true;
    }

    const unsigned char* p =
        reinterpret_cast<unsigned char*>(queue_entry.block);

    static const size_t size_of_buffer = sizeof(kMagicDeletedBuffer);
    const size_t size = queue_entry.size;
    const size_t buffers = size / size_of_buffer;
    const size_t remainder = size % size_of_buffer;
    size_t buffer_idx;
    for (buffer_idx = 0; buffer_idx < buffers; ++buffer_idx) {
      CheckForCorruptedBuffer(queue_entry, buffer_idx, p, size_of_buffer);
      p += size_of_buffer;
    }
    CheckForCorruptedBuffer(queue_entry, buffer_idx, p, remainder);
  }

  static void CheckForCorruptedBuffer(const MallocBlockQueueEntry& queue_entry,
                                      size_t buffer_idx,
                                      const unsigned char* buffer,
                                      size_t size_of_buffer) {
    if (memcmp(buffer, kMagicDeletedBuffer, size_of_buffer) == 0) {
      return;
    }

    RAW_LOG(ERROR,
            "Found a corrupted memory buffer in MallocBlock (may be offset "
            "from user ptr): buffer index: %zd, buffer ptr: %p, size of "
            "buffer: %zd", buffer_idx, buffer, size_of_buffer);

    // The magic deleted buffer should only be 1024 bytes, but in case
    // this changes, let's put an upper limit on the number of debug
    // lines we'll output:
    if (size_of_buffer <= 1024) {
      for (int i = 0; i < size_of_buffer; ++i) {
        if (buffer[i] != 0xcd) {
          RAW_LOG(ERROR, "Buffer byte %d is 0x%02x (should be 0xcd).",
                  i, buffer[i]);
        }
      }
    } else {
      RAW_LOG(ERROR, "Buffer too large to print corruption.");
    }

    const MallocBlock* b = queue_entry.block;
    const size_t size = queue_entry.size;
    if (queue_entry.num_deleter_pcs > 0) {
      TracePrintf(STDERR_FILENO, "Deleted by thread %p\n",
                  reinterpret_cast<void*>(
                      PRINTABLE_PTHREAD(queue_entry.deleter_threadid)));

      SymbolTable symbolization_table;
      const int num_symbols = queue_entry.num_deleter_pcs;  // short alias name
      for (int i = 0; i < num_symbols; i++) {
        // Symbolizes the previous address of pc because pc may be in the
        // next function.  This may happen when the function ends with
        // a call to a function annotated noreturn (e.g. CHECK).
        char* pc =
            reinterpret_cast<char*>(queue_entry.deleter_pcs[i]) - 1;
        symbolization_table.Add(pc);
      }
      if (FLAGS_symbolize_stacktrace)
        symbolization_table.Symbolize();
      for (int i = 0; i < num_symbols; i++) {
        char *pc =
            reinterpret_cast<char*>(queue_entry.deleter_pcs[i]) - 1;
        TracePrintf(STDERR_FILENO, "    @ %"PRIxPTR" %s\n",
                    reinterpret_cast<uintptr_t>(pc),
                    symbolization_table.GetSymbol(pc));
      }
    } else {
      RAW_LOG(ERROR,
              "Skipping the printing of the deleter's stack!  Its stack was "
              "not found; either the corruption occurred too early in "
              "execution to obtain a stack trace or --max_free_queue_size was "
              "set to 0.");
    }

    RAW_LOG(FATAL,
            "Memory was written to after being freed.  MallocBlock: %p, user "
            "ptr: %p, size: %zd.  If you can't find the source of the error, "
            "try using valgrind or purify, or study the output of the "
            "deleter's stack printed above.", b, b->data_addr(), size);
  }

  static MallocBlock* FromRawPointer(void* p) {
    const size_t data_offset = MallocBlock::data_offset();
    // Find the header just before client's memory.
    MallocBlock *mb = reinterpret_cast<MallocBlock *>(
                reinterpret_cast<char *>(p) - data_offset);
    // If mb->alloc_type_ is kMagicDeletedInt, we're not an ok pointer.
    if (mb->alloc_type_ == kMagicDeletedInt) {
      RAW_LOG(FATAL, "memory allocation bug: object at %p has been already"
                     " deallocated; or else a word before the object has been"
                     " corrupted (memory stomping bug)", p);
    }
    // If mb->offset_ is zero (common case), mb is the real header.  If
    // mb->offset_ is non-zero, this block was allocated by memalign, and
    // mb->offset_ is the distance backwards to the real header from mb,
    // which is a fake header.  The following subtraction works for both zero
    // and non-zero values.
    return reinterpret_cast<MallocBlock *>(
                reinterpret_cast<char *>(mb) - mb->offset_);
  }
  static const MallocBlock* FromRawPointer(const void* p) {
    // const-safe version: we just cast about
    return FromRawPointer(const_cast<void*>(p));
  }

  void Check(int type) {
    alloc_map_lock_.Lock();
    CheckLocked(type);
    alloc_map_lock_.Unlock();
  }

  static bool CheckEverything() {
    alloc_map_lock_.Lock();
    if (alloc_map_ != NULL)  alloc_map_->Iterate(CheckCallback, 0);
    alloc_map_lock_.Unlock();
    return true;  // if we get here, we're okay
  }

  static bool MemoryStats(int* blocks, size_t* total,
                          int histogram[kMallocHistogramSize]) {
    memset(histogram, 0, kMallocHistogramSize * sizeof(int));
    alloc_map_lock_.Lock();
    stats_blocks_ = 0;
    stats_total_ = 0;
    stats_histogram_ = histogram;
    if (alloc_map_ != NULL) alloc_map_->Iterate(StatsCallback, 0);
    *blocks = stats_blocks_;
    *total = stats_total_;
    alloc_map_lock_.Unlock();
    return true;
  }

 private:  // helpers for CheckEverything and MemoryStats

  static void CheckCallback(const void* ptr, int* type, int dummy) {
    if ((*type & kDeallocatedTypeBit) == 0) {
      FromRawPointer(ptr)->CheckLocked(*type);
    }
  }

  // Accumulation variables for StatsCallback protected by alloc_map_lock_
  static int stats_blocks_;
  static size_t stats_total_;
  static int* stats_histogram_;

  static void StatsCallback(const void* ptr, int* type, int dummy) {
    if ((*type & kDeallocatedTypeBit) == 0) {
      const MallocBlock* b = FromRawPointer(ptr);
      b->CheckLocked(*type);
      ++stats_blocks_;
      size_t mysize = b->size1_;
      int entry = 0;
      stats_total_ += mysize;
      while (mysize) {
        ++entry;
        mysize >>= 1;
      }
      RAW_CHECK(entry < kMallocHistogramSize,
                "kMallocHistogramSize should be at least as large as log2 "
                "of the maximum process memory size");
      stats_histogram_[entry] += 1;
    }
  }
};

void DanglingWriteChecker() {
  // Clear out the remaining free queue to check for dangling writes.
  MallocBlock::ProcessFreeQueue(NULL, 0, 0);
}

// ========================================================================= //

const int MallocBlock::kMagicMalloc;
const int MallocBlock::kMagicMMap;

MallocBlock::AllocMap* MallocBlock::alloc_map_ = NULL;
SpinLock MallocBlock::alloc_map_lock_(SpinLock::LINKER_INITIALIZED);

FreeQueue<MallocBlockQueueEntry>* MallocBlock::free_queue_ = NULL;
size_t MallocBlock::free_queue_size_ = 0;
SpinLock MallocBlock::free_queue_lock_(SpinLock::LINKER_INITIALIZED);

unsigned char MallocBlock::kMagicDeletedBuffer[1024];
bool MallocBlock::deleted_buffer_initialized_ = false;

const char* const MallocBlock::kAllocName[] = {
  "malloc",
  "new",
  "new []",
  NULL,
};

const char* const MallocBlock::kDeallocName[] = {
  "free",
  "delete",
  "delete []",
  NULL,
};

int MallocBlock::stats_blocks_;
size_t MallocBlock::stats_total_;
int* MallocBlock::stats_histogram_;

// ========================================================================= //

// The following cut-down version of printf() avoids
// using stdio or ostreams.
// This is to guarantee no recursive calls into
// the allocator and to bound the stack space consumed.  (The pthread
// manager thread in linuxthreads has a very small stack,
// so fprintf can't be called.)
static void TracePrintf(int fd, const char *fmt, ...) {
  char buf[64];
  int i = 0;
  va_list ap;
  va_start(ap, fmt);
  const char *p = fmt;
  char numbuf[25];
  numbuf[sizeof(numbuf)-1] = 0;
  while (*p != '\0') {              // until end of format string
    char *s = &numbuf[sizeof(numbuf)-1];
    if (p[0] == '%' && p[1] != 0) {  // handle % formats
      int64 l = 0;
      unsigned long base = 0;
      if (*++p == 's') {                            // %s
        s = va_arg(ap, char *);
      } else if (*p == 'l' && p[1] == 'd') {        // %ld
        l = va_arg(ap, long);
        base = 10;
        p++;
      } else if (*p == 'l' && p[1] == 'u') {        // %lu
        l = va_arg(ap, unsigned long);
        base = 10;
        p++;
      } else if (*p == 'z' && p[1] == 'u') {        // %zu
        l = va_arg(ap, size_t);
        base = 10;
        p++;
      } else if (*p == 'u') {                       // %u
        l = va_arg(ap, unsigned int);
        base = 10;
      } else if (*p == 'd') {                       // %d
        l = va_arg(ap, int);
        base = 10;
      } else if (*p == 'p') {                       // %p
        l = va_arg(ap, intptr_t);
        base = 16;
      } else {
        write(STDERR_FILENO, "Unimplemented TracePrintf format\n", 33);
        write(STDERR_FILENO, p, 2);
        write(STDERR_FILENO, "\n", 1);
        abort();
      }
      p++;
      if (base != 0) {
        bool minus = (l < 0 && base == 10);
        uint64 ul = minus? -l : l;
        do {
          *--s = "0123456789abcdef"[ul % base];
          ul /= base;
        } while (ul != 0);
        if (base == 16) {
          *--s = 'x';
          *--s = '0';
        } else if (minus) {
          *--s = '-';
        }
      }
    } else {                        // handle normal characters
      *--s = *p++;
    }
    while (*s != 0) {
      if (i == sizeof(buf)) {
        write(fd, buf, i);
        i = 0;
      }
      buf[i++] = *s++;
    }
  }
  if (i != 0) {
    write(fd, buf, i);
  }
  va_end(ap);
}

// Return the file descriptor we're writing a log to
static int TraceFd() {
  static int trace_fd = -1;
  if (trace_fd == -1) {            // Open the trace file on the first call
    trace_fd = open("/tmp/google.alloc", O_CREAT|O_TRUNC|O_WRONLY, 0666);
    if (trace_fd == -1) {
      trace_fd = 2;
      TracePrintf(trace_fd,
                  "Can't open /tmp/google.alloc.  Logging to stderr.\n");
    }
    // Add a header to the log.
    TracePrintf(trace_fd, "Trace started: %lu\n",
                static_cast<unsigned long>(time(NULL)));
    TracePrintf(trace_fd,
                "func\tsize\tptr\tthread_id\tstack pcs for tools/symbolize\n");
  }
  return trace_fd;
}

// Print the hex stack dump on a single line.   PCs are separated by tabs.
static void TraceStack(void) {
  void *pcs[16];
  int n = GetStackTrace(pcs, sizeof(pcs)/sizeof(pcs[0]), 0);
  for (int i = 0; i != n; i++) {
    TracePrintf(TraceFd(), "\t%p", pcs[i]);
  }
}

// This protects MALLOC_TRACE, to make sure its info is atomically written.
static SpinLock malloc_trace_lock(SpinLock::LINKER_INITIALIZED);

#define MALLOC_TRACE(name, size, addr)                                  \
  do {                                                                  \
    if (FLAGS_malloctrace) {                                            \
      SpinLockHolder l(&malloc_trace_lock);                             \
      TracePrintf(TraceFd(), "%s\t%"PRIuS"\t%p\t%"GPRIuPTHREAD,         \
                  name, size, addr, PRINTABLE_PTHREAD(pthread_self())); \
      TraceStack();                                                     \
      TracePrintf(TraceFd(), "\n");                                     \
    }                                                                   \
  } while (0)

// ========================================================================= //

// Write the characters buf[0, ..., size-1] to
// the malloc trace buffer.
// This function is intended for debugging,
// and is not declared in any header file.
// You must insert a declaration of it by hand when you need
// to use it.
void __malloctrace_write(const char *buf, size_t size) {
  if (FLAGS_malloctrace) {
    write(TraceFd(), buf, size);
  }
}

// ========================================================================= //

// General debug allocation/deallocation

static inline void* DebugAllocate(size_t size, int type) {
  MallocBlock* ptr = MallocBlock::Allocate(size, type);
  if (ptr == NULL)  return NULL;
  MALLOC_TRACE("malloc", size, ptr->data_addr());
  return ptr->data_addr();
}

static inline void DebugDeallocate(void* ptr, int type) {
  MALLOC_TRACE("free",
               (ptr != 0 ? MallocBlock::FromRawPointer(ptr)->data_size() : 0),
               ptr);
  if (ptr)  MallocBlock::FromRawPointer(ptr)->Deallocate(type);
}

// ========================================================================= //

// Alloc/free stuff for debug hooks for malloc & friends

// CAVEAT: The code structure below ensures that MallocHook methods are always
//         called from the stack frame of the invoked allocation function.
//         heap-checker.cc depends on this to start a stack trace from
//         the call to the (de)allocation function.

// Put all callers of MallocHook::Invoke* in this module into
// ATTRIBUTE_SECTION(google_malloc) section,
// so that MallocHook::GetCallerStackTrace can function accurately:

extern "C" {
  void* malloc(size_t size) __THROW ATTRIBUTE_SECTION(google_malloc);
  void free(void* ptr) __THROW ATTRIBUTE_SECTION(google_malloc);
  void* realloc(void* ptr, size_t size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
  void* calloc(size_t nmemb, size_t size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
  void cfree(void* ptr) __THROW ATTRIBUTE_SECTION(google_malloc);

  void* memalign(size_t __alignment, size_t __size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
  int posix_memalign(void** ptr, size_t align, size_t size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
  void* valloc(size_t __size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
  void* pvalloc(size_t __size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
}

static void *MemalignOverride(size_t align, size_t size,
                              const void *caller) __THROW
  ATTRIBUTE_SECTION(google_malloc);

void* operator new(size_t size) throw (std::bad_alloc)
  ATTRIBUTE_SECTION(google_malloc);
void* operator new(size_t size, const std::nothrow_t&) __THROW
  ATTRIBUTE_SECTION(google_malloc);
void operator delete(void* p) __THROW
  ATTRIBUTE_SECTION(google_malloc);
void operator delete(void* p, const std::nothrow_t&) __THROW
  ATTRIBUTE_SECTION(google_malloc);
void* operator new[](size_t size) throw (std::bad_alloc)
  ATTRIBUTE_SECTION(google_malloc);
void* operator new[](size_t size, const std::nothrow_t&) __THROW
  ATTRIBUTE_SECTION(google_malloc);
void operator delete[](void* p) __THROW
  ATTRIBUTE_SECTION(google_malloc);
void operator delete[](void* p, const std::nothrow_t&) __THROW
  ATTRIBUTE_SECTION(google_malloc);

extern "C" void* malloc(size_t size) __THROW {
  void* ptr = DebugAllocate(size, MallocBlock::kMallocType);
  MallocHook::InvokeNewHook(ptr, size);
  return ptr;
}

extern "C" void free(void* ptr) __THROW {
  MallocHook::InvokeDeleteHook(ptr);
  DebugDeallocate(ptr, MallocBlock::kMallocType);
}

extern "C" void* realloc(void* ptr, size_t size) __THROW {
  if (ptr == NULL) {
    ptr = DebugAllocate(size, MallocBlock::kMallocType);
    MallocHook::InvokeNewHook(ptr, size);
    return ptr;
  }
  if (size == 0) {
    MallocHook::InvokeDeleteHook(ptr);
    DebugDeallocate(ptr, MallocBlock::kMallocType);
    return NULL;
  }
  MallocBlock* old = MallocBlock::FromRawPointer(ptr);
  old->Check(MallocBlock::kMallocType);
  MallocBlock* p = MallocBlock::Allocate(size, MallocBlock::kMallocType);

  // If realloc fails we are to leave the old block untouched and
  // return null
  if (p == NULL)  return NULL;

  memcpy(p->data_addr(), old->data_addr(),
         (old->data_size() < size) ? old->data_size() : size);
  MallocHook::InvokeDeleteHook(ptr);
  MallocHook::InvokeNewHook(p->data_addr(), size);
  DebugDeallocate(ptr, MallocBlock::kMallocType);
  MALLOC_TRACE("realloc", p->data_size(), p->data_addr());
  return p->data_addr();
}

extern "C" void* calloc(size_t count, size_t size) __THROW {
  // Overflow check
  const size_t total_size = count * size;
  if (size != 0 && total_size / size != count) return NULL;

  void* block = DebugAllocate(total_size, MallocBlock::kMallocType);
  MallocHook::InvokeNewHook(block, total_size);
  if (block)  memset(block, 0, total_size);
  return block;
}

extern "C" void cfree(void* ptr) __THROW {
  MallocHook::InvokeDeleteHook(ptr);
  DebugDeallocate(ptr, MallocBlock::kMallocType);
}

// Round "value" up to next "alignment" boundary.
// Requires that "alignment" be a power of two.
static intptr_t RoundUp(intptr_t value, intptr_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

static void *do_debug_memalign(size_t alignment, size_t size) {
  // Allocate >= size bytes aligned on "alignment" boundary
  // "alignment" is a power of two.
  void *p = 0;
  RAW_CHECK((alignment & (alignment-1)) == 0, "must be power of two");
  const size_t data_offset = MallocBlock::data_offset();
  // Allocate "alignment-1" extra bytes to ensure alignment is possible, and
  // a further data_offset bytes for an additional fake header.
  size_t extra_bytes = data_offset + alignment - 1;
  if (size + extra_bytes < size) return NULL;         // Overflow
  p = DebugAllocate(size + extra_bytes, MallocBlock::kMallocType);
  if (p != 0) {
    intptr_t orig_p = reinterpret_cast<intptr_t>(p);
    // Leave data_offset bytes for fake header, and round up to meet
    // alignment.
    p = reinterpret_cast<void *>(RoundUp(orig_p + data_offset, alignment));
    // Create a fake header block with an offset_ that points back to the
    // real header.  FromRawPointer uses this value.
    MallocBlock *fake_hdr = reinterpret_cast<MallocBlock *>(
                reinterpret_cast<char *>(p) - data_offset);
    // offset_ is distance between real and fake headers.
    // p is now end of fake header (beginning of client area),
    // and orig_p is the end of the real header, so offset_
    // is their difference.
    fake_hdr->set_offset(reinterpret_cast<intptr_t>(p) - orig_p);
  }
  return p;
}

// Override __libc_memalign in libc on linux boxes.
// They have a bug in libc that causes them (very rarely) to allocate
// with __libc_memalign() yet deallocate with free().
// This function is an exception to the rule of calling MallocHook method
// from the stack frame of the allocation function;
// heap-checker handles this special case explicitly.
static void *MemalignOverride(size_t align, size_t size,
                              const void *caller) __THROW {
  void *p = do_debug_memalign(align, size);
  MallocHook::InvokeNewHook(p, size);
  return p;
}
void *(*__memalign_hook)(size_t, size_t, const void *) = MemalignOverride;

extern "C" void* memalign(size_t align, size_t size) __THROW {
  void *p = do_debug_memalign(align, size);
  MallocHook::InvokeNewHook(p, size);
  return p;
}

// Implementation taken from tcmalloc/tcmalloc.cc
extern "C" int posix_memalign(void** result_ptr,
                              size_t align, size_t size) __THROW {
  if (((align % sizeof(void*)) != 0) ||
      ((align & (align - 1)) != 0) ||
      (align == 0)) {
    return EINVAL;
  }

  void* result = do_debug_memalign(align, size);
  MallocHook::InvokeNewHook(result, size);
  if (result == NULL) {
    return ENOMEM;
  } else {
    *result_ptr = result;
    return 0;
  }
}

extern "C" void* valloc(size_t size) __THROW {
  // Allocate >= size bytes starting on a page boundary
  void *p = do_debug_memalign(getpagesize(), size);
  MallocHook::InvokeNewHook(p, size);
  return p;
}

extern "C" void* pvalloc(size_t size) __THROW {
  // Round size up to a multiple of pages
  // then allocate memory on a page boundary
  int pagesize = getpagesize();
  size = RoundUp(size, pagesize);
  if (size == 0) {     // pvalloc(0) should allocate one page, according to
    size = pagesize;   // http://man.free4web.biz/man3/libmpatrol.3.html
  }
  void *p = do_debug_memalign(pagesize, size);
  MallocHook::InvokeNewHook(p, size);
  return p;
}

extern "C" int mallopt(int cmd, int value) __THROW {
  return BASE_MALLOPT(cmd, value);
}

#ifdef HAVE_STRUCT_MALLINFO
extern "C" struct mallinfo mallinfo(void) __THROW {
  return BASE_MALLINFO();
}
#endif

// ========================================================================= //

// Alloc/free stuff for debug operator new & friends

// This is mostly the same a cpp_alloc in tcmalloc.cc.
inline void* cpp_debug_alloc(size_t size, int new_type, bool nothrow) {
  for (;;) {
    void* p = DebugAllocate(size, new_type);
#ifdef PREANSINEW
    return p;
#else
    if (p == NULL) {  // allocation failed
      // Get the current new handler.  NB: this function is not
      // thread-safe.  We make a feeble stab at making it so here, but
      // this lock only protects against tcmalloc interfering with
      // itself, not with other libraries calling set_new_handler.
      std::new_handler nh;
      {
        SpinLockHolder h(&set_new_handler_lock);
        nh = std::set_new_handler(0);
        (void) std::set_new_handler(nh);
      }
#if (defined(__GNUC__) && !defined(__EXCEPTIONS)) || (defined(_HAS_EXCEPTIONS) && !_HAS_EXCEPTIONS)
      if (nh) {
        // Since exceptions are disabled, we don't really know if new_handler
        // failed.  Assume it will abort if it fails.
        (*nh)();
        continue;
      }
      return 0;
#else
      // If no new_handler is established, the allocation failed.
      if (!nh) {
        if (nothrow) return 0;
        throw std::bad_alloc();
      }
      // Otherwise, try the new_handler.  If it returns, retry the
      // allocation.  If it throws std::bad_alloc, fail the allocation.
      // if it throws something else, don't interfere.
      try {
        (*nh)();
      } catch (const std::bad_alloc&) {
        if (!nothrow) throw;
        return p;
      }
#endif  // (defined(__GNUC__) && !defined(__EXCEPTIONS)) || (defined(_HAS_EXCEPTIONS) && !_HAS_EXCEPTIONS)
    } else {  // allocation success
      return p;
    }
#endif  // PREANSINEW
  }
}

void* operator new(size_t size) throw (std::bad_alloc) {
  void* ptr = cpp_debug_alloc(size, MallocBlock::kNewType, false);
  MallocHook::InvokeNewHook(ptr, size);
  if (ptr == NULL) {
    RAW_LOG(FATAL, "Unable to allocate %"PRIuS" bytes: new failed.", size);
  }
  return ptr;
}

void* operator new(size_t size, const std::nothrow_t&) __THROW {
  void* ptr = cpp_debug_alloc(size, MallocBlock::kNewType, true);
  MallocHook::InvokeNewHook(ptr, size);
  return ptr;
}

void operator delete(void* ptr) __THROW {
  MallocHook::InvokeDeleteHook(ptr);
  DebugDeallocate(ptr, MallocBlock::kNewType);
}

// Some STL implementations explicitly invoke this.
// It is completely equivalent to a normal delete (delete never throws).
void operator delete(void* ptr, const std::nothrow_t&) __THROW {
  MallocHook::InvokeDeleteHook(ptr);
  DebugDeallocate(ptr, MallocBlock::kNewType);
}

// ========================================================================= //

// Alloc/free stuff for debug operator new[] & friends

void* operator new[](size_t size) throw (std::bad_alloc) {
  void* ptr = cpp_debug_alloc(size, MallocBlock::kArrayNewType, false);
  MallocHook::InvokeNewHook(ptr, size);
  if (ptr == NULL) {
    RAW_LOG(FATAL, "Unable to allocate %"PRIuS" bytes: new[] failed.", size);
  }
  return ptr;
}

void* operator new[](size_t size, const std::nothrow_t&) __THROW {
  void* ptr = cpp_debug_alloc(size, MallocBlock::kArrayNewType, true);
  MallocHook::InvokeNewHook(ptr, size);
  return ptr;
}

void operator delete[](void* ptr) __THROW {
  MallocHook::InvokeDeleteHook(ptr);
  DebugDeallocate(ptr, MallocBlock::kArrayNewType);
}

// Some STL implementations explicitly invoke this.
// It is completely equivalent to a normal delete (delete never throws).
void operator delete[](void* ptr, const std::nothrow_t&) __THROW {
  MallocHook::InvokeDeleteHook(ptr);
  DebugDeallocate(ptr, MallocBlock::kArrayNewType);
}

// ========================================================================= //

// The following functions may be called via MallocExtension::instance()
// for memory verification and statistics.
#ifdef TCMALLOC_FOR_DEBUGALLOCATION
// Inherit from tcmalloc's version
typedef TCMallocImplementation ParentImplementation;
#else
// Inherit from default version
typedef MallocExtension ParentImplementation;
#endif

class DebugMallocImplementation : public ParentImplementation {
 public:
  virtual bool GetNumericProperty(const char* name, size_t* value) {
    bool result = ParentImplementation::GetNumericProperty(name, value);
    if (result && (strcmp(name, "generic.current_allocated_bytes") == 0)) {
      // Subtract bytes kept in the free queue
      size_t qsize = MallocBlock::FreeQueueSize();
      if (*value >= qsize) {
        *value -= qsize;
      }
    }
    return result;
  }

  virtual bool VerifyNewMemory(void* p) {
    if (p)  MallocBlock::FromRawPointer(p)->Check(MallocBlock::kNewType);
    return true;
  }

  virtual bool VerifyArrayNewMemory(void* p) {
    if (p)  MallocBlock::FromRawPointer(p)->Check(MallocBlock::kArrayNewType);
    return true;
  }

  virtual bool VerifyMallocMemory(void* p) {
    if (p)  MallocBlock::FromRawPointer(p)->Check(MallocBlock::kMallocType);
    return true;
  }

  virtual bool VerifyAllMemory() {
    return MallocBlock::CheckEverything();
  }

  virtual bool MallocMemoryStats(int* blocks, size_t* total,
                                 int histogram[kMallocHistogramSize]) {
    return MallocBlock::MemoryStats(blocks, total, histogram);
  }

  virtual size_t GetAllocatedSize(void* p) {
    if (p) {
      return MallocBlock::FromRawPointer(p)->data_size();
    }
    return 0;
  }
  virtual size_t GetEstimatedAllocatedSize(size_t size) {
    return size;
  }
 };

static DebugMallocImplementation debug_malloc_implementation;

REGISTER_MODULE_INITIALIZER(debugallocation, {
  // Either we or valgrind will control memory management.  We
  // register our extension if we're the winner.
  if (RunningOnValgrind()) {
    // Let Valgrind uses its own malloc (so don't register our extension).
  } else {
    MallocExtension::Register(&debug_malloc_implementation);
    // When the program exits, check all blocks still in the free
    // queue for corruption.
    atexit(DanglingWriteChecker);
  }
});

#ifdef TCMALLOC_FOR_DEBUGALLOCATION

// Redefine malloc_stats to use tcmalloc's implementation:
extern "C" void malloc_stats(void) __THROW {
  do_malloc_stats();
}

// Some library routines on RedHat 9 allocate memory using malloc()
// and free it using __libc_free() (or vice-versa).  Since we provide
// our own implementations of malloc/free using tcmalloc.cc,
// we need to make sure that the __libc_XXX variants
// also point to the same implementations.
//
// Note: this might not override __libc_XXX calls withing libc itself,
// but it can be important for other libraries that mention these functions
// or when this code is LD_PRELOAD-ed.
// TODO: In case these __libc_* definitions do not actually matter,
// they should go away from here and from tcmalloc/tcmalloc.cc.
//
extern "C" {
  void* __libc_malloc(size_t size)              { return malloc(size);       }
  void  __libc_free(void* ptr)                  { free(ptr);                 }
  void* __libc_realloc(void* ptr, size_t size)  { return realloc(ptr, size); }
  void* __libc_calloc(size_t n, size_t size)    { return calloc(n, size);    }
  void  __libc_cfree(void* ptr)                 { cfree(ptr);                }
  void* __libc_memalign(size_t align, size_t s) { return memalign(align, s); }
  void* __libc_valloc(size_t size)              { return valloc(size);       }
  void* __libc_pvalloc(size_t size)             { return pvalloc(size);      }
  int __posix_memalign(void** r, size_t a, size_t s) {
    return posix_memalign(r, a, s);
  }
}

#endif  // #ifdef TCMALLOC_FOR_DEBUGALLOCATION
