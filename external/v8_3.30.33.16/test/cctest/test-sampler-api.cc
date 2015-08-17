// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Tests the sampling API in include/v8.h

#include <map>
#include <string>
#include "include/v8.h"
#include "src/simulator.h"
#include "test/cctest/cctest.h"

namespace {

class Sample {
 public:
  enum { kFramesLimit = 255 };

  Sample() {}

  typedef const void* const* const_iterator;
  const_iterator begin() const { return data_.start(); }
  const_iterator end() const { return &data_[data_.length()]; }

  int size() const { return data_.length(); }
  v8::internal::Vector<void*>& data() { return data_; }

 private:
  v8::internal::EmbeddedVector<void*, kFramesLimit> data_;
};


#if defined(USE_SIMULATOR)
class SimulatorHelper {
 public:
  inline bool Init(v8::Isolate* isolate) {
    simulator_ = reinterpret_cast<v8::internal::Isolate*>(isolate)
                     ->thread_local_top()
                     ->simulator_;
    // Check if there is active simulator.
    return simulator_ != NULL;
  }

  inline void FillRegisters(v8::RegisterState* state) {
#if V8_TARGET_ARCH_ARM
    state->pc = reinterpret_cast<void*>(simulator_->get_pc());
    state->sp = reinterpret_cast<void*>(
        simulator_->get_register(v8::internal::Simulator::sp));
    state->fp = reinterpret_cast<void*>(
        simulator_->get_register(v8::internal::Simulator::r11));
#elif V8_TARGET_ARCH_ARM64
    if (simulator_->sp() == 0 || simulator_->fp() == 0) {
      // It's possible that the simulator is interrupted while it is updating
      // the sp or fp register. ARM64 simulator does this in two steps:
      // first setting it to zero and then setting it to a new value.
      // Bailout if sp/fp doesn't contain the new value.
      return;
    }
    state->pc = reinterpret_cast<void*>(simulator_->pc());
    state->sp = reinterpret_cast<void*>(simulator_->sp());
    state->fp = reinterpret_cast<void*>(simulator_->fp());
#elif V8_TARGET_ARCH_MIPS || V8_TARGET_ARCH_MIPS64
    state->pc = reinterpret_cast<void*>(simulator_->get_pc());
    state->sp = reinterpret_cast<void*>(
        simulator_->get_register(v8::internal::Simulator::sp));
    state->fp = reinterpret_cast<void*>(
        simulator_->get_register(v8::internal::Simulator::fp));
#endif
  }

 private:
  v8::internal::Simulator* simulator_;
};
#endif  // USE_SIMULATOR


class SamplingTestHelper {
 public:
  struct CodeEventEntry {
    std::string name;
    const void* code_start;
    size_t code_len;
  };
  typedef std::map<const void*, CodeEventEntry> CodeEntries;

  explicit SamplingTestHelper(const std::string& test_function)
      : sample_is_taken_(false), isolate_(CcTest::isolate()) {
    DCHECK_EQ(NULL, instance_);
    instance_ = this;
    v8::HandleScope scope(isolate_);
    v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate_);
    global->Set(v8::String::NewFromUtf8(isolate_, "CollectSample"),
                v8::FunctionTemplate::New(isolate_, CollectSample));
    LocalContext env(isolate_, NULL, global);
    isolate_->SetJitCodeEventHandler(v8::kJitCodeEventDefault,
                                     JitCodeEventHandler);
    v8::Script::Compile(
        v8::String::NewFromUtf8(isolate_, test_function.c_str()))->Run();
  }

  ~SamplingTestHelper() {
    isolate_->SetJitCodeEventHandler(v8::kJitCodeEventDefault, NULL);
    instance_ = NULL;
  }

  Sample& sample() { return sample_; }

  const CodeEventEntry* FindEventEntry(const void* address) {
    CodeEntries::const_iterator it = code_entries_.upper_bound(address);
    if (it == code_entries_.begin()) return NULL;
    const CodeEventEntry& entry = (--it)->second;
    const void* code_end =
        static_cast<const uint8_t*>(entry.code_start) + entry.code_len;
    return address < code_end ? &entry : NULL;
  }

 private:
  static void CollectSample(const v8::FunctionCallbackInfo<v8::Value>& args) {
    instance_->DoCollectSample();
  }

