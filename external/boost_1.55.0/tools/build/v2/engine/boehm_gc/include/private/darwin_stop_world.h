#ifndef GC_DARWIN_STOP_WORLD_H
#define GC_DARWIN_STOP_WORLD_H

#if !defined(GC_DARWIN_THREADS)
#error darwin_stop_world.h included without GC_DARWIN_THREADS defined
#endif

#include <mach/mach.h>
#include <mach/thread_act.h>

struct thread_stop_info {
    mach_port_t mach_thread;
};

struct GC_mach_thread {
  thread_act_t thread;
  int already_suspended;
};

void GC_darwin_register_mach_handler_thread(mach_port_t thread);

#endif
