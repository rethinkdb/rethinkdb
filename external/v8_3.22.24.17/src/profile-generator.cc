// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
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

#include "v8.h"

#include "profile-generator-inl.h"

#include "compiler.h"
#include "debug.h"
#include "sampler.h"
#include "global-handles.h"
#include "scopeinfo.h"
#include "unicode.h"
#include "zone-inl.h"

namespace v8 {
namespace internal {


bool StringsStorage::StringsMatch(void* key1, void* key2) {
  return strcmp(reinterpret_cast<char*>(key1),
                reinterpret_cast<char*>(key2)) == 0;
}


StringsStorage::StringsStorage(Heap* heap)
    : hash_seed_(heap->HashSeed()), names_(StringsMatch) {
}


StringsStorage::~StringsStorage() {
  for (HashMap::Entry* p = names_.Start();
       p != NULL;
       p = names_.Next(p)) {
    DeleteArray(reinterpret_cast<const char*>(p->value));
  }
}


const char* StringsStorage::GetCopy(const char* src) {
  int len = static_cast<int>(strlen(src));
  HashMap::Entry* entry = GetEntry(src, len);
  if (entry->value == NULL) {
    Vector<char> dst = Vector<char>::New(len + 1);
    OS::StrNCpy(dst, src, len);
    dst[len] = '\0';
    entry->key = dst.start();
    entry->value = entry->key;
  }
  return reinterpret_cast<const char*>(entry->value);
}


const char* StringsStorage::GetFormatted(const char* format, ...) {
  va_list args;
  va_start(args, format);
  const char* result = GetVFormatted(format, args);
  va_end(args);
  return result;
}


const char* StringsStorage::AddOrDisposeString(char* str, int len) {
  HashMap::Entry* entry = GetEntry(str, len);
  if (entry->value == NULL) {
    // New entry added.
    entry->key = str;
    entry->value = str;
  } else {
    DeleteArray(str);
  }
  return reinterpret_cast<const char*>(entry->value);
}


const char* StringsStorage::GetVFormatted(const char* format, va_list args) {
  Vector<char> str = Vector<char>::New(1024);
  int len = OS::VSNPrintF(str, format, args);
  if (len == -1) {
    DeleteArray(str.start());
    return GetCopy(format);
  }
  return AddOrDisposeString(str.start(), len);
}


const char* StringsStorage::GetName(Name* name) {
  if (name->IsString()) {
    String* str = String::cast(name);
    int length = Min(kMaxNameSize, str->length());
    int actual_length = 0;
    SmartArrayPointer<char> data =
        str->ToCString(DISALLOW_NULLS, ROBUST_STRING_TRAVERSAL, 0, length,
                       &actual_length);
    return AddOrDisposeString(data.Detach(), actual_length);
  } else if (name->IsSymbol()) {
    return "<symbol>";
  }
  return "";
}


const char* StringsStorage::GetName(int index) {
  return GetFormatted("%d", index);
}


const char* StringsStorage::GetFunctionName(Name* name) {
  return BeautifyFunctionName(GetName(name));
}


const char* StringsStorage::GetFunctionName(const char* name) {
  return BeautifyFunctionName(GetCopy(name));
}


const char* StringsStorage::BeautifyFunctionName(const char* name) {
  return (*name == 0) ? ProfileGenerator::kAnonymousFunctionName : name;
}


size_t StringsStorage::GetUsedMemorySize() const {
  size_t size = sizeof(*this);
  size += sizeof(HashMap::Entry) * names_.capacity();
  for (HashMap::Entry* p = names_.Start(); p != NULL; p = names_.Next(p)) {
    size += strlen(reinterpret_cast<const char*>(p->value)) + 1;
  }
  return size;
}


HashMap::Entry* StringsStorage::GetEntry(const char* str, int len) {
  uint32_t hash = StringHasher::HashSequentialString(str, len, hash_seed_);
  return names_.Lookup(const_cast<char*>(str), hash, true);
}


const char* const CodeEntry::kEmptyNamePrefix = "";
const char* const CodeEntry::kEmptyResourceName = "";
const char* const CodeEntry::kEmptyBailoutReason = "";


CodeEntry::~CodeEntry() {
  delete no_frame_ranges_;
}


uint32_t CodeEntry::GetCallUid() const {
  uint32_t hash = ComputeIntegerHash(tag_, v8::internal::kZeroHashSeed);
  if (shared_id_ != 0) {
    hash ^= ComputeIntegerHash(static_cast<uint32_t>(shared_id_),
                               v8::internal::kZeroHashSeed);
  } else {
    hash ^= ComputeIntegerHash(
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(name_prefix_)),
        v8::internal::kZeroHashSeed);
    hash ^= ComputeIntegerHash(
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(name_)),
        v8::internal::kZeroHashSeed);
    hash ^= ComputeIntegerHash(
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(resource_name_)),
        v8::internal::kZeroHashSeed);
    hash ^= ComputeIntegerHash(line_number_, v8::internal::kZeroHashSeed);
  }
  return hash;
}


