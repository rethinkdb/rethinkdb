The collector supports both incremental collection and threads under
Solaris 2.  The incremental collector normally retrieves page dirty information
through the appropriate /proc calls.  But it can also be configured
(by defining MPROTECT_VDB instead of PROC_VDB in gcconfig.h) to use mprotect
and signals.  This may result in shorter pause times, but it is no longer
safe to issue arbitrary system calls that write to the heap.

Under other UNIX versions,
the collector normally obtains memory through sbrk.  There is some reason
to expect that this is not safe if the client program also calls the system
malloc, or especially realloc.  The sbrk man page strongly suggests this is
not safe: "Many library routines use malloc() internally, so use brk()
and sbrk() only when you know  that malloc() definitely will not be used by
any library routine."  This doesn't make a lot of sense to me, since there
seems to be no documentation as to which routines can transitively call malloc.
Nonetheless, under Solaris2, the collector now (since 4.12) allocates
memory using mmap by default.  (It defines USE_MMAP in gcconfig.h.)
You may want to reverse this decisions if you use -DREDIRECT_MALLOC=...


SOLARIS THREADS:

The collector must be compiled with -DGC_SOLARIS_THREADS (thr_ functions)
or -DGC_THREADS to be thread safe.  This assumes use of the pthread_
interface.  Old style Solaris threads are no longer supported.

It is also essential that gc.h be included in files that call thr_create,
thr_join, thr_suspend, thr_continue, or dlopen.  Gc.h macro defines
these to also do GC bookkeeping, etc.  Gc.h must be included with
one or both of these macros defined, otherwise
these replacements are not visible.
A collector built in this way way only be used by programs that are
linked with the threads library.

Since 5.0 alpha5, dlopen disables collection temporarily,
unless USE_PROC_FOR_LIBRARIES is defined.  In some unlikely cases, this
can result in unpleasant heap growth.  But it seems better than the
race/deadlock issues we had before.

If solaris_threads are used on an X86 processor with malloc redirected to
GC_malloc, it is necessary to call GC_thr_init explicitly before forking the
first thread.  (This avoids a deadlock arising from calling GC_thr_init
with the allocation lock held.)

It appears that there is a problem in using gc_cpp.h in conjunction with
Solaris threads and Sun's C++ runtime.  Apparently the overloaded new operator
is invoked by some iostream initialization code before threads are correctly
initialized.  As a result, call to thr_self() in garbage collector
initialization  segfaults.  Currently the only known workaround is to not
invoke the garbage collector from a user defined global operator new, or to
have it invoke the garbage-collector's allocators only after main has started.
(Note that the latter requires a moderately expensive test in operator
delete.)

I encountered "symbol <unknown>: offet .... is non-aligned" errors.  These
appear to be traceable to the use of the GNU assembler with the Sun linker.
The former appears to generate a relocation not understood by the latter.
The fix appears to be to use a consistent tool chain.  (As a non-Solaris-expert
my solution involved hacking the libtool script, but I'm sure you can
do something less ugly.)

Hans-J. Boehm
(The above contains my personal opinions, which are probably not shared
by anyone else.)
