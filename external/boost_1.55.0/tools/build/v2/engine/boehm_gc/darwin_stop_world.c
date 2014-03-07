#include "private/pthread_support.h"

/* This probably needs more porting work to ppc64. */

# if defined(GC_DARWIN_THREADS)

/* From "Inside Mac OS X - Mach-O Runtime Architecture" published by Apple
   Page 49:
   "The space beneath the stack pointer, where a new stack frame would normally
   be allocated, is called the red zone. This area as shown in Figure 3-2 may
   be used for any purpose as long as a new stack frame does not need to be
   added to the stack."

   Page 50: "If a leaf procedure's red zone usage would exceed 224 bytes, then
   it must set up a stack frame just like routines that call other routines."
*/
#ifdef POWERPC
# if CPP_WORDSZ == 32
#   define PPC_RED_ZONE_SIZE 224
# elif CPP_WORDSZ == 64
#   define PPC_RED_ZONE_SIZE 320
# endif
#endif

typedef struct StackFrame {
  unsigned long	savedSP;
  unsigned long	savedCR;
  unsigned long	savedLR;
  unsigned long	reserved[2];
  unsigned long	savedRTOC;
} StackFrame;

unsigned long FindTopOfStack(unsigned long stack_start)
{
  StackFrame	*frame;

  if (stack_start == 0) {
# ifdef POWERPC
#   if CPP_WORDSZ == 32
      __asm__ volatile("lwz	%0,0(r1)" : "=r" (frame));
#   else
      __asm__ volatile("ld	%0,0(r1)" : "=r" (frame));
#   endif
# endif
  } else {
    frame = (StackFrame *)stack_start;
  }

# ifdef DEBUG_THREADS
    /* GC_printf("FindTopOfStack start at sp = %p\n", frame); */
# endif
  do {
    if (frame->savedSP == 0)
      break;
    /* if there are no more stack frames, stop */

    frame = (StackFrame*)frame->savedSP;

    /* we do these next two checks after going to the next frame
       because the LR for the first stack frame in the loop
       is not set up on purpose, so we shouldn't check it. */
    if ((frame->savedLR & ~3) == 0)
      break; /* if the next LR is bogus, stop */
    if ((~(frame->savedLR) & ~3) == 0)
      break; /* ditto */
  } while (1);

# ifdef DEBUG_THREADS
    /* GC_printf("FindTopOfStack finish at sp = %p\n", frame); */
# endif

  return (unsigned long)frame;
}