bool CodeEntry::IsSameAs(CodeEntry* entry) const {
  return this == entry
      || (tag_ == entry->tag_
          && shared_id_ == entry->shared_id_
          && (shared_id_ != 0
              || (name_prefix_ == entry->name_prefix_
                  && name_ == entry->name_
                  && resource_name_ == entry->resource_name_
                  && line_number_ == entry->line_number_)));
}


void CodeEntry::SetBuiltinId(Builtins::Name id) {
  tag_ = Logger::BUILTIN_TAG;
  builtin_id_ = id;
}


ProfileNode* ProfileNode::FindChild(CodeEntry* entry) {
  HashMap::Entry* map_entry =
      children_.Lookup(entry, CodeEntryHash(entry), false);
  return map_entry != NULL ?
      reinterpret_cast<ProfileNode*>(map_entry->value) : NULL;
}


ProfileNode* ProfileNode::FindOrAddChild(CodeEntry* entry) {
  HashMap::Entry* map_entry =
      children_.Lookup(entry, CodeEntryHash(entry), true);
  if (map_entry->value == NULL) {
    // New node added.
    ProfileNode* new_node = new ProfileNode(tree_, entry);
    map_entry->value = new_node;
    children_list_.Add(new_node);
  }
  return reinterpret_cast<ProfileNode*>(map_entry->value);
}


void ProfileNode::Print(int indent) {
  OS::Print("%5u %*c %s%s %d #%d %s",
            self_ticks_,
            indent, ' ',
            entry_->name_prefix(),
            entry_->name(),
            entry_->script_id(),
            id(),
            entry_->bailout_reason());
  if (entry_->resource_name()[0] != '\0')
    OS::Print(" %s:%d", entry_->resource_name(), entry_->line_number());
  OS::Print("\n");
  for (HashMap::Entry* p = children_.Start();
       p != NULL;
       p = children_.Next(p)) {
    reinterpret_cast<ProfileNode*>(p->value)->Print(indent + 2);
  }
}


class DeleteNodesCallback {
 public:
  void BeforeTraversingChild(ProfileNode*, ProfileNode*) { }

  void AfterAllChildrenTraversed(ProfileNode* node) {
    delete node;
  }

  void AfterChildTraversed(ProfileNode*, ProfileNode*) { }
};


ProfileTree::ProfileTree()
    : root_entry_(Logger::FUNCTION_TAG, "(root)"),
      next_node_id_(1),
      root_(new ProfileNode(this, &root_entry_)) {
}


ProfileTree::~ProfileTree() {
  DeleteNodesCallback cb;
  TraverseDepthFirst(&cb);
}


ProfileNode* ProfileTree::AddPathFromEnd(const Vector<CodeEntry*>& path) {
  ProfileNode* node = root_;
  for (CodeEntry** entry = path.start() + path.length() - 1;
       entry != path.start() - 1;
       --entry) {
    if (*entry != NULL) {
      node = node->FindOrAddChild(*entry);
    }
  }
  node->IncrementSelfTicks();
  return node;
}


void ProfileTree::AddPathFromStart(const Vector<CodeEntry*>& path) {
  ProfileNode* node = root_;
  for (CodeEntry** entry = path.start();
       entry != path.start() + path.length();
       ++entry) {
    if (*entry != NULL) {
      node = node->FindOrAddChild(*entry);
    }
  }
  node->IncrementSelfTicks();
}


struct NodesPair {
  NodesPair(ProfileNode* src, ProfileNode* dst)
      : src(src), dst(dst) { }
  ProfileNode* src;
  ProfileNode* dst;
};


