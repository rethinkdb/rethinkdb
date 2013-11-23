/*
 * Copyright (c) 1999, 2007 Apple Inc. All rights reserved.
 * Some parts Copyright (c) 2013 RethinkDB.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 * Modified by RethinkDB to handle RethinkDB coroutines.  Also moved
 * rethinkdb_backtrace here (renamed from backtrace, which was in OS X Libc's
 * gen/backtrace.cc).
 */

/*	Bertrand from vmutils -> CF -> System */

#ifdef __MACH__

#include <pthread.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <stdlib.h>

#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/context_switching.hpp"

int rethinkdb_backtrace(void** buffer, int size) {
    extern void _rethinkdb_thread_stack_pcs(vm_address_t *buffer, unsigned max, unsigned *nb, unsigned skip);
    unsigned int num_frames;
    _rethinkdb_thread_stack_pcs((vm_address_t*)buffer, size, &num_frames, 1);
    while (num_frames >= 1 && buffer[num_frames-1] == NULL) num_frames -= 1;
    return num_frames;
}


#if defined(__i386__) || defined(__x86_64__) || defined(__arm__)
#define FP_LINK_OFFSET 1
#elif defined(__ppc__) || defined(__ppc64__)
#define FP_LINK_OFFSET 2
#else
#error  ********** Unimplemented architecture
#endif

#define	INSTACK(a)	((a) >= stackbot && (a) <= stacktop)
#if defined(__ppc__) || defined(__ppc64__) || defined(__x86_64__)
#define	ISALIGNED(a)	((((uintptr_t)(a)) & 0xf) == 0)
#elif defined(__arm__)
#define	ISALIGNED(a)	((((uintptr_t)(a)) & 0x1) == 0)
#elif defined(__i386__)
#define	ISALIGNED(a)	((((uintptr_t)(a)) & 0xf) == 8)
#endif

__private_extern__  __attribute__((noinline))
void
_rethinkdb_thread_stack_pcs(vm_address_t *buffer, unsigned max,
                            unsigned *nb, unsigned skip)
{
    void *frame, *next;
    void *stacktop;
    void *stackbot;

    {
        // RethinkDB changes here: We add code that gets the stack bounds when
        // on a coroutine stack.
        coro_t *coro = coro_t::self();
        if (coro != NULL) {
            stack_t *stack = coro->get_stack();
            stacktop = stack->get_stack_base();
            stackbot = stack->get_stack_bound();
        } else {
            pthread_t self = pthread_self();
            stacktop = pthread_get_stackaddr_np(self);
            stackbot = static_cast<char *>(stacktop) - pthread_get_stacksize_np(self);
        }
    }

    *nb = 0;

    /* make sure return address is never out of bounds */
    stacktop = static_cast<char *>(stacktop) - (FP_LINK_OFFSET + 1) * sizeof(void *);

    /*
     * The original implementation called the first_frame_address() function,
     * which returned the stack frame pointer.  The problem was that in ppc,
     * it was a leaf function, so no new stack frame was set up with
     * optimization turned on (while a new stack frame was set up without
     * optimization).  We now inline the code to get the stack frame pointer,
     * so we are consistent about the stack frame.
     */
#if defined(__i386__) || defined(__x86_64__) || defined(__arm__)
    frame = __builtin_frame_address(0);
#elif defined(__ppc__) || defined(__ppc64__)
    /* __builtin_frame_address IS BROKEN IN BEAKER: RADAR #2340421 */
    __asm__ volatile("mr %0, r1" : "=r" (frame));
#endif
    if(!INSTACK(frame) || !ISALIGNED(frame))
	return;
#if defined(__ppc__) || defined(__ppc64__)
    /* back up the stack pointer up over the current stack frame */
    next = *(void **)frame;
    if(!INSTACK(next) || !ISALIGNED(next) || next <= frame)
	return;
    frame = next;
#endif
    while (skip--) {
	next = *(void **)frame;
	if(!INSTACK(next) || !ISALIGNED(next) || next <= frame)
	    return;
	frame = next;
    }
    while (max--) {
        buffer[*nb] = *(vm_address_t *)(((void **)frame) + FP_LINK_OFFSET);
        (*nb)++;
	next = *(void **)frame;
	if(!INSTACK(next) || !ISALIGNED(next) || next <= frame)
	    return;
	frame = next;
    }
}

void
rethinkdb_thread_stack_pcs(vm_address_t *buffer, unsigned max, unsigned *nb)
{
    _rethinkdb_thread_stack_pcs(buffer, max, nb, 0);

    // The following prevents thread_stack_pcs() from getting tail-call-optimized into _thread_stack_pcs() on 64-bit environments,
    // thus making the "number of hot frames to skip" be more predictable, giving more consistent backtraces.
    // See <rdar://problem/5364825> "stack logging: frames keep getting truncated" for why this is necessary.
    __asm__ volatile("");
}

#else
#include <execinfo.h>

int rethinkdb_backtrace(void **buffer, int size) {
    return backtrace(buffer, size);
}
#endif  // __MACH__
