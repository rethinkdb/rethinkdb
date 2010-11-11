/* Copyright (c) 2008, Google Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * --
 * Author: Craig Silverstein
 *
 * C shims for the C++ malloc_hook.h.  See malloc_hook.h for details
 * on how to use these.
 */

#ifndef _MALLOC_HOOK_C_H_
#define _MALLOC_HOOK_C_H_

#include <stddef.h>
#include <sys/types.h>

/* Get the current stack trace.  Try to skip all routines up to and
 * and including the caller of MallocHook::Invoke*.
 * Use "skip_count" (similarly to GetStackTrace from stacktrace.h)
 * as a hint about how many routines to skip if better information
 * is not available.
 */
int MallocHook_GetCallerStackTrace(void** result, int max_depth,
                                   int skip_count);


typedef void (*MallocHook_NewHook)(const void* ptr, size_t size);
MallocHook_NewHook MallocHook_SetNewHook(MallocHook_NewHook hook);

typedef void (*MallocHook_DeleteHook)(const void* ptr);
MallocHook_DeleteHook MallocHook_SetDeleteHook(MallocHook_DeleteHook hook);

typedef void (*MallocHook_PreMmapHook)(const void *start,
                                       size_t size,
                                       int protection,
                                       int flags,
                                       int fd,
                                       off_t offset);
MallocHook_PreMmapHook MallocHook_SetPreMmapHook(MallocHook_PreMmapHook hook);

typedef void (*MallocHook_MmapHook)(const void* result,
                                    const void* start,
                                    size_t size,
                                    int protection,
                                    int flags,
                                    int fd,
                                    off_t offset);
MallocHook_MmapHook MallocHook_SetMmapHook(MallocHook_MmapHook hook);

typedef void (*MallocHook_MunmapHook)(const void* ptr, size_t size);
MallocHook_MunmapHook MallocHook_SetMunmapHook(MallocHook_MunmapHook hook);

typedef void (*MallocHook_MremapHook)(const void* result,
                                      const void* old_addr,
                                      size_t old_size,
                                      size_t new_size,
                                      int flags,
                                      const void* new_addr);
MallocHook_MremapHook MallocHook_SetMremapHook(MallocHook_MremapHook hook);

typedef void (*MallocHook_PreSbrkHook)(ptrdiff_t increment);
MallocHook_PreSbrkHook MallocHook_SetPreSbrkHook(MallocHook_PreSbrkHook hook);

typedef void (*MallocHook_SbrkHook)(const void* result, ptrdiff_t increment);
MallocHook_SbrkHook MallocHook_SetSbrkHook(MallocHook_SbrkHook hook);

#endif /* _MALLOC_HOOK_C_H_ */