class Position {
 public:
  explicit Position(ProfileNode* node)
      : node(node), child_idx_(0) { }
  INLINE(ProfileNode* current_child()) {
    return node->children()->at(child_idx_);
  }
  INLINE(bool has_current_child()) {
    return child_idx_ < node->children()->length();
  }
  INLINE(void next_child()) { ++child_idx_; }

  ProfileNode* node;
 private:
  int child_idx_;
};


// Non-recursive implementation of a depth-first post-order tree traversal.
template <typename Callback>
void ProfileTree::TraverseDepthFirst(Callback* callback) {
  List<Position> stack(10);
  stack.Add(Position(root_));
  while (stack.length() > 0) {
    Position& current = stack.last();
    if (current.has_current_child()) {
      callback->BeforeTraversingChild(current.node, current.current_child());
      stack.Add(Position(current.current_child()));
    } else {
      callback->AfterAllChildrenTraversed(current.node);
      if (stack.length() > 1) {
        Position& parent = stack[stack.length() - 2];
        callback->AfterChildTraversed(parent.node, current.node);
        parent.next_child();
      }
      // Remove child from the stack.
      stack.RemoveLast();
    }
  }
}


CpuProfile::CpuProfile(const char* title, unsigned uid, bool record_samples)
    : title_(title),
      uid_(uid),
      record_samples_(record_samples),
      start_time_(Time::NowFromSystemTime()) {
  timer_.Start();
}


void CpuProfile::AddPath(const Vector<CodeEntry*>& path) {
  ProfileNode* top_frame_node = top_down_.AddPathFromEnd(path);
  if (record_samples_) samples_.Add(top_frame_node);
}


void CpuProfile::CalculateTotalTicksAndSamplingRate() {
  end_time_ = start_time_ + timer_.Elapsed();
}


void CpuProfile::Print() {
  OS::Print("[Top down]:\n");
  top_down_.Print();
}


CodeEntry* const CodeMap::kSharedFunctionCodeEntry = NULL;
const CodeMap::CodeTreeConfig::Key CodeMap::CodeTreeConfig::kNoKey = NULL;


void CodeMap::AddCode(Address addr, CodeEntry* entry, unsigned size) {
  DeleteAllCoveredCode(addr, addr + size);
  CodeTree::Locator locator;
  tree_.Insert(addr, &locator);
  locator.set_value(CodeEntryInfo(entry, size));
}


void CodeMap::DeleteAllCoveredCode(Address start, Address end) {
  List<Address> to_delete;
  Address addr = end - 1;
  while (addr >= start) {
    CodeTree::Locator locator;
    if (!tree_.FindGreatestLessThan(addr, &locator)) break;
    Address start2 = locator.key(), end2 = start2 + locator.value().size;
    if (start2 < end && start < end2) to_delete.Add(start2);
    addr = start2 - 1;
  }
  for (int i = 0; i < to_delete.length(); ++i) tree_.Remove(to_delete[i]);
}


CodeEntry* CodeMap::FindEntry(Address addr, Address* start) {
  CodeTree::Locator locator;
  if (tree_.FindGreatestLessThan(addr, &locator)) {
    // locator.key() <= addr. Need to check that addr is within entry.
    const CodeEntryInfo& entry = locator.value();
    if (addr < (locator.key() + entry.size)) {
      if (start) {
        *start = locator.key();
      }
      return entry.entry;
    }
  }
  return NULL;
}


int CodeMap::GetSharedId(Address addr) {
  CodeTree::Locator locator;
  // For shared function entries, 'size' field is used to store their IDs.
  if (tree_.Find(addr, &locator)) {
    const CodeEntryInfo& entry = locator.value();
    ASSERT(entry.entry == kSharedFunctionCodeEntry);
    return entry.size;
  } else {
    tree_.Insert(addr, &locator);
    int id = next_shared_id_++;
    locator.set_value(CodeEntryInfo(kSharedFunctionCodeEntry, id));
    return id;
  }
}


void CodeMap::MoveCode(Address from, Address to) {
  if (from == to) return;
  CodeTree::Locator locator;
  if (!tree_.Find(from, &locator)) return;
  CodeEntryInfo entry = locator.value();
  tree_.Remove(from);
  AddCode(to, entry.entry, entry.size);
}


