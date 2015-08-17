// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_ZONE_H_
#define V8_ZONE_H_

#include <limits>

#include "src/allocation.h"
#include "src/base/logging.h"
#include "src/globals.h"
#include "src/hashmap.h"
#include "src/list.h"
#include "src/splay-tree.h"

namespace v8 {
namespace internal {


class Segment;
class Isolate;

// The Zone supports very fast allocation of small chunks of
// memory. The chunks cannot be deallocated individually, but instead
// the Zone supports deallocating all chunks in one fast
// operation. The Zone is used to hold temporary data structures like
// the abstract syntax tree, which is deallocated after compilation.

// Note: There is no need to initialize the Zone; the first time an
// allocation is attempted, a segment of memory will be requested
// through a call to malloc().

// Note: The implementation is inherently not thread safe. Do not use
// from multi-threaded code.

class Zone {
 public:
  explicit Zone(Isolate* isolate);
  ~Zone();
  // Allocate 'size' bytes of memory in the Zone; expands the Zone by
  // allocating new segments of memory on demand using malloc().
  void* New(int size);

  template <typename T>
  T* NewArray(int length) {
    CHECK(std::numeric_limits<int>::max() / static_cast<int>(sizeof(T)) >
          length);
    return static_cast<T*>(New(length * sizeof(T)));
  }

  // Deletes all objects and free all memory allocated in the Zone. Keeps one
  // small (size <= kMaximumKeptSegmentSize) segment around if it finds one.
  void DeleteAll();

  // Deletes the last small segment kept around by DeleteAll(). You
  // may no longer allocate in the Zone after a call to this method.
  void DeleteKeptSegment();

  // Returns true if more memory has been allocated in zones than
  // the limit allows.
  inline bool excess_allocation();

  inline void adjust_segment_bytes_allocated(int delta);

  inline unsigned allocation_size() const { return allocation_size_; }

  inline Isolate* isolate() const { return isolate_; }

 private:
  friend class Isolate;

  // All pointers returned from New() have this alignment.  In addition, if the
  // object being allocated has a size that is divisible by 8 then its alignment
  // will be 8. ASan requires 8-byte alignment.
#ifdef V8_USE_ADDRESS_SANITIZER
  static const int kAlignment = 8;
  STATIC_ASSERT(kPointerSize <= 8);
#else
  static const int kAlignment = kPointerSize;
#endif

  // Never allocate segments smaller than this size in bytes.
  static const int kMinimumSegmentSize = 8 * KB;

  // Never allocate segments larger than this size in bytes.
  static const int kMaximumSegmentSize = 1 * MB;

  // Never keep segments larger than this size in bytes around.
  static const int kMaximumKeptSegmentSize = 64 * KB;

  // Report zone excess when allocation exceeds this limit.
  static const int kExcessLimit = 256 * MB;

  // The number of bytes allocated in this zone so far.
  unsigned allocation_size_;

  // The number of bytes allocated in segments.  Note that this number
  // includes memory allocated from the OS but not yet allocated from
  // the zone.
  int segment_bytes_allocated_;

  // Expand the Zone to hold at least 'size' more bytes and allocate
  // the bytes. Returns the address of the newly allocated chunk of
  // memory in the Zone. Should only be called if there isn't enough
  // room in the Zone already.
  Address NewExpand(int size);

  // Creates a new segment, sets it size, and pushes it to the front
  // of the segment chain. Returns the new segment.
  INLINE(Segment* NewSegment(int size));

  // Deletes the given segment. Does not touch the segment chain.
  INLINE(void DeleteSegment(Segment* segment, int size));

  // The free region in the current (front) segment is represented as
  // the half-open interval [position, limit). The 'position' variable
  // is guaranteed to be aligned as dictated by kAlignment.
  Address position_;
  Address limit_;

  Segment* segment_head_;
  Isolate* isolate_;
};


// ZoneObject is an abstraction that helps define classes of objects
// allocated in the Zone. Use it as a base class; see ast.h.
class ZoneObject {
 public:
  // Allocate a new ZoneObject of 'size' bytes in the Zone.
  INLINE(void* operator new(size_t size, Zone* zone));

  // Ideally, the delete operator should be private instead of
  // public, but unfortunately the compiler sometimes synthesizes
  // (unused) destructors for classes derived from ZoneObject, which
  // require the operator to be visible. MSVC requires the delete
  // operator to be public.