#ifdef DARWIN_DONT_PARSE_STACK
void GC_push_all_stacks()
{
  int i;
  kern_return_t r;
  GC_thread p;
  pthread_t me;
  ptr_t lo, hi;
  GC_THREAD_STATE_T state;
  /* MACHINE_THREAD_STATE_COUNT doesn't seem to be defined everywhere.	*/
  /* Hence we use our own version.					*/
  mach_msg_type_number_t thread_state_count = GC_MACH_THREAD_STATE_COUNT;

  me = pthread_self();
  if (!GC_thr_initialized)
    GC_thr_init();

  for(i = 0; i < THREAD_TABLE_SZ; i++) {
    for(p = GC_threads[i]; p != 0; p = p->next) {
      if(p->flags & FINISHED) continue;
      if(pthread_equal(p->id, me)) {
	lo = GC_approx_sp();
      } else {
	/* Get the thread state (registers, etc) */
	r = thread_get_state(p->stop_info.mach_thread, GC_MACH_THREAD_STATE,
			     (natural_t*)&state, &thread_state_count);

#       ifdef DEBUG_THREADS
	  GC_printf("thread_get_state return value = %d\n", r);
#	endif

	if(r != KERN_SUCCESS)
	  ABORT("thread_get_state failed");

#       if defined(I386)
	  lo = (void*)state . THREAD_FLD (esp);
	  GC_push_one(state . THREAD_FLD (eax));
	  GC_push_one(state . THREAD_FLD (ebx));
	  GC_push_one(state . THREAD_FLD (ecx));
	  GC_push_one(state . THREAD_FLD (edx));
	  GC_push_one(state . THREAD_FLD (edi));
	  GC_push_one(state . THREAD_FLD (esi));
	  GC_push_one(state . THREAD_FLD (ebp));

#       elif defined(X86_64)
	  lo = (void*)state . THREAD_FLD (rsp);
	  GC_push_one(state . THREAD_FLD (rax));
	  GC_push_one(state . THREAD_FLD (rbx));
	  GC_push_one(state . THREAD_FLD (rcx));
	  GC_push_one(state . THREAD_FLD (rdx));
	  GC_push_one(state . THREAD_FLD (rdi));
	  GC_push_one(state . THREAD_FLD (rsi));
	  GC_push_one(state . THREAD_FLD (rbp));
	  GC_push_one(state . THREAD_FLD (rsp));
	  GC_push_one(state . THREAD_FLD (r8));
	  GC_push_one(state . THREAD_FLD (r9));
	  GC_push_one(state . THREAD_FLD (r10));
	  GC_push_one(state . THREAD_FLD (r11));
	  GC_push_one(state . THREAD_FLD (r12));
	  GC_push_one(state . THREAD_FLD (r13));
	  GC_push_one(state . THREAD_FLD (r14));
	  GC_push_one(state . THREAD_FLD (r15));
	  GC_push_one(state . THREAD_FLD (rip));
	  GC_push_one(state . THREAD_FLD (rflags));
	  GC_push_one(state . THREAD_FLD (cs));
	  GC_push_one(state . THREAD_FLD (fs));
	  GC_push_one(state . THREAD_FLD (gs));

#       elif defined(POWERPC)
	  lo = (void*)(state . THREAD_FLD (r1) - PPC_RED_ZONE_SIZE);

	  GC_push_one(state . THREAD_FLD (r0));
	  GC_push_one(state . THREAD_FLD (r2));
	  GC_push_one(state . THREAD_FLD (r3));
	  GC_push_one(state . THREAD_FLD (r4));
	  GC_push_one(state . THREAD_FLD (r5));
	  GC_push_one(state . THREAD_FLD (r6));
	  GC_push_one(state . THREAD_FLD (r7));
	  GC_push_one(state . THREAD_FLD (r8));
	  GC_push_one(state . THREAD_FLD (r9));
	  GC_push_one(state . THREAD_FLD (r10));
	  GC_push_one(state . THREAD_FLD (r11));
	  GC_push_one(state . THREAD_FLD (r12));
	  GC_push_one(state . THREAD_FLD (r13));
	  GC_push_one(state . THREAD_FLD (r14));
	  GC_push_one(state . THREAD_FLD (r15));
	  GC_push_one(state . THREAD_FLD (r16));
	  GC_push_one(state . THREAD_FLD (r17));
	  GC_push_one(state . THREAD_FLD (r18));
	  GC_push_one(state . THREAD_FLD (r19));
	  GC_push_one(state . THREAD_FLD (r20));
	  GC_push_one(state . THREAD_FLD (r21));
	  GC_push_one(state . THREAD_FLD (r22));
	  GC_push_one(state . THREAD_FLD (r23));
	  GC_push_one(state . THREAD_FLD (r24));
	  GC_push_one(state . THREAD_FLD (r25));
	  GC_push_one(state . THREAD_FLD (r26));
	  GC_push_one(state . THREAD_FLD (r27));
	  GC_push_one(state . THREAD_FLD (r28));
	  GC_push_one(state . THREAD_FLD (r29));
	  GC_push_one(state . THREAD_FLD (r30));
	  GC_push_one(state . THREAD_FLD (r31));
#	else
#	  error FIXME for non-x86 || ppc architectures
#	endif
      } /* p != me */
      if(p->flags & MAIN_THREAD)
	hi = GC_stackbottom;
      else
	hi = p->stack_end;
#     if DEBUG_THREADS
        GC_printf("Darwin: Stack for thread 0x%lx = [%lx,%lx)\n",
		  (unsigned long) p -> id, (unsigned long) lo,
		  (unsigned long) hi);
#     endif
      GC_push_all_stack(lo, hi);
    } /* for(p=GC_threads[i]...) */
  } /* for(i=0;i<THREAD_TABLE_SZ...) */
}