  static void JitCodeEventHandler(const v8::JitCodeEvent* event) {
    instance_->DoJitCodeEventHandler(event);
  }

  // The JavaScript calls this function when on full stack depth.
  void DoCollectSample() {
    v8::RegisterState state;
#if defined(USE_SIMULATOR)
    SimulatorHelper simulator_helper;
    if (!simulator_helper.Init(isolate_)) return;
    simulator_helper.FillRegisters(&state);
#else
    state.pc = NULL;
    state.fp = &state;
    state.sp = &state;
#endif
    v8::SampleInfo info;
    isolate_->GetStackSample(state, sample_.data().start(),
                             static_cast<size_t>(sample_.size()), &info);
    size_t frames_count = info.frames_count;
    CHECK_LE(frames_count, static_cast<size_t>(sample_.size()));
    sample_.data().Truncate(static_cast<int>(frames_count));
    sample_is_taken_ = true;
  }

  void DoJitCodeEventHandler(const v8::JitCodeEvent* event) {
    if (sample_is_taken_) return;
    switch (event->type) {
      case v8::JitCodeEvent::CODE_ADDED: {
        CodeEventEntry entry;
        entry.name = std::string(event->name.str, event->name.len);
        entry.code_start = event->code_start;
        entry.code_len = event->code_len;
        code_entries_.insert(std::make_pair(entry.code_start, entry));
        break;
      }
      case v8::JitCodeEvent::CODE_MOVED: {
        CodeEntries::iterator it = code_entries_.find(event->code_start);
        CHECK(it != code_entries_.end());
        code_entries_.erase(it);
        CodeEventEntry entry;
        entry.name = std::string(event->name.str, event->name.len);
        entry.code_start = event->new_code_start;
        entry.code_len = event->code_len;
        code_entries_.insert(std::make_pair(entry.code_start, entry));
        break;
      }
      case v8::JitCodeEvent::CODE_REMOVED:
        code_entries_.erase(event->code_start);
        break;
      default:
        break;
    }
  }

  Sample sample_;
  bool sample_is_taken_;
  v8::Isolate* isolate_;
  CodeEntries code_entries_;

  static SamplingTestHelper* instance_;
};

SamplingTestHelper* SamplingTestHelper::instance_;

}  // namespace


// A JavaScript function which takes stack depth
// (minimum value 2) as an argument.
// When at the bottom of the recursion,
// the JavaScript code calls into C++ test code,
// waiting for the sampler to take a sample.
static const char* test_function =
    "function func(depth) {"
    "  if (depth == 2) CollectSample();"
    "  else return func(depth - 1);"
    "}";


TEST(StackDepthIsConsistent) {
  SamplingTestHelper helper(std::string(test_function) + "func(8);");
  CHECK_EQ(8, helper.sample().size());
}


TEST(StackDepthDoesNotExceedMaxValue) {
  SamplingTestHelper helper(std::string(test_function) + "func(300);");
  CHECK_EQ(Sample::kFramesLimit, helper.sample().size());
}


// The captured sample should have three pc values.
// They should fall in the range where the compiled code resides.
// The expected stack is:
// bottom of stack [{anon script}, outer, inner] top of stack
//                              ^      ^       ^
// sample.stack indices         2      1       0
TEST(StackFramesConsistent) {
  // Note: The arguments.callee stuff is there so that the
  //       functions are not optimized away.
  const char* test_script =
      "function test_sampler_api_inner() {"
      "  CollectSample();"
      "  return arguments.callee.toString();"
      "}"
      "function test_sampler_api_outer() {"
      "  return test_sampler_api_inner() + arguments.callee.toString();"
      "}"
      "test_sampler_api_outer();";

  SamplingTestHelper helper(test_script);
  Sample& sample = helper.sample();
  CHECK_EQ(3, sample.size());

  const SamplingTestHelper::CodeEventEntry* entry;
  entry = helper.FindEventEntry(sample.begin()[0]);
  CHECK_NE(NULL, entry);
  CHECK(std::string::npos != entry->name.find("test_sampler_api_inner"));

  entry = helper.FindEventEntry(sample.begin()[1]);
  CHECK_NE(NULL, entry);
  CHECK(std::string::npos != entry->name.find("test_sampler_api_outer"));
}
