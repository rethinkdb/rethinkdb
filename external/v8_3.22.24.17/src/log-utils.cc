// Copyright 2009 the V8 project authors. All rights reserved.
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

#include "log-utils.h"
#include "string-stream.h"

namespace v8 {
namespace internal {


const char* const Log::kLogToTemporaryFile = "&";
const char* const Log::kLogToConsole = "-";


Log::Log(Logger* logger)
  : is_stopped_(false),
    output_handle_(NULL),
    message_buffer_(NULL),
    logger_(logger) {
}


void Log::Initialize(const char* log_file_name) {
  message_buffer_ = NewArray<char>(kMessageBufferSize);

  // --log-all enables all the log flags.
  if (FLAG_log_all) {
    FLAG_log_runtime = true;
    FLAG_log_api = true;
    FLAG_log_code = true;
    FLAG_log_gc = true;
    FLAG_log_suspect = true;
    FLAG_log_handles = true;
    FLAG_log_regexp = true;
    FLAG_log_internal_timer_events = true;
  }

  // --prof implies --log-code.
  if (FLAG_prof) FLAG_log_code = true;

  // If we're logging anything, we need to open the log file.
  if (Log::InitLogAtStart()) {
    if (strcmp(log_file_name, kLogToConsole) == 0) {
      OpenStdout();
    } else if (strcmp(log_file_name, kLogToTemporaryFile) == 0) {
      OpenTemporaryFile();
    } else {
      OpenFile(log_file_name);
    }
  }
}


void Log::OpenStdout() {
  ASSERT(!IsEnabled());
  output_handle_ = stdout;
}


void Log::OpenTemporaryFile() {
  ASSERT(!IsEnabled());
  output_handle_ = i::OS::OpenTemporaryFile();
}


void Log::OpenFile(const char* name) {
  ASSERT(!IsEnabled());
  output_handle_ = OS::FOpen(name, OS::LogFileOpenMode);
}


FILE* Log::Close() {
  FILE* result = NULL;
  if (output_handle_ != NULL) {
    if (strcmp(FLAG_logfile, kLogToTemporaryFile) != 0) {
      fclose(output_handle_);
    } else {
      result = output_handle_;
    }
  }
  output_handle_ = NULL;

  DeleteArray(message_buffer_);
  message_buffer_ = NULL;

  is_stopped_ = false;
  return result;
}


Log::MessageBuilder::MessageBuilder(Log* log)
  : log_(log),
    lock_guard_(&log_->mutex_),
    pos_(0) {
  ASSERT(log_->message_buffer_ != NULL);
}


void Log::MessageBuilder::Append(const char* format, ...) {
  Vector<char> buf(log_->message_buffer_ + pos_,
                   Log::kMessageBufferSize - pos_);
  va_list args;
  va_start(args, format);
  AppendVA(format, args);
  va_end(args);
  ASSERT(pos_ <= Log::kMessageBufferSize);
}


void Log::MessageBuilder::AppendVA(const char* format, va_list args) {
  Vector<char> buf(log_->message_buffer_ + pos_,
                   Log::kMessageBufferSize - pos_);
  int result = v8::internal::OS::VSNPrintF(buf, format, args);

  // Result is -1 if output was truncated.
  if (result >= 0) {
    pos_ += result;
  } else {
    pos_ = Log::kMessageBufferSize;
  }
  ASSERT(pos_ <= Log::kMessageBufferSize);
}


void Log::MessageBuilder::Append(const char c) {
  if (pos_ < Log::kMessageBufferSize) {
    log_->message_buffer_[pos_++] = c;
  }
  ASSERT(pos_ <= Log::kMessageBufferSize);
}


void Log::MessageBuilder::AppendDoubleQuotedString(const char* string) {
  Append('"');
  for (const char* p = string; *p != '\0'; p++) {
    if (*p == '"') {
      Append('\\');
    }
    Append(*p);
  }
  Append('"');
}


void Log::MessageBuilder::Append(String* str) {
  DisallowHeapAllocation no_gc;  // Ensure string stay valid.
  int length = str->length();
  for (int i = 0; i < length; i++) {
    Append(static_cast<char>(str->Get(i)));
  }
}


void Log::MessageBuilder::AppendAddress(Address addr) {
  Append("0x%" V8PRIxPTR, addr);
}


void Log::MessageBuilder::AppendSymbolName(Symbol* symbol) {
  ASSERT(symbol);
  Append("symbol(");
  if (!symbol->name()->IsUndefined()) {
    Append("\"");
    AppendDetailed(String::cast(symbol->name()), false);
    Append("\" ");
  }
  Append("hash %x)", symbol->Hash());
}


void Log::MessageBuilder::AppendDetailed(String* str, bool show_impl_info) {
  if (str == NULL) return;
  DisallowHeapAllocation no_gc;  // Ensure string stay valid.
  int len = str->length();
  if (len > 0x1000)
    len = 0x1000;
  if (show_impl_info) {
    Append(str->IsOneByteRepresentation() ? 'a' : '2');
    if (StringShape(str).IsExternal())
      Append('e');
    if (StringShape(str).IsInternalized())
      Append('#');
    Append(":%i:", str->length());
  }
  for (int i = 0; i < len; i++) {
    uc32 c = str->Get(i);
    if (c > 0xff) {
      Append("\\u%04x", c);
    } else if (c < 32 || c > 126) {
      Append("\\x%02x", c);
    } else if (c == ',') {
      Append("\\,");
    } else if (c == '\\') {
      Append("\\\\");
    } else if (c == '\"') {
      Append("\"\"");
    } else {
      Append("%lc", c);
    }
  }
}


void Log::MessageBuilder::AppendStringPart(const char* str, int len) {
  if (pos_ + len > Log::kMessageBufferSize) {
    len = Log::kMessageBufferSize - pos_;
    ASSERT(len >= 0);
    if (len == 0) return;
  }
  Vector<char> buf(log_->message_buffer_ + pos_,
                   Log::kMessageBufferSize - pos_);
  OS::StrNCpy(buf, str, len);
  pos_ += len;
  ASSERT(pos_ <= Log::kMessageBufferSize);
}


void Log::MessageBuilder::WriteToLogFile() {
  ASSERT(pos_ <= Log::kMessageBufferSize);
  const int written = log_->WriteToFile(log_->message_buffer_, pos_);
  if (written != pos_) {
    log_->stop();
    log_->logger_->LogFailure();
  }
}


} }  // namespace v8::internal