#else /* !DARWIN_DONT_PARSE_STACK; Use FindTopOfStack() */

void GC_push_all_stacks()
{
  unsigned int i;
  task_t my_task;
  kern_return_t r;
  mach_port_t me;
  ptr_t lo, hi;
  thread_act_array_t act_list = 0;
  mach_msg_type_number_t listcount = 0;

  me = mach_thread_self();
  if (!GC_thr_initialized)
    GC_thr_init();

  my_task = current_task();
  r = task_threads(my_task, &act_list, &listcount);
  if(r != KERN_SUCCESS)
    ABORT("task_threads failed");
  for(i = 0; i < listcount; i++) {
    thread_act_t thread = act_list[i];
    if (thread == me) {
      lo = GC_approx_sp();
      hi = (ptr_t)FindTopOfStack(0);
    } else {
#     if defined(POWERPC)
        GC_THREAD_STATE_T info;
	mach_msg_type_number_t outCount = THREAD_STATE_MAX;
	r = thread_get_state(thread, GC_MACH_THREAD_STATE, (natural_t *)&info,
			     &outCount);
	if(r != KERN_SUCCESS)
	  ABORT("task_get_state failed");

	lo = (void*)(info . THREAD_FLD (r1) - PPC_RED_ZONE_SIZE);
	hi = (ptr_t)FindTopOfStack(info . THREAD_FLD (r1));

	GC_push_one(info . THREAD_FLD (r0));
	GC_push_one(info . THREAD_FLD (r2));
	GC_push_one(info . THREAD_FLD (r3));
	GC_push_one(info . THREAD_FLD (r4));
	GC_push_one(info . THREAD_FLD (r5));
	GC_push_one(info . THREAD_FLD (r6));
	GC_push_one(info . THREAD_FLD (r7));
	GC_push_one(info . THREAD_FLD (r8));
	GC_push_one(info . THREAD_FLD (r9));
	GC_push_one(info . THREAD_FLD (r10));
	GC_push_one(info . THREAD_FLD (r11));
	GC_push_one(info . THREAD_FLD (r12));
	GC_push_one(info . THREAD_FLD (r13));
	GC_push_one(info . THREAD_FLD (r14));
	GC_push_one(info . THREAD_FLD (r15));
	GC_push_one(info . THREAD_FLD (r16));
	GC_push_one(info . THREAD_FLD (r17));
	GC_push_one(info . THREAD_FLD (r18));
	GC_push_one(info . THREAD_FLD (r19));
	GC_push_one(info . THREAD_FLD (r20));
	GC_push_one(info . THREAD_FLD (r21));
	GC_push_one(info . THREAD_FLD (r22));
	GC_push_one(info . THREAD_FLD (r23));
	GC_push_one(info . THREAD_FLD (r24));
	GC_push_one(info . THREAD_FLD (r25));
	GC_push_one(info . THREAD_FLD (r26));
	GC_push_one(info . THREAD_FLD (r27));
	GC_push_one(info . THREAD_FLD (r28));
	GC_push_one(info . THREAD_FLD (r29));
	GC_push_one(info . THREAD_FLD (r30));
	GC_push_one(info . THREAD_FLD (r31));

#     elif defined(I386)
	/* FIXME: Remove after testing:	*/
	WARN("This is completely untested and likely will not work\n", 0);
	GC_THREAD_STATE_T info;
	mach_msg_type_number_t outCount = THREAD_STATE_MAX;
	r = thread_get_state(thread, GC_MACH_THREAD_STATE, (natural_t *)&info,
			     &outCount);
	if(r != KERN_SUCCESS)
	  ABORT("task_get_state failed");

	lo = (void*)info . THREAD_FLD (esp);
	hi = (ptr_t)FindTopOfStack(info . THREAD_FLD (esp));

	GC_push_one(info . THREAD_FLD (eax));
	GC_push_one(info . THREAD_FLD (ebx));
	GC_push_one(info . THREAD_FLD (ecx));
	GC_push_one(info . THREAD_FLD (edx));
	GC_push_one(info . THREAD_FLD (edi));
	GC_push_one(info . THREAD_FLD (esi));
	/* GC_push_one(info . THREAD_FLD (ebp));  */
	/* GC_push_one(info . THREAD_FLD (esp));  */
	GC_push_one(info . THREAD_FLD (ss));
	GC_push_one(info . THREAD_FLD (eip));
	GC_push_one(info . THREAD_FLD (cs));
	GC_push_one(info . THREAD_FLD (ds));
	GC_push_one(info . THREAD_FLD (es));
	GC_push_one(info . THREAD_FLD (fs));
	GC_push_one(info . THREAD_FLD (gs));

#     elif defined(X86_64)
	GC_THREAD_STATE_T info;
	mach_msg_type_number_t outCount = THREAD_STATE_MAX;
	r = thread_get_state(thread, GC_MACH_THREAD_STATE, (natural_t *)&info,
			     &outCount);
	if(r != KERN_SUCCESS)
	  ABORT("task_get_state failed");

	lo = (void*)info . THREAD_FLD (rsp);
	hi = (ptr_t)FindTopOfStack(info . THREAD_FLD (rsp));

	GC_push_one(info . THREAD_FLD (rax));
	GC_push_one(info . THREAD_FLD (rbx));
	GC_push_one(info . THREAD_FLD (rcx));
	GC_push_one(info . THREAD_FLD (rdx));
	GC_push_one(info . THREAD_FLD (rdi));
	GC_push_one(info . THREAD_FLD (rsi));
	GC_push_one(info . THREAD_FLD (rbp));
	GC_push_one(info . THREAD_FLD (rsp));
	GC_push_one(info . THREAD_FLD (r8));
	GC_push_one(info . THREAD_FLD (r9));
	GC_push_one(info . THREAD_FLD (r10));
	GC_push_one(info . THREAD_FLD (r11));
	GC_push_one(info . THREAD_FLD (r12));
	GC_push_one(info . THREAD_FLD (r13));
	GC_push_one(info . THREAD_FLD (r14));
	GC_push_one(info . THREAD_FLD (r15));
	GC_push_one(info . THREAD_FLD (rip));
	GC_push_one(info . THREAD_FLD (rflags));
	GC_push_one(info . THREAD_FLD (cs));
	GC_push_one(info . THREAD_FLD (fs));
	GC_push_one(info . THREAD_FLD (gs));

#     else
#	error FIXME for non-x86 || ppc architectures
#     endif
      }
#     if DEBUG_THREADS
        GC_printf("Darwin: Stack for thread 0x%lx = [%p,%p)\n",
		  (unsigned long) thread, lo, hi);
#     endif
      GC_push_all_stack(lo, hi);
      mach_port_deallocate(my_task, thread);
    } /* for(p=GC_threads[i]...) */
    vm_deallocate(my_task, (vm_address_t)act_list,
		  sizeof(thread_t) * listcount);
    mach_port_deallocate(my_task, me);
}
#endif /* !DARWIN_DONT_PARSE_STACK */

