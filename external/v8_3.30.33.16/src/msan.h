// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// MemorySanitizer support.

#ifndef V8_MSAN_H_
#define V8_MSAN_H_

#include "src/globals.h"

#ifndef __has_feature
# define __has_feature(x) 0
#endif

#if __has_feature(memory_sanitizer) && !defined(MEMORY_SANITIZER)
# define MEMORY_SANITIZER
#endif

#if defined(MEMORY_SANITIZER)
# include <sanitizer/msan_interface.h>  // NOLINT

// Marks a memory range as uninitialized, as if it was allocated here.
# define MSAN_ALLOCATED_UNINITIALIZED_MEMORY(p, s) \
    __msan_allocated_memory((p), (s))
// Marks a memory range as initialized.
#define MSAN_MEMORY_IS_INITIALIZED(p, s) __msan_unpoison((p), (s))
#else
# define MSAN_ALLOCATED_UNINITIALIZED_MEMORY(p, s)
#define MSAN_MEMORY_IS_INITIALIZED(p, s)
#endif

#endif  // V8_MSAN_H_
