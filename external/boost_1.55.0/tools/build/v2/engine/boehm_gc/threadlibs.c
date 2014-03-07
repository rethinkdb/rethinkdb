# include "gc_config_macros.h"
# include "private/gcconfig.h"
# include <stdio.h>

int main()
{
#   if defined(GC_USE_LD_WRAP)
	printf("-Wl,--wrap -Wl,dlopen "
	       "-Wl,--wrap -Wl,pthread_create -Wl,--wrap -Wl,pthread_join "
	       "-Wl,--wrap -Wl,pthread_detach "
	       "-Wl,--wrap -Wl,pthread_sigmask -Wl,--wrap -Wl,sleep\n");
#   endif
#   if defined(GC_LINUX_THREADS) || defined(GC_IRIX_THREADS) \
	|| defined(GC_DARWIN_THREADS) || defined(GC_AIX_THREADS) \
	|| defined(GC_GNU_THREADS)
#       ifdef GC_USE_DLOPEN_WRAP
	  printf("-ldl ");
#	endif
        printf("-lpthread\n");
#   endif
#   if defined(GC_FREEBSD_THREADS)
#       ifdef GC_USE_DLOPEN_WRAP
	  printf("-ldl ");
#	endif
#       if (__FREEBSD_version >= 500000)
          printf("-lpthread\n");
#       else
          printf("-pthread\n");
#       endif
#   endif
#   if defined(GC_NETBSD_THREADS)
	  printf("-lpthread -lrt\n");
#   endif

#   if defined(GC_HPUX_THREADS) || defined(GC_OSF1_THREADS)
	printf("-lpthread -lrt\n");
#   endif
#   if defined(GC_SOLARIS_THREADS)
        printf("-lthread -lposix4\n");
		/* Is this right for recent versions? */
#   endif
#   if defined(GC_WIN32_THREADS) && defined(CYGWIN32)
        printf("-lpthread\n");
#   endif
#   if defined(GC_WIN32_PTHREADS)
#      ifdef PTW32_STATIC_LIB
	 /* assume suffix s for static version of the win32 pthread library */
         printf("-lpthreadGC2s -lws2_32\n");
#      else
         printf("-lpthreadGC2\n");
#      endif
#   endif
#   if defined(GC_OSF1_THREADS)
	printf("-pthread -lrt"); /* DOB: must be -pthread, not -lpthread */
#   endif
    /* You need GCC 3.0.3 to build this one!           */  
    /* DG/UX native gcc doesnt know what "-pthread" is */
#   if defined(GC_DGUX386_THREADS)
        printf("-ldl -pthread\n");
#   endif
    return 0;
}