static mach_port_t GC_mach_handler_thread;
static int GC_use_mach_handler_thread = 0;

static struct GC_mach_thread GC_mach_threads[THREAD_TABLE_SZ];
static int GC_mach_threads_count;

void GC_stop_init()
{
  int i;

  for (i = 0; i < THREAD_TABLE_SZ; i++) {
    GC_mach_threads[i].thread = 0;
    GC_mach_threads[i].already_suspended = 0;
  }
  GC_mach_threads_count = 0;
}

/* returns true if there's a thread in act_list that wasn't in old_list */
int GC_suspend_thread_list(thread_act_array_t act_list, int count,
			   thread_act_array_t old_list, int old_count)
{
  mach_port_t my_thread = mach_thread_self();
  int i, j;

  int changed = 0;

  for(i = 0; i < count; i++) {
    thread_act_t thread = act_list[i];
#   if DEBUG_THREADS
      GC_printf("Attempting to suspend thread %p\n", thread);
#   endif
    /* find the current thread in the old list */
    int found = 0;
    for(j = 0; j < old_count; j++) {
      thread_act_t old_thread = old_list[j];
      if (old_thread == thread) {
	found = 1;
	break;
      }
    }
    if (!found) {
      /* add it to the GC_mach_threads list */
      GC_mach_threads[GC_mach_threads_count].thread = thread;
      /* default is not suspended */
      GC_mach_threads[GC_mach_threads_count].already_suspended = 0;
      changed = 1;
    }

    if (thread != my_thread
	&& (!GC_use_mach_handler_thread
	    || (GC_use_mach_handler_thread
		&& GC_mach_handler_thread != thread))) {
      struct thread_basic_info info;
      mach_msg_type_number_t outCount = THREAD_INFO_MAX;
      kern_return_t kern_result = thread_info(thread, THREAD_BASIC_INFO,
				(thread_info_t)&info, &outCount);
      if(kern_result != KERN_SUCCESS) {
	/* the thread may have quit since the thread_threads () call
	 * we mark already_suspended so it's not dealt with anymore later
	 */
	if (!found) {
	  GC_mach_threads[GC_mach_threads_count].already_suspended = TRUE;
	  GC_mach_threads_count++;
	}
	continue;
      }
#     if DEBUG_THREADS
        GC_printf("Thread state for 0x%lx = %d\n", (unsigned long)thread,
		  info.run_state);
#     endif
      if (!found) {
	GC_mach_threads[GC_mach_threads_count].already_suspended
	  = info.suspend_count;
      }
      if (info.suspend_count)
	continue;

#     if DEBUG_THREADS
        GC_printf("Suspending 0x%lx\n", (unsigned long)thread);
#     endif
      /* Suspend the thread */
      kern_result = thread_suspend(thread);
      if(kern_result != KERN_SUCCESS) {
	/* the thread may have quit since the thread_threads () call
	 * we mark already_suspended so it's not dealt with anymore later
	 */
	if (!found) {
	  GC_mach_threads[GC_mach_threads_count].already_suspended = TRUE;
	  GC_mach_threads_count++;
	}
	continue;
      }
    }
    if (!found) GC_mach_threads_count++;
  }
  mach_port_deallocate(current_task(), my_thread);
  return changed;
}