void CodeMap::CodeTreePrinter::Call(
    const Address& key, const CodeMap::CodeEntryInfo& value) {
  // For shared function entries, 'size' field is used to store their IDs.
  if (value.entry == kSharedFunctionCodeEntry) {
    OS::Print("%p SharedFunctionInfo %d\n", key, value.size);
  } else {
    OS::Print("%p %5d %s\n", key, value.size, value.entry->name());
  }
}


void CodeMap::Print() {
  CodeTreePrinter printer;
  tree_.ForEach(&printer);
}


CpuProfilesCollection::CpuProfilesCollection(Heap* heap)
    : function_and_resource_names_(heap),
      current_profiles_semaphore_(1) {
}


static void DeleteCodeEntry(CodeEntry** entry_ptr) {
  delete *entry_ptr;
}


static void DeleteCpuProfile(CpuProfile** profile_ptr) {
  delete *profile_ptr;
}


CpuProfilesCollection::~CpuProfilesCollection() {
  finished_profiles_.Iterate(DeleteCpuProfile);
  current_profiles_.Iterate(DeleteCpuProfile);
  code_entries_.Iterate(DeleteCodeEntry);
}


bool CpuProfilesCollection::StartProfiling(const char* title, unsigned uid,
                                           bool record_samples) {
  ASSERT(uid > 0);
  current_profiles_semaphore_.Wait();
  if (current_profiles_.length() >= kMaxSimultaneousProfiles) {
    current_profiles_semaphore_.Signal();
    return false;
  }
  for (int i = 0; i < current_profiles_.length(); ++i) {
    if (strcmp(current_profiles_[i]->title(), title) == 0) {
      // Ignore attempts to start profile with the same title.
      current_profiles_semaphore_.Signal();
      return false;
    }
  }
  current_profiles_.Add(new CpuProfile(title, uid, record_samples));
  current_profiles_semaphore_.Signal();
  return true;
}


CpuProfile* CpuProfilesCollection::StopProfiling(const char* title) {
  const int title_len = StrLength(title);
  CpuProfile* profile = NULL;
  current_profiles_semaphore_.Wait();
  for (int i = current_profiles_.length() - 1; i >= 0; --i) {
    if (title_len == 0 || strcmp(current_profiles_[i]->title(), title) == 0) {
      profile = current_profiles_.Remove(i);
      break;
    }
  }
  current_profiles_semaphore_.Signal();

  if (profile == NULL) return NULL;
  profile->CalculateTotalTicksAndSamplingRate();
  finished_profiles_.Add(profile);
  return profile;
}


bool CpuProfilesCollection::IsLastProfile(const char* title) {
  // Called from VM thread, and only it can mutate the list,
  // so no locking is needed here.
  if (current_profiles_.length() != 1) return false;
  return StrLength(title) == 0
      || strcmp(current_profiles_[0]->title(), title) == 0;
}


void CpuProfilesCollection::RemoveProfile(CpuProfile* profile) {
  // Called from VM thread for a completed profile.
  unsigned uid = profile->uid();
  for (int i = 0; i < finished_profiles_.length(); i++) {
    if (uid == finished_profiles_[i]->uid()) {
      finished_profiles_.Remove(i);
      return;
    }
  }
  UNREACHABLE();
}


void CpuProfilesCollection::AddPathToCurrentProfiles(
    const Vector<CodeEntry*>& path) {
  // As starting / stopping profiles is rare relatively to this
  // method, we don't bother minimizing the duration of lock holding,
  // e.g. copying contents of the list to a local vector.
  current_profiles_semaphore_.Wait();
  for (int i = 0; i < current_profiles_.length(); ++i) {
    current_profiles_[i]->AddPath(path);
  }
  current_profiles_semaphore_.Signal();
}


CodeEntry* CpuProfilesCollection::NewCodeEntry(
      Logger::LogEventsAndTags tag,
      const char* name,
      const char* name_prefix,
      const char* resource_name,
      int line_number,
      int column_number) {
  CodeEntry* code_entry = new CodeEntry(tag,
                                        name,
                                        name_prefix,
                                        resource_name,
                                        line_number,
                                        column_number);
  code_entries_.Add(code_entry);
  return code_entry;
}


const char* const ProfileGenerator::kAnonymousFunctionName =
    "(anonymous function)";
const char* const ProfileGenerator::kProgramEntryName =
    "(program)";