  // ZoneObjects should never be deleted individually; use
  // Zone::DeleteAll() to delete all zone objects in one go.
  void operator delete(void*, size_t) { UNREACHABLE(); }
  void operator delete(void* pointer, Zone* zone) { UNREACHABLE(); }
};


// The ZoneScope is used to automatically call DeleteAll() on a
// Zone when the ZoneScope is destroyed (i.e. goes out of scope)
struct ZoneScope {
 public:
  explicit ZoneScope(Zone* zone) : zone_(zone) { }
  ~ZoneScope() { zone_->DeleteAll(); }

  Zone* zone() { return zone_; }

 private:
  Zone* zone_;
};


// The ZoneAllocationPolicy is used to specialize generic data
// structures to allocate themselves and their elements in the Zone.
struct ZoneAllocationPolicy {
 public:
  explicit ZoneAllocationPolicy(Zone* zone) : zone_(zone) { }
  INLINE(void* New(size_t size));
  INLINE(static void Delete(void *pointer)) { }
  Zone* zone() { return zone_; }

 private:
  Zone* zone_;
};


// ZoneLists are growable lists with constant-time access to the
// elements. The list itself and all its elements are allocated in the
// Zone. ZoneLists cannot be deleted individually; you can delete all
// objects in the Zone by calling Zone::DeleteAll().
template<typename T>
class ZoneList: public List<T, ZoneAllocationPolicy> {
 public:
  // Construct a new ZoneList with the given capacity; the length is
  // always zero. The capacity must be non-negative.
  ZoneList(int capacity, Zone* zone)
      : List<T, ZoneAllocationPolicy>(capacity, ZoneAllocationPolicy(zone)) { }

  INLINE(void* operator new(size_t size, Zone* zone));

  // Construct a new ZoneList by copying the elements of the given ZoneList.
  ZoneList(const ZoneList<T>& other, Zone* zone)
      : List<T, ZoneAllocationPolicy>(other.length(),
                                      ZoneAllocationPolicy(zone)) {
    AddAll(other, zone);
  }

  // We add some convenience wrappers so that we can pass in a Zone
  // instead of a (less convenient) ZoneAllocationPolicy.
  INLINE(void Add(const T& element, Zone* zone)) {
    List<T, ZoneAllocationPolicy>::Add(element, ZoneAllocationPolicy(zone));
  }
  INLINE(void AddAll(const List<T, ZoneAllocationPolicy>& other, Zone* zone)) {
    List<T, ZoneAllocationPolicy>::AddAll(other, ZoneAllocationPolicy(zone));
  }
  INLINE(void AddAll(const Vector<T>& other, Zone* zone)) {
    List<T, ZoneAllocationPolicy>::AddAll(other, ZoneAllocationPolicy(zone));
  }
  INLINE(void InsertAt(int index, const T& element, Zone* zone)) {
    List<T, ZoneAllocationPolicy>::InsertAt(index, element,
                                            ZoneAllocationPolicy(zone));
  }
  INLINE(Vector<T> AddBlock(T value, int count, Zone* zone)) {
    return List<T, ZoneAllocationPolicy>::AddBlock(value, count,
                                                   ZoneAllocationPolicy(zone));
  }
  INLINE(void Allocate(int length, Zone* zone)) {
    List<T, ZoneAllocationPolicy>::Allocate(length, ZoneAllocationPolicy(zone));
  }
  INLINE(void Initialize(int capacity, Zone* zone)) {
    List<T, ZoneAllocationPolicy>::Initialize(capacity,
                                              ZoneAllocationPolicy(zone));
  }

  void operator delete(void* pointer) { UNREACHABLE(); }
  void operator delete(void* pointer, Zone* zone) { UNREACHABLE(); }
};


// A zone splay tree.  The config type parameter encapsulates the
// different configurations of a concrete splay tree (see splay-tree.h).
// The tree itself and all its elements are allocated in the Zone.
template <typename Config>
class ZoneSplayTree: public SplayTree<Config, ZoneAllocationPolicy> {
 public:
  explicit ZoneSplayTree(Zone* zone)
      : SplayTree<Config, ZoneAllocationPolicy>(ZoneAllocationPolicy(zone)) {}
  ~ZoneSplayTree();

  INLINE(void* operator new(size_t size, Zone* zone));

  void operator delete(void* pointer) { UNREACHABLE(); }
  void operator delete(void* pointer, Zone* zone) { UNREACHABLE(); }
};


typedef TemplateHashMapImpl<ZoneAllocationPolicy> ZoneHashMap;

} }  // namespace v8::internal

#endif  // V8_ZONE_H_