/* Caller holds allocation lock.	*/
void GC_stop_world()
{
    unsigned int i, changes;
    task_t my_task = current_task();
    mach_port_t my_thread = mach_thread_self();
    kern_return_t kern_result;
    thread_act_array_t act_list, prev_list;
    mach_msg_type_number_t listcount, prevcount;

#   if DEBUG_THREADS
      GC_printf("Stopping the world from 0x%lx\n",
		(unsigned long)mach_thread_self());
#   endif

    /* clear out the mach threads list table */
    GC_stop_init();

    /* Make sure all free list construction has stopped before we start. */
    /* No new construction can start, since free list construction is	*/
    /* required to acquire and release the GC lock before it starts,	*/
    /* and we have the lock.						*/
#   ifdef PARALLEL_MARK
      GC_acquire_mark_lock();
      GC_ASSERT(GC_fl_builder_count == 0);
      /* We should have previously waited for it to become zero. */
#   endif /* PARALLEL_MARK */

    /* Loop stopping threads until you have gone over the whole list
       twice without a new one appearing. thread_create() won't
       return (and thus the thread stop) until the new thread
       exists, so there is no window whereby you could stop a
       thread, recognise it is stopped, but then have a new thread
       it created before stopping show up later.
    */

    changes = 1;
    prev_list = NULL;
    prevcount = 0;
    do {
      int result;
      kern_result = task_threads(my_task, &act_list, &listcount);

      if(kern_result == KERN_SUCCESS) {
	result = GC_suspend_thread_list(act_list, listcount, prev_list,
					prevcount);
	changes = result;

	if(prev_list != NULL) {
	  for(i = 0; i < prevcount; i++)
	    mach_port_deallocate(my_task, prev_list[i]);

	  vm_deallocate(my_task, (vm_address_t)prev_list,
			sizeof(thread_t) * prevcount);
	}
	prev_list = act_list;
	prevcount = listcount;
      }
    } while (changes);
    GC_ASSERT(prev_list != 0);
    for(i = 0; i < prevcount; i++)
      mach_port_deallocate(my_task, prev_list[i]);

    vm_deallocate(my_task, (vm_address_t)act_list,
		  sizeof(thread_t) * listcount);

#   ifdef MPROTECT_VDB
      if(GC_incremental) {
	extern void GC_mprotect_stop();
	GC_mprotect_stop();
      }
#   endif

#   ifdef PARALLEL_MARK
      GC_release_mark_lock();
#   endif
#   if DEBUG_THREADS
      GC_printf("World stopped from 0x%lx\n", (unsigned long)my_thread);
#   endif

    mach_port_deallocate(my_task, my_thread);
}