const char* const ProfileGenerator::kIdleEntryName =
    "(idle)";
const char* const ProfileGenerator::kGarbageCollectorEntryName =
    "(garbage collector)";
const char* const ProfileGenerator::kUnresolvedFunctionName =
    "(unresolved function)";


ProfileGenerator::ProfileGenerator(CpuProfilesCollection* profiles)
    : profiles_(profiles),
      program_entry_(
          profiles->NewCodeEntry(Logger::FUNCTION_TAG, kProgramEntryName)),
      idle_entry_(
          profiles->NewCodeEntry(Logger::FUNCTION_TAG, kIdleEntryName)),
      gc_entry_(
          profiles->NewCodeEntry(Logger::BUILTIN_TAG,
                                 kGarbageCollectorEntryName)),
      unresolved_entry_(
          profiles->NewCodeEntry(Logger::FUNCTION_TAG,
                                 kUnresolvedFunctionName)) {
}


void ProfileGenerator::RecordTickSample(const TickSample& sample) {
  // Allocate space for stack frames + pc + function + vm-state.
  ScopedVector<CodeEntry*> entries(sample.frames_count + 3);
  // As actual number of decoded code entries may vary, initialize
  // entries vector with NULL values.
  CodeEntry** entry = entries.start();
  memset(entry, 0, entries.length() * sizeof(*entry));
  if (sample.pc != NULL) {
    if (sample.has_external_callback && sample.state == EXTERNAL &&
        sample.top_frame_type == StackFrame::EXIT) {
      // Don't use PC when in external callback code, as it can point
      // inside callback's code, and we will erroneously report
      // that a callback calls itself.
      *entry++ = code_map_.FindEntry(sample.external_callback);
    } else {
      Address start;
      CodeEntry* pc_entry = code_map_.FindEntry(sample.pc, &start);
      // If pc is in the function code before it set up stack frame or after the
      // frame was destroyed SafeStackFrameIterator incorrectly thinks that
      // ebp contains return address of the current function and skips caller's
      // frame. Check for this case and just skip such samples.
      if (pc_entry) {
        List<OffsetRange>* ranges = pc_entry->no_frame_ranges();
        if (ranges) {
          Code* code = Code::cast(HeapObject::FromAddress(start));
          int pc_offset = static_cast<int>(
              sample.pc - code->instruction_start());
          for (int i = 0; i < ranges->length(); i++) {
            OffsetRange& range = ranges->at(i);
            if (range.from <= pc_offset && pc_offset < range.to) {
              return;
            }
          }
        }
        *entry++ = pc_entry;

        if (pc_entry->builtin_id() == Builtins::kFunctionCall ||
            pc_entry->builtin_id() == Builtins::kFunctionApply) {
          // When current function is FunctionCall or FunctionApply builtin the
          // top frame is either frame of the calling JS function or internal
          // frame. In the latter case we know the caller for sure but in the
          // former case we don't so we simply replace the frame with
          // 'unresolved' entry.
          if (sample.top_frame_type == StackFrame::JAVA_SCRIPT) {
            *entry++ = unresolved_entry_;
          }
        }
      }
    }

    for (const Address* stack_pos = sample.stack,
           *stack_end = stack_pos + sample.frames_count;
         stack_pos != stack_end;
         ++stack_pos) {
      *entry++ = code_map_.FindEntry(*stack_pos);
    }
  }

  if (FLAG_prof_browser_mode) {
    bool no_symbolized_entries = true;
    for (CodeEntry** e = entries.start(); e != entry; ++e) {
      if (*e != NULL) {
        no_symbolized_entries = false;
        break;
      }
    }
    // If no frames were symbolized, put the VM state entry in.
    if (no_symbolized_entries) {
      *entry++ = EntryForVMState(sample.state);
    }
  }

  profiles_->AddPathToCurrentProfiles(entries);
}


CodeEntry* ProfileGenerator::EntryForVMState(StateTag tag) {
  switch (tag) {
    case GC:
      return gc_entry_;
    case JS:
    case COMPILER:
    // DOM events handlers are reported as OTHER / EXTERNAL entries.
    // To avoid confusing people, let's put all these entries into
    // one bucket.
    case OTHER:
    case EXTERNAL:
      return program_entry_;
    case IDLE:
      return idle_entry_;
    default: return NULL;
  }
}

} }  // namespace v8::internal
