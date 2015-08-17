// Copyright 2006-2008 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/logging.h"

#if V8_LIBC_GLIBC || V8_OS_BSD
# include <cxxabi.h>
# include <execinfo.h>
#elif V8_OS_QNX
# include <backtrace.h>
#endif  // V8_LIBC_GLIBC || V8_OS_BSD
#include <stdio.h>
#include <stdlib.h>

#include "src/base/platform/platform.h"

namespace v8 {
namespace base {

// Attempts to dump a backtrace (if supported).
void DumpBacktrace() {
#if V8_LIBC_GLIBC || V8_OS_BSD
  void* trace[100];
  int size = backtrace(trace, arraysize(trace));
  char** symbols = backtrace_symbols(trace, size);
  OS::PrintError("\n==== C stack trace ===============================\n\n");
  if (size == 0) {
    OS::PrintError("(empty)\n");
  } else if (symbols == NULL) {
    OS::PrintError("(no symbols)\n");
  } else {
    for (int i = 1; i < size; ++i) {
      OS::PrintError("%2d: ", i);
      char mangled[201];
      if (sscanf(symbols[i], "%*[^(]%*[(]%200[^)+]", mangled) == 1) {  // NOLINT
        int status;
        size_t length;
        char* demangled = abi::__cxa_demangle(mangled, NULL, &length, &status);
        OS::PrintError("%s\n", demangled != NULL ? demangled : mangled);
        free(demangled);
      } else {
        OS::PrintError("??\n");
      }
    }
  }
  free(symbols);
#elif V8_OS_QNX
  char out[1024];
  bt_accessor_t acc;
  bt_memmap_t memmap;
  bt_init_accessor(&acc, BT_SELF);
  bt_load_memmap(&acc, &memmap);
  bt_sprn_memmap(&memmap, out, sizeof(out));
  OS::PrintError(out);
  bt_addr_t trace[100];
  int size = bt_get_backtrace(&acc, trace, arraysize(trace));
  OS::PrintError("\n==== C stack trace ===============================\n\n");
  if (size == 0) {
    OS::PrintError("(empty)\n");
  } else {
    bt_sprnf_addrs(&memmap, trace, size, const_cast<char*>("%a\n"),
                   out, sizeof(out), NULL);
    OS::PrintError(out);
  }
  bt_unload_memmap(&memmap);
  bt_release_accessor(&acc);
#endif  // V8_LIBC_GLIBC || V8_OS_BSD
}

} }  // namespace v8::base


// Contains protection against recursive calls (faults while handling faults).
extern "C" void V8_Fatal(const char* file, int line, const char* format, ...) {
  fflush(stdout);
  fflush(stderr);
  v8::base::OS::PrintError("\n\n#\n# Fatal error in %s, line %d\n# ", file,
                           line);
  va_list arguments;
  va_start(arguments, format);
  v8::base::OS::VPrintError(format, arguments);
  va_end(arguments);
  v8::base::OS::PrintError("\n#\n");
  v8::base::DumpBacktrace();
  fflush(stderr);
  v8::base::OS::Abort();
}