/* Caller holds allocation lock, and has held it continuously since	*/
/* the world stopped.							*/
void GC_start_world()
{
  task_t my_task = current_task();
  mach_port_t my_thread = mach_thread_self();
  unsigned int i;
  int j;
  kern_return_t kern_result;
  thread_act_array_t act_list;
  mach_msg_type_number_t listcount;
  struct thread_basic_info info;
  mach_msg_type_number_t outCount = THREAD_INFO_MAX;

#   if DEBUG_THREADS
      GC_printf("World starting\n");
#   endif

#   ifdef MPROTECT_VDB
      if(GC_incremental) {
	extern void GC_mprotect_resume();
	GC_mprotect_resume();
      }
#   endif

    kern_result = task_threads(my_task, &act_list, &listcount);
    for(i = 0; i < listcount; i++) {
      thread_act_t thread = act_list[i];
      if (thread != my_thread
	  && (!GC_use_mach_handler_thread
	      || (GC_use_mach_handler_thread
		  && GC_mach_handler_thread != thread))) {
	for(j = 0; j < GC_mach_threads_count; j++) {
	  if (thread == GC_mach_threads[j].thread) {
	    if (GC_mach_threads[j].already_suspended) {
#             if DEBUG_THREADS
	        GC_printf("Not resuming already suspended thread %p\n", thread);
#             endif
	      continue;
	    }
	    kern_result = thread_info(thread, THREAD_BASIC_INFO,
				      (thread_info_t)&info, &outCount);
	    if(kern_result != KERN_SUCCESS)
	      ABORT("thread_info failed");
#           if DEBUG_THREADS
	      GC_printf("Thread state for 0x%lx = %d\n", (unsigned long)thread,
			 info.run_state);
	      GC_printf("Resuming 0x%lx\n", (unsigned long)thread);
#           endif
	    /* Resume the thread */
	    kern_result = thread_resume(thread);
	    if(kern_result != KERN_SUCCESS)
	      ABORT("thread_resume failed");
	  }
	}
      }
      mach_port_deallocate(my_task, thread);
    }
    vm_deallocate(my_task, (vm_address_t)act_list,
		  sizeof(thread_t) * listcount);

    mach_port_deallocate(my_task, my_thread);
#   if DEBUG_THREADS
      GC_printf("World started\n");
#   endif
}

void GC_darwin_register_mach_handler_thread(mach_port_t thread)
{
  GC_mach_handler_thread = thread;
  GC_use_mach_handler_thread = 1;
}

#endif
