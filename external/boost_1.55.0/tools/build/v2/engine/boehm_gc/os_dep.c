/*
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1995 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1996-1999 by Silicon Graphics.  All rights reserved.
 * Copyright (c) 1999 by Hewlett-Packard Company.  All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */

# include "private/gc_priv.h"
# ifdef THREADS
#   include "atomic_ops.h"
# endif

# if defined(LINUX) && !defined(POWERPC)
#   include <linux/version.h>
#   if (LINUX_VERSION_CODE <= 0x10400)
      /* Ugly hack to get struct sigcontext_struct definition.  Required      */
      /* for some early 1.3.X releases.  Will hopefully go away soon. */
      /* in some later Linux releases, asm/sigcontext.h may have to   */
      /* be included instead.                                         */
#     define __KERNEL__
#     include <asm/signal.h>
#     undef __KERNEL__
#   else
      /* Kernels prior to 2.1.1 defined struct sigcontext_struct instead of */
      /* struct sigcontext.  libc6 (glibc2) uses "struct sigcontext" in     */
      /* prototypes, so we have to include the top-level sigcontext.h to    */
      /* make sure the former gets defined to be the latter if appropriate. */
#     include <features.h>
#     if 2 <= __GLIBC__
#       if 2 == __GLIBC__ && 0 == __GLIBC_MINOR__
	  /* glibc 2.1 no longer has sigcontext.h.  But signal.h	*/
	  /* has the right declaration for glibc 2.1.			*/
#         include <sigcontext.h>
#       endif /* 0 == __GLIBC_MINOR__ */
#     else /* not 2 <= __GLIBC__ */
        /* libc5 doesn't have <sigcontext.h>: go directly with the kernel   */
        /* one.  Check LINUX_VERSION_CODE to see which we should reference. */
#       include <asm/sigcontext.h>
#     endif /* 2 <= __GLIBC__ */
#   endif
# endif
# if !defined(OS2) && !defined(PCR) && !defined(AMIGA) && !defined(MACOS) \
    && !defined(MSWINCE)
#   include <sys/types.h>
#   if !defined(MSWIN32)
#   	include <unistd.h>
#   endif
# endif

# include <stdio.h>
# if defined(MSWINCE)
#   define SIGSEGV 0 /* value is irrelevant */
# else
#   include <signal.h>
# endif

#ifdef UNIX_LIKE
# include <fcntl.h>
#endif

#if defined(LINUX) || defined(LINUX_STACKBOTTOM)
# include <ctype.h>
#endif

/* Blatantly OS dependent routines, except for those that are related 	*/
/* to dynamic loading.							*/

#ifdef AMIGA
# define GC_AMIGA_DEF
# include "AmigaOS.c"
# undef GC_AMIGA_DEF
#endif

#if defined(MSWIN32) || defined(MSWINCE) || defined(CYGWIN32)
# define WIN32_LEAN_AND_MEAN
# define NOSERVICE
# include <windows.h>
  /* It's not clear this is completely kosher under Cygwin.  But it	*/
  /* allows us to get a working GC_get_stack_base.			*/
#endif

#ifdef MACOS
# include <Processes.h>
#endif

#ifdef IRIX5
# include <sys/uio.h>
# include <malloc.h>   /* for locking */
#endif

#if defined(LINUX) || defined(FREEBSD) || defined(SOLARIS) || defined(IRIX5) \
	|| defined(USE_MMAP) || defined(USE_MUNMAP)
# define MMAP_SUPPORTED
#endif

#if defined(MMAP_SUPPORTED) || defined(ADD_HEAP_GUARD_PAGES)
# if defined(USE_MUNMAP) && !defined(USE_MMAP)
    --> USE_MUNMAP requires USE_MMAP
# endif
# include <sys/types.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <errno.h>
#endif

#ifdef DARWIN
/* for get_etext and friends */
#include <mach-o/getsect.h>
#endif

#ifdef DJGPP
  /* Apparently necessary for djgpp 2.01.  May cause problems with	*/
  /* other versions.							*/
  typedef long unsigned int caddr_t;
#endif

#ifdef PCR
# include "il/PCR_IL.h"
# include "th/PCR_ThCtl.h"
# include "mm/PCR_MM.h"
#endif

#if !defined(NO_EXECUTE_PERMISSION)
# define OPT_PROT_EXEC PROT_EXEC
#else
# define OPT_PROT_EXEC 0
#endif

#if defined(LINUX) && \
    (defined(USE_PROC_FOR_LIBRARIES) || defined(IA64) || !defined(SMALL_CONFIG))
# define NEED_PROC_MAPS
#endif

#ifdef NEED_PROC_MAPS
/* We need to parse /proc/self/maps, either to find dynamic libraries,	*/
/* and/or to find the register backing store base (IA64).  Do it once	*/
/* here.								*/

#define READ read

/* Repeatedly perform a read call until the buffer is filled or	*/
/* we encounter EOF.						*/
ssize_t GC_repeat_read(int fd, char *buf, size_t count)
{
    ssize_t num_read = 0;
    ssize_t result;
    
    while (num_read < count) {
	result = READ(fd, buf + num_read, count - num_read);
	if (result < 0) return result;
	if (result == 0) break;
	num_read += result;
    }
    return num_read;
}

/* Determine the length of a file by incrementally reading it into a 	*/
/* This would be sily to use on a file supporting lseek, but Linux	*/
/* /proc files usually do not.						*/
size_t GC_get_file_len(int f)
{
    size_t total = 0;
    ssize_t result;
#   define GET_FILE_LEN_BUF_SZ 500
    char buf[GET_FILE_LEN_BUF_SZ];

    do {
	result = read(f, buf, GET_FILE_LEN_BUF_SZ);
	if (result == -1) return 0;
	total += result;
    } while (result > 0);
    return total;
}

size_t GC_get_maps_len(void)
{
    int f = open("/proc/self/maps", O_RDONLY);
    size_t result = GC_get_file_len(f);
    close(f);
    return result;
}

/*
 * Copy the contents of /proc/self/maps to a buffer in our address space.
 * Return the address of the buffer, or zero on failure.
 * This code could be simplified if we could determine its size
 * ahead of time.
 */
char * GC_get_maps(void)
{
    int f;
    int result;
    static char init_buf[1];
    static char *maps_buf = init_buf;
    static size_t maps_buf_sz = 1;
    size_t maps_size, old_maps_size = 0;

    /* The buffer is essentially static, so there must be a single client. */
    GC_ASSERT(I_HOLD_LOCK());

    /* Note that in the presence of threads, the maps file can	*/
    /* essentially shrink asynchronously and unexpectedly as 	*/
    /* threads that we already think of as dead release their	*/
    /* stacks.  And there is no easy way to read the entire	*/
    /* file atomically.  This is arguably a misfeature of the	*/
    /* /proc/.../maps interface.				*/

    /* Since we dont believe the file can grow			*/
    /* asynchronously, it should suffice to first determine	*/
    /* the size (using lseek or read), and then to reread the	*/
    /* file.  If the size is inconsistent we have to retry.	*/
    /* This only matters with threads enabled, and if we use 	*/
    /* this to locate roots (not the default).			*/

    /* Determine the initial size of /proc/self/maps.		*/
    /* Note that lseek doesn't work, at least as of 2.6.15.	*/
#   ifdef THREADS
	maps_size = GC_get_maps_len();
	if (0 == maps_size) return 0;
#   else
	maps_size = 4000;	/* Guess */
#   endif

    /* Read /proc/self/maps, growing maps_buf as necessary.	*/
    /* Note that we may not allocate conventionally, and	*/
    /* thus can't use stdio.					*/
	do {
	    while (maps_size >= maps_buf_sz) {
	      /* Grow only by powers of 2, since we leak "too small" buffers. */
	      while (maps_size >= maps_buf_sz) maps_buf_sz *= 2;
	      maps_buf = GC_scratch_alloc(maps_buf_sz);
#	      ifdef THREADS
	        /* Recompute initial length, since we allocated.	*/
	        /* This can only happen a few times per program		*/
	        /* execution.						*/
	        maps_size = GC_get_maps_len();
	        if (0 == maps_size) return 0;
#	      endif
	      if (maps_buf == 0) return 0;
	    }
	    GC_ASSERT(maps_buf_sz >= maps_size + 1);
	    f = open("/proc/self/maps", O_RDONLY);
	    if (-1 == f) return 0;
#	    ifdef THREADS
	      old_maps_size = maps_size;
#	    endif
	    maps_size = 0;
	    do {
	        result = GC_repeat_read(f, maps_buf, maps_buf_sz-1);
	        if (result <= 0) return 0;
	        maps_size += result;
	    } while (result == maps_buf_sz-1);
	    close(f);
#	    ifdef THREADS
	      if (maps_size > old_maps_size) {
		GC_err_printf("Old maps size = %d, new maps size = %d\n",
			      old_maps_size, maps_size);
		ABORT("Unexpected asynchronous /proc/self/maps growth: "
		      "Unregistered thread?");
	      }
#	    endif
	} while (maps_size >= maps_buf_sz || maps_size < old_maps_size);
		/* In the single-threaded case, the second clause is false.	*/
        maps_buf[maps_size] = '\0';
	
    /* Apply fn to result. */
	return maps_buf;
}

//
//  GC_parse_map_entry parses an entry from /proc/self/maps so we can
//  locate all writable data segments that belong to shared libraries.
//  The format of one of these entries and the fields we care about
//  is as follows:
//  XXXXXXXX-XXXXXXXX r-xp 00000000 30:05 260537     name of mapping...\n
//  ^^^^^^^^ ^^^^^^^^ ^^^^          ^^
//  start    end      prot          maj_dev
//
//  Note that since about august 2003 kernels, the columns no longer have
//  fixed offsets on 64-bit kernels.  Hence we no longer rely on fixed offsets
//  anywhere, which is safer anyway.
//

/*
 * Assign various fields of the first line in buf_ptr to *start, *end,
 * *prot, *maj_dev and *mapping_name.  Mapping_name may be NULL.
 * *prot and *mapping_name are assigned pointers into the original
 * buffer.
 */
char *GC_parse_map_entry(char *buf_ptr, ptr_t *start, ptr_t *end,
                                char **prot, unsigned int *maj_dev,
				char **mapping_name)
{
    char *start_start, *end_start, *maj_dev_start;
    char *p;
    char *endp;

    if (buf_ptr == NULL || *buf_ptr == '\0') {
        return NULL;
    }

    p = buf_ptr;
    while (isspace(*p)) ++p;
    start_start = p;
    GC_ASSERT(isxdigit(*start_start));
    *start = (ptr_t)strtoul(start_start, &endp, 16); p = endp;
    GC_ASSERT(*p=='-');

    ++p;
    end_start = p;
    GC_ASSERT(isxdigit(*end_start));
    *end = (ptr_t)strtoul(end_start, &endp, 16); p = endp;
    GC_ASSERT(isspace(*p));

    while (isspace(*p)) ++p;
    GC_ASSERT(*p == 'r' || *p == '-');
    *prot = p;
    /* Skip past protection field to offset field */
       while (!isspace(*p)) ++p; while (isspace(*p)) ++p;
    GC_ASSERT(isxdigit(*p));
    /* Skip past offset field, which we ignore */
          while (!isspace(*p)) ++p; while (isspace(*p)) ++p;
    maj_dev_start = p;
    GC_ASSERT(isxdigit(*maj_dev_start));
    *maj_dev = strtoul(maj_dev_start, NULL, 16);

    if (mapping_name == 0) {
      while (*p && *p++ != '\n');
    } else {
      while (*p && *p != '\n' && *p != '/' && *p != '[') p++;
      *mapping_name = p;
      while (*p && *p++ != '\n');
    }

    return p;
}

/* Try to read the backing store base from /proc/self/maps.		*/
/* Return the bounds of the writable mapping with a 0 major device,	*/
/* which includes the address passed as data.				*/
/* Return FALSE if there is no such mapping.				*/
GC_bool GC_enclosing_mapping(ptr_t addr, ptr_t *startp, ptr_t *endp)
{
  char *prot;
  ptr_t my_start, my_end;
  unsigned int maj_dev;
  char *maps = GC_get_maps();
  char *buf_ptr = maps;
  
  if (0 == maps) return(FALSE);
  for (;;) {
    buf_ptr = GC_parse_map_entry(buf_ptr, &my_start, &my_end,
		    	         &prot, &maj_dev, 0);

    if (buf_ptr == NULL) return FALSE;
    if (prot[1] == 'w' && maj_dev == 0) {
        if (my_end > addr && my_start <= addr) {
    	  *startp = my_start;
	  *endp = my_end;
	  return TRUE;
	}
    }
  }
  return FALSE;
}

/* Find the text(code) mapping for the library whose name starts with nm. */
GC_bool GC_text_mapping(char *nm, ptr_t *startp, ptr_t *endp)
{
  size_t nm_len = strlen(nm);
  char *prot;
  char *map_path;
  ptr_t my_start, my_end;
  unsigned int maj_dev;
  char *maps = GC_get_maps();
  char *buf_ptr = maps;
  
  if (0 == maps) return(FALSE);
  for (;;) {
    buf_ptr = GC_parse_map_entry(buf_ptr, &my_start, &my_end,
		    	         &prot, &maj_dev, &map_path);

    if (buf_ptr == NULL) return FALSE;
    if (prot[0] == 'r' && prot[1] == '-' && prot[2] == 'x' &&
	strncmp(nm, map_path, nm_len) == 0) {
    	  *startp = my_start;
	  *endp = my_end;
	  return TRUE;
    }
  }
  return FALSE;
}

#ifdef IA64
static ptr_t backing_store_base_from_proc(void)
{
    ptr_t my_start, my_end;
    if (!GC_enclosing_mapping(GC_save_regs_in_stack(), &my_start, &my_end)) {
	if (GC_print_stats) {
	    GC_log_printf("Failed to find backing store base from /proc\n");
	}
	return 0;
    }
    return my_start;
}
#endif

#endif /* NEED_PROC_MAPS */	

#if defined(SEARCH_FOR_DATA_START)
  /* The I386 case can be handled without a search.  The Alpha case	*/
  /* used to be handled differently as well, but the rules changed	*/
  /* for recent Linux versions.  This seems to be the easiest way to	*/
  /* cover all versions.						*/

# if defined(LINUX) || defined(HURD)
    /* Some Linux distributions arrange to define __data_start.  Some	*/
    /* define data_start as a weak symbol.  The latter is technically	*/
    /* broken, since the user program may define data_start, in which	*/
    /* case we lose.  Nonetheless, we try both, prefering __data_start.	*/
    /* We assume gcc-compatible pragmas.	*/
#   pragma weak __data_start
    extern int __data_start[];
#   pragma weak data_start
    extern int data_start[];
# endif /* LINUX */
  extern int _end[];

  ptr_t GC_data_start;

  void GC_init_linux_data_start()
  {
    extern ptr_t GC_find_limit(ptr_t, GC_bool);

#   if defined(LINUX) || defined(HURD)
      /* Try the easy approaches first:	*/
      if ((ptr_t)__data_start != 0) {
	  GC_data_start = (ptr_t)(__data_start);
	  return;
      }
      if ((ptr_t)data_start != 0) {
	  GC_data_start = (ptr_t)(data_start);
	  return;
      }
#   endif /* LINUX */
    GC_data_start = GC_find_limit((ptr_t)(_end), FALSE);
  }
#endif

# ifdef ECOS

# ifndef ECOS_GC_MEMORY_SIZE
# define ECOS_GC_MEMORY_SIZE (448 * 1024)
# endif /* ECOS_GC_MEMORY_SIZE */

// FIXME: This is a simple way of allocating memory which is
// compatible with ECOS early releases.  Later releases use a more
// sophisticated means of allocating memory than this simple static
// allocator, but this method is at least bound to work.
static char memory[ECOS_GC_MEMORY_SIZE];
static char *brk = memory;

static void *tiny_sbrk(ptrdiff_t increment)
{
  void *p = brk;

  brk += increment;

  if (brk >  memory + sizeof memory)
    {
      brk -= increment;
      return NULL;
    }

  return p;
}
#define sbrk tiny_sbrk
# endif /* ECOS */

#if (defined(NETBSD) || defined(OPENBSD)) && defined(__ELF__)
  ptr_t GC_data_start;

  void GC_init_netbsd_elf(void)
  {
    extern ptr_t GC_find_limit(ptr_t, GC_bool);
    extern char **environ;
	/* This may need to be environ, without the underscore, for	*/
	/* some versions.						*/
    GC_data_start = GC_find_limit((ptr_t)&environ, FALSE);
  }
#endif

# ifdef OS2

# include <stddef.h>

# if !defined(__IBMC__) && !defined(__WATCOMC__) /* e.g. EMX */

struct exe_hdr {
    unsigned short      magic_number;
    unsigned short      padding[29];
    long                new_exe_offset;
};

#define E_MAGIC(x)      (x).magic_number
#define EMAGIC          0x5A4D  
#define E_LFANEW(x)     (x).new_exe_offset

struct e32_exe {
    unsigned char       magic_number[2]; 
    unsigned char       byte_order; 
    unsigned char       word_order; 
    unsigned long       exe_format_level;
    unsigned short      cpu;       
    unsigned short      os;
    unsigned long       padding1[13];
    unsigned long       object_table_offset;
    unsigned long       object_count;    
    unsigned long       padding2[31];
};

#define E32_MAGIC1(x)   (x).magic_number[0]
#define E32MAGIC1       'L'
#define E32_MAGIC2(x)   (x).magic_number[1]
#define E32MAGIC2       'X'
#define E32_BORDER(x)   (x).byte_order
#define E32LEBO         0
#define E32_WORDER(x)   (x).word_order
#define E32LEWO         0
#define E32_CPU(x)      (x).cpu
#define E32CPU286       1
#define E32_OBJTAB(x)   (x).object_table_offset
#define E32_OBJCNT(x)   (x).object_count

struct o32_obj {
    unsigned long       size;  
    unsigned long       base;
    unsigned long       flags;  
    unsigned long       pagemap;
    unsigned long       mapsize; 
    unsigned long       reserved;
};

#define O32_FLAGS(x)    (x).flags
#define OBJREAD         0x0001L
#define OBJWRITE        0x0002L
#define OBJINVALID      0x0080L
#define O32_SIZE(x)     (x).size
#define O32_BASE(x)     (x).base

# else  /* IBM's compiler */

/* A kludge to get around what appears to be a header file bug */
# ifndef WORD
#   define WORD unsigned short
# endif
# ifndef DWORD
#   define DWORD unsigned long
# endif

# define EXE386 1
# include <newexe.h>
# include <exe386.h>

# endif  /* __IBMC__ */

# define INCL_DOSEXCEPTIONS
# define INCL_DOSPROCESS
# define INCL_DOSERRORS
# define INCL_DOSMODULEMGR
# define INCL_DOSMEMMGR
# include <os2.h>


/* Disable and enable signals during nontrivial allocations	*/

void GC_disable_signals(void)
{
    ULONG nest;
    
    DosEnterMustComplete(&nest);
    if (nest != 1) ABORT("nested GC_disable_signals");
}

void GC_enable_signals(void)
{
    ULONG nest;
    
    DosExitMustComplete(&nest);
    if (nest != 0) ABORT("GC_enable_signals");
}


# else

#  if !defined(PCR) && !defined(AMIGA) && !defined(MSWIN32) \
      && !defined(MSWINCE) \
      && !defined(MACOS) && !defined(DJGPP) && !defined(DOS4GW) \
      && !defined(NOSYS) && !defined(ECOS)

#   if 0
	/* Use the traditional BSD interface */
#	define SIGSET_T int
#	define SIG_DEL(set, signal) (set) &= ~(sigmask(signal))
#	define SIG_FILL(set)  (set) = 0x7fffffff
    	  /* Setting the leading bit appears to provoke a bug in some	*/
    	  /* longjmp implementations.  Most systems appear not to have	*/
    	  /* a signal 32.						*/
#	define SIGSETMASK(old, new) (old) = sigsetmask(new)
#   endif

    /* Use POSIX/SYSV interface	*/
#   define SIGSET_T sigset_t
#   define SIG_DEL(set, signal) sigdelset(&(set), (signal))
#   define SIG_FILL(set) sigfillset(&set)
#   define SIGSETMASK(old, new) sigprocmask(SIG_SETMASK, &(new), &(old))


static GC_bool mask_initialized = FALSE;

static SIGSET_T new_mask;

static SIGSET_T old_mask;

static SIGSET_T dummy;

#if defined(GC_ASSERTIONS) && !defined(THREADS)
# define CHECK_SIGNALS
  int GC_sig_disabled = 0;
#endif

void GC_disable_signals(void)
{
    if (!mask_initialized) {
    	SIG_FILL(new_mask);

	SIG_DEL(new_mask, SIGSEGV);
	SIG_DEL(new_mask, SIGILL);
	SIG_DEL(new_mask, SIGQUIT);
#	ifdef SIGBUS
	    SIG_DEL(new_mask, SIGBUS);
#	endif
#	ifdef SIGIOT
	    SIG_DEL(new_mask, SIGIOT);
#	endif
#	ifdef SIGEMT
	    SIG_DEL(new_mask, SIGEMT);
#	endif
#	ifdef SIGTRAP
	    SIG_DEL(new_mask, SIGTRAP);
#	endif 
	mask_initialized = TRUE;
    }
#   ifdef CHECK_SIGNALS
	if (GC_sig_disabled != 0) ABORT("Nested disables");
	GC_sig_disabled++;
#   endif
    SIGSETMASK(old_mask,new_mask);
}

void GC_enable_signals(void)
{
#   ifdef CHECK_SIGNALS
	if (GC_sig_disabled != 1) ABORT("Unmatched enable");
	GC_sig_disabled--;
#   endif
    SIGSETMASK(dummy,old_mask);
}

#  endif  /* !PCR */

# endif /*!OS/2 */

/* Ivan Demakov: simplest way (to me) */
#if defined (DOS4GW)
  void GC_disable_signals() { }
  void GC_enable_signals() { }
#endif

/* Find the page size */
word GC_page_size;

# if defined(MSWIN32) || defined(MSWINCE)
  void GC_setpagesize(void)
  {
    GetSystemInfo(&GC_sysinfo);
    GC_page_size = GC_sysinfo.dwPageSize;
  }

# else
#   if defined(MPROTECT_VDB) || defined(PROC_VDB) || defined(USE_MMAP)
	void GC_setpagesize(void)
	{
	    GC_page_size = GETPAGESIZE();
	}
#   else
	/* It's acceptable to fake it. */
	void GC_setpagesize(void)
	{
	    GC_page_size = HBLKSIZE;
	}
#   endif
# endif

/* 
 * Find the base of the stack. 
 * Used only in single-threaded environment.
 * With threads, GC_mark_roots needs to know how to do this.
 * Called with allocator lock held.
 */
# if defined(MSWIN32) || defined(MSWINCE) || defined(CYGWIN32)
# define is_writable(prot) ((prot) == PAGE_READWRITE \
			    || (prot) == PAGE_WRITECOPY \
			    || (prot) == PAGE_EXECUTE_READWRITE \
			    || (prot) == PAGE_EXECUTE_WRITECOPY)
/* Return the number of bytes that are writable starting at p.	*/
/* The pointer p is assumed to be page aligned.			*/
/* If base is not 0, *base becomes the beginning of the 	*/
/* allocation region containing p.				*/
word GC_get_writable_length(ptr_t p, ptr_t *base)
{
    MEMORY_BASIC_INFORMATION buf;
    word result;
    word protect;
    
    result = VirtualQuery(p, &buf, sizeof(buf));
    if (result != sizeof(buf)) ABORT("Weird VirtualQuery result");
    if (base != 0) *base = (ptr_t)(buf.AllocationBase);
    protect = (buf.Protect & ~(PAGE_GUARD | PAGE_NOCACHE));
    if (!is_writable(protect)) {
        return(0);
    }
    if (buf.State != MEM_COMMIT) return(0);
    return(buf.RegionSize);
}

int GC_get_stack_base(struct GC_stack_base *sb)
{
    int dummy;
    ptr_t sp = (ptr_t)(&dummy);
    ptr_t trunc_sp = (ptr_t)((word)sp & ~(GC_page_size - 1));
    word size = GC_get_writable_length(trunc_sp, 0);
   
    sb -> mem_base = trunc_sp + size;
    return GC_SUCCESS;
}

#define HAVE_GET_STACK_BASE

/* This is always called from the main thread.	*/
ptr_t GC_get_main_stack_base(void)
{
    struct GC_stack_base sb;

    GC_get_stack_base(&sb);
    return (ptr_t)sb.mem_base;
}

# endif /* MS Windows */

# ifdef BEOS
# include <kernel/OS.h>
ptr_t GC_get_main_stack_base(void){
	thread_info th;
	get_thread_info(find_thread(NULL),&th);
	return th.stack_end;
}
# endif /* BEOS */


# ifdef OS2

ptr_t GC_get_main_stack_base(void)
{
    PTIB ptib;
    PPIB ppib;
    
    if (DosGetInfoBlocks(&ptib, &ppib) != NO_ERROR) {
    	GC_err_printf("DosGetInfoBlocks failed\n");
    	ABORT("DosGetInfoBlocks failed\n");
    }
    return((ptr_t)(ptib -> tib_pstacklimit));
}

# endif /* OS2 */

# ifdef AMIGA
#   define GC_AMIGA_SB
#   include "AmigaOS.c"
#   undef GC_AMIGA_SB
# endif /* AMIGA */

# if defined(NEED_FIND_LIMIT) || defined(UNIX_LIKE)

    typedef void (*handler)(int);

#   if defined(SUNOS5SIGS) || defined(IRIX5) || defined(OSF1) \
    || defined(HURD) || defined(NETBSD)
	static struct sigaction old_segv_act;
#	if defined(_sigargs) /* !Irix6.x */ || defined(HPUX) \
	|| defined(HURD) || defined(NETBSD)
	    static struct sigaction old_bus_act;
#	endif
#   else
        static handler old_segv_handler, old_bus_handler;
#   endif
    
    void GC_set_and_save_fault_handler(handler h)
    {
#	if defined(SUNOS5SIGS) || defined(IRIX5)  \
        || defined(OSF1) || defined(HURD) || defined(NETBSD)
	  struct sigaction	act;

	  act.sa_handler	= h;
#	  if 0 /* Was necessary for Solaris 2.3 and very temporary 	*/
	       /* NetBSD bugs.						*/
            act.sa_flags          = SA_RESTART | SA_NODEFER;
#         else
            act.sa_flags          = SA_RESTART;
#	  endif

	  (void) sigemptyset(&act.sa_mask);
#	  ifdef GC_IRIX_THREADS
		/* Older versions have a bug related to retrieving and	*/
		/* and setting a handler at the same time.		*/
	        (void) sigaction(SIGSEGV, 0, &old_segv_act);
	        (void) sigaction(SIGSEGV, &act, 0);
#	  else
	        (void) sigaction(SIGSEGV, &act, &old_segv_act);
#		if defined(IRIX5) && defined(_sigargs) /* Irix 5.x, not 6.x */ \
		   || defined(HPUX) || defined(HURD) || defined(NETBSD)
		    /* Under Irix 5.x or HP/UX, we may get SIGBUS.	*/
		    /* Pthreads doesn't exist under Irix 5.x, so we	*/
		    /* don't have to worry in the threads case.		*/
		    (void) sigaction(SIGBUS, &act, &old_bus_act);
#		endif
#	  endif	/* GC_IRIX_THREADS */
#	else
    	  old_segv_handler = signal(SIGSEGV, h);
#	  ifdef SIGBUS
	    old_bus_handler = signal(SIGBUS, h);
#	  endif
#	endif
    }
# endif /* NEED_FIND_LIMIT || UNIX_LIKE */

# if defined(NEED_FIND_LIMIT) || \
     defined(USE_PROC_FOR_LIBRARIES) && defined(THREADS)
  /* Some tools to implement HEURISTIC2	*/
#   define MIN_PAGE_SIZE 256	/* Smallest conceivable page size, bytes */
    
    /*ARGSUSED*/
    void GC_fault_handler(int sig)
    {
        LONGJMP(GC_jmp_buf, 1);
    }

    void GC_setup_temporary_fault_handler(void)
    {
	/* Handler is process-wide, so this should only happen in */
	/* one thread at a time.				  */
	GC_ASSERT(I_HOLD_LOCK());
	GC_set_and_save_fault_handler(GC_fault_handler);
    }
    
    void GC_reset_fault_handler(void)
    {
#       if defined(SUNOS5SIGS) || defined(IRIX5) \
	   || defined(OSF1) || defined(HURD) || defined(NETBSD)
	  (void) sigaction(SIGSEGV, &old_segv_act, 0);
#	  if defined(IRIX5) && defined(_sigargs) /* Irix 5.x, not 6.x */ \
	     || defined(HPUX) || defined(HURD) || defined(NETBSD)
	      (void) sigaction(SIGBUS, &old_bus_act, 0);
#	  endif
#       else
  	  (void) signal(SIGSEGV, old_segv_handler);
#	  ifdef SIGBUS
	    (void) signal(SIGBUS, old_bus_handler);
#	  endif
#       endif
    }

    /* Return the first nonaddressible location > p (up) or 	*/
    /* the smallest location q s.t. [q,p) is addressable (!up).	*/
    /* We assume that p (up) or p-1 (!up) is addressable.	*/
    /* Requires allocation lock.				*/
    ptr_t GC_find_limit_with_bound(ptr_t p, GC_bool up, ptr_t bound)
    {
        static volatile ptr_t result;
    		/* Safer if static, since otherwise it may not be	*/
    		/* preserved across the longjmp.  Can safely be 	*/
    		/* static since it's only called with the		*/
    		/* allocation lock held.				*/

	GC_ASSERT(I_HOLD_LOCK());
	GC_setup_temporary_fault_handler();
	if (SETJMP(GC_jmp_buf) == 0) {
	    result = (ptr_t)(((word)(p))
			      & ~(MIN_PAGE_SIZE-1));
	    for (;;) {
 	        if (up) {
		    result += MIN_PAGE_SIZE;
		    if (result >= bound) return bound;
 	        } else {
		    result -= MIN_PAGE_SIZE;
		    if (result <= bound) return bound;
 	        }
		GC_noop1((word)(*result));
	    }
	}
	GC_reset_fault_handler();
 	if (!up) {
	    result += MIN_PAGE_SIZE;
 	}
	return(result);
    }

    ptr_t GC_find_limit(ptr_t p, GC_bool up)
    {
      if (up) {
	return GC_find_limit_with_bound(p, up, (ptr_t)(word)(-1));
      } else {
	return GC_find_limit_with_bound(p, up, 0);
      }
    }
# endif

#if defined(ECOS) || defined(NOSYS)
  ptr_t GC_get_main_stack_base(void)
  {
    return STACKBOTTOM;
  }
#endif

#ifdef HPUX_STACKBOTTOM

#include <sys/param.h>
#include <sys/pstat.h>

  ptr_t GC_get_register_stack_base(void)
  {
    struct pst_vm_status vm_status;

    int i = 0;
    while (pstat_getprocvm(&vm_status, sizeof(vm_status), 0, i++) == 1) {
      if (vm_status.pst_type == PS_RSESTACK) {
        return (ptr_t) vm_status.pst_vaddr;
      }
    }

    /* old way to get the register stackbottom */
    return (ptr_t)(((word)GC_stackbottom - BACKING_STORE_DISPLACEMENT - 1)
                   & ~(BACKING_STORE_ALIGNMENT - 1));
  }

#endif /* HPUX_STACK_BOTTOM */

#ifdef LINUX_STACKBOTTOM

#include <sys/types.h>
#include <sys/stat.h>

# define STAT_SKIP 27   /* Number of fields preceding startstack	*/
			/* field in /proc/self/stat			*/

#ifdef USE_LIBC_PRIVATES
# pragma weak __libc_stack_end
  extern ptr_t __libc_stack_end;
#endif

# ifdef IA64
#   ifdef USE_LIBC_PRIVATES
#     pragma weak __libc_ia64_register_backing_store_base
      extern ptr_t __libc_ia64_register_backing_store_base;
#   endif

    ptr_t GC_get_register_stack_base(void)
    {
      ptr_t result;

#     ifdef USE_LIBC_PRIVATES
        if (0 != &__libc_ia64_register_backing_store_base
	    && 0 != __libc_ia64_register_backing_store_base) {
	  /* Glibc 2.2.4 has a bug such that for dynamically linked	*/
	  /* executables __libc_ia64_register_backing_store_base is 	*/
	  /* defined but uninitialized during constructor calls.  	*/
	  /* Hence we check for both nonzero address and value.		*/
	  return __libc_ia64_register_backing_store_base;
        }
#     endif
      result = backing_store_base_from_proc();
      if (0 == result) {
	  result = GC_find_limit(GC_save_regs_in_stack(), FALSE);
	  /* Now seems to work better than constant displacement	*/
	  /* heuristic used in 6.X versions.  The latter seems to	*/
	  /* fail for 2.6 kernels.					*/
      }
      return result;
    }
# endif

  ptr_t GC_linux_stack_base(void)
  {
    /* We read the stack base value from /proc/self/stat.  We do this	*/
    /* using direct I/O system calls in order to avoid calling malloc   */
    /* in case REDIRECT_MALLOC is defined.				*/ 
#   define STAT_BUF_SIZE 4096
#   define STAT_READ read
	  /* Should probably call the real read, if read is wrapped.	*/
    char stat_buf[STAT_BUF_SIZE];
    int f;
    char c;
    word result = 0;
    size_t i, buf_offset = 0;

    /* First try the easy way.  This should work for glibc 2.2	*/
    /* This fails in a prelinked ("prelink" command) executable */
    /* since the correct value of __libc_stack_end never	*/
    /* becomes visible to us.  The second test works around 	*/
    /* this.							*/  
#   ifdef USE_LIBC_PRIVATES
      if (0 != &__libc_stack_end && 0 != __libc_stack_end ) {
#       if defined(IA64)
	  /* Some versions of glibc set the address 16 bytes too	*/
	  /* low while the initialization code is running.		*/
	  if (((word)__libc_stack_end & 0xfff) + 0x10 < 0x1000) {
	    return __libc_stack_end + 0x10;
	  } /* Otherwise it's not safe to add 16 bytes and we fall	*/
	    /* back to using /proc.					*/
#	elif defined(SPARC)
	  /* Older versions of glibc for 64-bit Sparc do not set
	   * this variable correctly, it gets set to either zero
	   * or one.
	   */
	  if (__libc_stack_end != (ptr_t) (unsigned long)0x1)
	    return __libc_stack_end;
#	else
	  return __libc_stack_end;
#	endif
      }
#   endif
    f = open("/proc/self/stat", O_RDONLY);
    if (f < 0 || STAT_READ(f, stat_buf, STAT_BUF_SIZE) < 2 * STAT_SKIP) {
	ABORT("Couldn't read /proc/self/stat");
    }
    c = stat_buf[buf_offset++];
    /* Skip the required number of fields.  This number is hopefully	*/
    /* constant across all Linux implementations.			*/
      for (i = 0; i < STAT_SKIP; ++i) {
	while (isspace(c)) c = stat_buf[buf_offset++];
	while (!isspace(c)) c = stat_buf[buf_offset++];
      }
    while (isspace(c)) c = stat_buf[buf_offset++];
    while (isdigit(c)) {
      result *= 10;
      result += c - '0';
      c = stat_buf[buf_offset++];
    }
    close(f);
    if (result < 0x10000000) ABORT("Absurd stack bottom value");
    return (ptr_t)result;
  }

#endif /* LINUX_STACKBOTTOM */

#ifdef FREEBSD_STACKBOTTOM

/* This uses an undocumented sysctl call, but at least one expert 	*/
/* believes it will stay.						*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

  ptr_t GC_freebsd_stack_base(void)
  {
    int nm[2] = {CTL_KERN, KERN_USRSTACK};
    ptr_t base;
    size_t len = sizeof(ptr_t);
    int r = sysctl(nm, 2, &base, &len, NULL, 0);
    
    if (r) ABORT("Error getting stack base");

    return base;
  }

#endif /* FREEBSD_STACKBOTTOM */

#if !defined(BEOS) && !defined(AMIGA) && !defined(MSWIN32) \
    && !defined(MSWINCE) && !defined(OS2) && !defined(NOSYS) && !defined(ECOS) \
    && !defined(CYGWIN32)

ptr_t GC_get_main_stack_base(void)
{
#   if defined(HEURISTIC1) || defined(HEURISTIC2)
      word dummy;
#   endif
    ptr_t result;

#   define STACKBOTTOM_ALIGNMENT_M1 ((word)STACK_GRAN - 1)

#   ifdef STACKBOTTOM
	return(STACKBOTTOM);
#   else
#	ifdef HEURISTIC1
#	   ifdef STACK_GROWS_DOWN
	     result = (ptr_t)((((word)(&dummy))
	     		       + STACKBOTTOM_ALIGNMENT_M1)
			      & ~STACKBOTTOM_ALIGNMENT_M1);
#	   else
	     result = (ptr_t)(((word)(&dummy))
			      & ~STACKBOTTOM_ALIGNMENT_M1);
#	   endif
#	endif /* HEURISTIC1 */
#	ifdef LINUX_STACKBOTTOM
	   result = GC_linux_stack_base();
#	endif
#	ifdef FREEBSD_STACKBOTTOM
	   result = GC_freebsd_stack_base();
#	endif
#	ifdef HEURISTIC2
#	    ifdef STACK_GROWS_DOWN
		result = GC_find_limit((ptr_t)(&dummy), TRUE);
#           	ifdef HEURISTIC2_LIMIT
		    if (result > HEURISTIC2_LIMIT
		        && (ptr_t)(&dummy) < HEURISTIC2_LIMIT) {
		            result = HEURISTIC2_LIMIT;
		    }
#	        endif
#	    else
		result = GC_find_limit((ptr_t)(&dummy), FALSE);
#           	ifdef HEURISTIC2_LIMIT
		    if (result < HEURISTIC2_LIMIT
		        && (ptr_t)(&dummy) > HEURISTIC2_LIMIT) {
		            result = HEURISTIC2_LIMIT;
		    }
#	        endif
#	    endif

#	endif /* HEURISTIC2 */
#	ifdef STACK_GROWS_DOWN
	    if (result == 0) result = (ptr_t)(signed_word)(-sizeof(ptr_t));
#	endif
    	return(result);
#   endif /* STACKBOTTOM */
}

# endif /* ! AMIGA, !OS 2, ! MS Windows, !BEOS, !NOSYS, !ECOS */

#if defined(GC_LINUX_THREADS) && !defined(HAVE_GET_STACK_BASE)

#include <pthread.h>

#ifdef IA64
  ptr_t GC_greatest_stack_base_below(ptr_t bound);
  	/* From pthread_support.c */
#endif

int GC_get_stack_base(struct GC_stack_base *b)
{
    pthread_attr_t attr;
    size_t size;

    if (pthread_getattr_np(pthread_self(), &attr) != 0) {
	WARN("pthread_getattr_np failed\n", 0);
	return GC_UNIMPLEMENTED;
    }
    if (pthread_attr_getstack(&attr, &(b -> mem_base), &size) != 0) {
	ABORT("pthread_attr_getstack failed");
    }
#   ifdef STACK_GROWS_DOWN
        b -> mem_base = (char *)(b -> mem_base) + size;
#   endif
#   ifdef IA64
      /* We could try backing_store_base_from_proc, but that's safe	*/
      /* only if no mappings are being asynchronously created.		*/
      /* Subtracting the size from the stack base doesn't work for at	*/
      /* least the main thread.						*/
      LOCK();
      {
	ptr_t bsp = GC_save_regs_in_stack();
	ptr_t next_stack = GC_greatest_stack_base_below(bsp);
	if (0 == next_stack) {
          b -> reg_base = GC_find_limit(bsp, FALSE);
	} else {
	  /* Avoid walking backwards into preceding memory stack and	*/
	  /* growing it.						*/
          b -> reg_base = GC_find_limit_with_bound(bsp, FALSE, next_stack);
	}
      }
      UNLOCK();
#   endif
    return GC_SUCCESS;
}

#define HAVE_GET_STACK_BASE

#endif /* GC_LINUX_THREADS */

#ifndef HAVE_GET_STACK_BASE
/* Retrieve stack base.						*/
/* Using the GC_find_limit version is risky.			*/
/* On IA64, for example, there is no guard page between the	*/
/* stack of one thread and the register backing store of the 	*/
/* next.  Thus this is likely to identify way too large a	*/
/* "stack" and thus at least result in disastrous performance.	*/
/* FIXME - Implement better strategies here.			*/
int GC_get_stack_base(struct GC_stack_base *b)
{
    int dummy;

#   ifdef NEED_FIND_LIMIT
#     ifdef STACK_GROWS_DOWN
    	b -> mem_base = GC_find_limit((ptr_t)(&dummy), TRUE);
#       ifdef IA64
	  b -> reg_base = GC_find_limit(GC_save_regs_in_stack(), FALSE);
#       endif
#     else
	b -> mem_base = GC_find_limit(&dummy, FALSE);
#     endif
      return GC_SUCCESS;
#   else
      return GC_UNIMPLEMENTED;
#   endif
}
#endif

/*
 * Register static data segment(s) as roots.
 * If more data segments are added later then they need to be registered
 * add that point (as we do with SunOS dynamic loading),
 * or GC_mark_roots needs to check for them (as we do with PCR).
 * Called with allocator lock held.
 */

# ifdef OS2

void GC_register_data_segments(void)
{
    PTIB ptib;
    PPIB ppib;
    HMODULE module_handle;
#   define PBUFSIZ 512
    UCHAR path[PBUFSIZ];
    FILE * myexefile;
    struct exe_hdr hdrdos;	/* MSDOS header.	*/
    struct e32_exe hdr386;	/* Real header for my executable */
    struct o32_obj seg;	/* Currrent segment */
    int nsegs;
    
    
    if (DosGetInfoBlocks(&ptib, &ppib) != NO_ERROR) {
    	GC_err_printf("DosGetInfoBlocks failed\n");
    	ABORT("DosGetInfoBlocks failed\n");
    }
    module_handle = ppib -> pib_hmte;
    if (DosQueryModuleName(module_handle, PBUFSIZ, path) != NO_ERROR) {
    	GC_err_printf("DosQueryModuleName failed\n");
    	ABORT("DosGetInfoBlocks failed\n");
    }
    myexefile = fopen(path, "rb");
    if (myexefile == 0) {
        GC_err_puts("Couldn't open executable ");
        GC_err_puts(path); GC_err_puts("\n");
        ABORT("Failed to open executable\n");
    }
    if (fread((char *)(&hdrdos), 1, sizeof hdrdos, myexefile) < sizeof hdrdos) {
        GC_err_puts("Couldn't read MSDOS header from ");
        GC_err_puts(path); GC_err_puts("\n");
        ABORT("Couldn't read MSDOS header");
    }
    if (E_MAGIC(hdrdos) != EMAGIC) {
        GC_err_puts("Executable has wrong DOS magic number: ");
        GC_err_puts(path); GC_err_puts("\n");
        ABORT("Bad DOS magic number");
    }
    if (fseek(myexefile, E_LFANEW(hdrdos), SEEK_SET) != 0) {
        GC_err_puts("Seek to new header failed in ");
        GC_err_puts(path); GC_err_puts("\n");
        ABORT("Bad DOS magic number");
    }
    if (fread((char *)(&hdr386), 1, sizeof hdr386, myexefile) < sizeof hdr386) {
        GC_err_puts("Couldn't read MSDOS header from ");
        GC_err_puts(path); GC_err_puts("\n");
        ABORT("Couldn't read OS/2 header");
    }
    if (E32_MAGIC1(hdr386) != E32MAGIC1 || E32_MAGIC2(hdr386) != E32MAGIC2) {
        GC_err_puts("Executable has wrong OS/2 magic number:");
        GC_err_puts(path); GC_err_puts("\n");
        ABORT("Bad OS/2 magic number");
    }
    if ( E32_BORDER(hdr386) != E32LEBO || E32_WORDER(hdr386) != E32LEWO) {
        GC_err_puts("Executable %s has wrong byte order: ");
        GC_err_puts(path); GC_err_puts("\n");
        ABORT("Bad byte order");
    }
    if ( E32_CPU(hdr386) == E32CPU286) {
        GC_err_puts("GC can't handle 80286 executables: ");
        GC_err_puts(path); GC_err_puts("\n");
        EXIT();
    }
    if (fseek(myexefile, E_LFANEW(hdrdos) + E32_OBJTAB(hdr386),
    	      SEEK_SET) != 0) {
        GC_err_puts("Seek to object table failed: ");
        GC_err_puts(path); GC_err_puts("\n");
        ABORT("Seek to object table failed");
    }
    for (nsegs = E32_OBJCNT(hdr386); nsegs > 0; nsegs--) {
      int flags;
      if (fread((char *)(&seg), 1, sizeof seg, myexefile) < sizeof seg) {
        GC_err_puts("Couldn't read obj table entry from ");
        GC_err_puts(path); GC_err_puts("\n");
        ABORT("Couldn't read obj table entry");
      }
      flags = O32_FLAGS(seg);
      if (!(flags & OBJWRITE)) continue;
      if (!(flags & OBJREAD)) continue;
      if (flags & OBJINVALID) {
          GC_err_printf("Object with invalid pages?\n");
          continue;
      } 
      GC_add_roots_inner(O32_BASE(seg), O32_BASE(seg)+O32_SIZE(seg), FALSE);
    }
}

# else /* !OS2 */

# if defined(MSWIN32) || defined(MSWINCE)

# ifdef MSWIN32
  /* Unfortunately, we have to handle win32s very differently from NT, 	*/
  /* Since VirtualQuery has very different semantics.  In particular,	*/
  /* under win32s a VirtualQuery call on an unmapped page returns an	*/
  /* invalid result.  Under NT, GC_register_data_segments is a noop and	*/
  /* all real work is done by GC_register_dynamic_libraries.  Under	*/
  /* win32s, we cannot find the data segments associated with dll's.	*/
  /* We register the main data segment here.				*/
  GC_bool GC_no_win32_dlls = FALSE;	 
  	/* This used to be set for gcc, to avoid dealing with		*/
  	/* the structured exception handling issues.  But we now have	*/
  	/* assembly code to do that right.				*/

# if defined(GWW_VDB)

#   ifndef _BASETSD_H_
      typedef ULONG * PULONG_PTR;
#   endif
    typedef UINT (WINAPI * GetWriteWatch_type)(
      DWORD, PVOID, SIZE_T, PVOID*, PULONG_PTR, PULONG);
    static GetWriteWatch_type GetWriteWatch_func;
    static DWORD GetWriteWatch_alloc_flag;

#   define GC_GWW_AVAILABLE() (GetWriteWatch_func != NULL)

    static void detect_GetWriteWatch(void)
    {
      static GC_bool done;
      if (done)
        return;

      GetWriteWatch_func = (GetWriteWatch_type)
        GetProcAddress(GetModuleHandle("kernel32.dll"), "GetWriteWatch");
      if (GetWriteWatch_func != NULL) {
        /* Also check whether VirtualAlloc accepts MEM_WRITE_WATCH,   */
        /* as some versions of kernel32.dll have one but not the      */
        /* other, making the feature completely broken.               */
        void * page = VirtualAlloc(NULL, GC_page_size,
                                    MEM_WRITE_WATCH | MEM_RESERVE,
                                    PAGE_READWRITE);
        if (page != NULL) {
          PVOID pages[16];
          ULONG_PTR count = 16;
          DWORD page_size;
          /* Check that it actually works.  In spite of some 		*/
	  /* documentation it actually seems to exist on W2K.		*/
	  /* This test may be unnecessary, but ...			*/
          if (GetWriteWatch_func(WRITE_WATCH_FLAG_RESET,
                                 page, GC_page_size,
                                 pages,
                                 &count,
                                 &page_size) != 0) {
            /* GetWriteWatch always fails. */
            GetWriteWatch_func = NULL;
          } else {
            GetWriteWatch_alloc_flag = MEM_WRITE_WATCH;
          }
          VirtualFree(page, GC_page_size, MEM_RELEASE);
        } else {
          /* GetWriteWatch will be useless. */
          GetWriteWatch_func = NULL;
        }
      }
      if (GC_print_stats) {
        if (GetWriteWatch_func == NULL) {
          GC_log_printf("Did not find a usable GetWriteWatch()\n");
        } else {
          GC_log_printf("Using GetWriteWatch()\n");
        }
      }
      done = TRUE;
    }

# endif /* GWW_VDB */

  GC_bool GC_wnt = FALSE;
         /* This is a Windows NT derivative, i.e. NT, W2K, XP or later.  */
  
  void GC_init_win32(void)
  {
    /* Set GC_wnt.							 */
    /* If we're running under win32s, assume that no DLLs will be loaded */
    /* I doubt anyone still runs win32s, but ...			 */
    DWORD v = GetVersion();
    GC_wnt = !(v & 0x80000000);
    GC_no_win32_dlls |= ((!GC_wnt) && (v & 0xff) <= 3);
  }

  /* Return the smallest address a such that VirtualQuery		*/
  /* returns correct results for all addresses between a and start.	*/
  /* Assumes VirtualQuery returns correct information for start.	*/
  ptr_t GC_least_described_address(ptr_t start)
  {  
    MEMORY_BASIC_INFORMATION buf;
    size_t result;
    LPVOID limit;
    ptr_t p;
    LPVOID q;
    
    limit = GC_sysinfo.lpMinimumApplicationAddress;
    p = (ptr_t)((word)start & ~(GC_page_size - 1));
    for (;;) {
    	q = (LPVOID)(p - GC_page_size);
    	if ((ptr_t)q > (ptr_t)p /* underflow */ || q < limit) break;
    	result = VirtualQuery(q, &buf, sizeof(buf));
    	if (result != sizeof(buf) || buf.AllocationBase == 0) break;
    	p = (ptr_t)(buf.AllocationBase);
    }
    return p;
  }
# endif

# ifndef REDIRECT_MALLOC
  /* We maintain a linked list of AllocationBase values that we know	*/
  /* correspond to malloc heap sections.  Currently this is only called */
  /* during a GC.  But there is some hope that for long running		*/
  /* programs we will eventually see most heap sections.		*/

  /* In the long run, it would be more reliable to occasionally walk 	*/
  /* the malloc heap with HeapWalk on the default heap.  But that	*/
  /* apparently works only for NT-based Windows. 			*/ 

  /* In the long run, a better data structure would also be nice ...	*/
  struct GC_malloc_heap_list {
    void * allocation_base;
    struct GC_malloc_heap_list *next;
  } *GC_malloc_heap_l = 0;

  /* Is p the base of one of the malloc heap sections we already know	*/
  /* about?								*/
  GC_bool GC_is_malloc_heap_base(ptr_t p)
  {
    struct GC_malloc_heap_list *q = GC_malloc_heap_l;

    while (0 != q) {
      if (q -> allocation_base == p) return TRUE;
      q = q -> next;
    }
    return FALSE;
  }

  void *GC_get_allocation_base(void *p)
  {
    MEMORY_BASIC_INFORMATION buf;
    size_t result = VirtualQuery(p, &buf, sizeof(buf));
    if (result != sizeof(buf)) {
      ABORT("Weird VirtualQuery result");
    }
    return buf.AllocationBase;
  }

  size_t GC_max_root_size = 100000;	/* Appr. largest root size.	*/

  void GC_add_current_malloc_heap()
  {
    struct GC_malloc_heap_list *new_l =
                 malloc(sizeof(struct GC_malloc_heap_list));
    void * candidate = GC_get_allocation_base(new_l);

    if (new_l == 0) return;
    if (GC_is_malloc_heap_base(candidate)) {
      /* Try a little harder to find malloc heap.			*/
	size_t req_size = 10000;
	do {
	  void *p = malloc(req_size);
	  if (0 == p) { free(new_l); return; }
 	  candidate = GC_get_allocation_base(p);
	  free(p);
	  req_size *= 2;
	} while (GC_is_malloc_heap_base(candidate)
	         && req_size < GC_max_root_size/10 && req_size < 500000);
	if (GC_is_malloc_heap_base(candidate)) {
	  free(new_l); return;
	}
    }
    if (GC_print_stats)
	  GC_log_printf("Found new system malloc AllocationBase at %p\n",
                        candidate);
    new_l -> allocation_base = candidate;
    new_l -> next = GC_malloc_heap_l;
    GC_malloc_heap_l = new_l;
  }
# endif /* REDIRECT_MALLOC */
  
  /* Is p the start of either the malloc heap, or of one of our */
  /* heap sections?						*/
  GC_bool GC_is_heap_base (ptr_t p)
  {
     
     unsigned i;
     
#    ifndef REDIRECT_MALLOC
       static word last_gc_no = (word)(-1);
     
       if (last_gc_no != GC_gc_no) {
	 GC_add_current_malloc_heap();
	 last_gc_no = GC_gc_no;
       }
       if (GC_root_size > GC_max_root_size) GC_max_root_size = GC_root_size;
       if (GC_is_malloc_heap_base(p)) return TRUE;
#    endif
     for (i = 0; i < GC_n_heap_bases; i++) {
         if (GC_heap_bases[i] == p) return TRUE;
     }
     return FALSE ;
  }

# ifdef MSWIN32
  void GC_register_root_section(ptr_t static_root)
  {
      MEMORY_BASIC_INFORMATION buf;
      size_t result;
      DWORD protect;
      LPVOID p;
      char * base;
      char * limit, * new_limit;
    
      if (!GC_no_win32_dlls) return;
      p = base = limit = GC_least_described_address(static_root);
      while (p < GC_sysinfo.lpMaximumApplicationAddress) {
        result = VirtualQuery(p, &buf, sizeof(buf));
        if (result != sizeof(buf) || buf.AllocationBase == 0
            || GC_is_heap_base(buf.AllocationBase)) break;
        new_limit = (char *)p + buf.RegionSize;
        protect = buf.Protect;
        if (buf.State == MEM_COMMIT
            && is_writable(protect)) {
            if ((char *)p == limit) {
                limit = new_limit;
            } else {
                if (base != limit) GC_add_roots_inner(base, limit, FALSE);
                base = p;
                limit = new_limit;
            }
        }
        if (p > (LPVOID)new_limit /* overflow */) break;
        p = (LPVOID)new_limit;
      }
      if (base != limit) GC_add_roots_inner(base, limit, FALSE);
  }
#endif
  
  void GC_register_data_segments()
  {
#     ifdef MSWIN32
      static char dummy;
      GC_register_root_section((ptr_t)(&dummy));
#     endif
  }

# else /* !OS2 && !Windows */

# if (defined(SVR4) || defined(AUX) || defined(DGUX) \
      || (defined(LINUX) && defined(SPARC))) && !defined(PCR)
ptr_t GC_SysVGetDataStart(size_t max_page_size, ptr_t etext_addr)
{
    word text_end = ((word)(etext_addr) + sizeof(word) - 1)
    		    & ~(sizeof(word) - 1);
    	/* etext rounded to word boundary	*/
    word next_page = ((text_end + (word)max_page_size - 1)
    		      & ~((word)max_page_size - 1));
    word page_offset = (text_end & ((word)max_page_size - 1));
    volatile char * result = (char *)(next_page + page_offset);
    /* Note that this isnt equivalent to just adding		*/
    /* max_page_size to &etext if &etext is at a page boundary	*/
    
    GC_setup_temporary_fault_handler();
    if (SETJMP(GC_jmp_buf) == 0) {
    	/* Try writing to the address.	*/
    	*result = *result;
        GC_reset_fault_handler();
    } else {
        GC_reset_fault_handler();
    	/* We got here via a longjmp.  The address is not readable.	*/
    	/* This is known to happen under Solaris 2.4 + gcc, which place	*/
    	/* string constants in the text segment, but after etext.	*/
    	/* Use plan B.  Note that we now know there is a gap between	*/
    	/* text and data segments, so plan A bought us something.	*/
    	result = (char *)GC_find_limit((ptr_t)(DATAEND), FALSE);
    }
    return((ptr_t)result);
}
# endif

# if defined(FREEBSD) && (defined(I386) || defined(X86_64) || defined(powerpc) || defined(__powerpc__)) && !defined(PCR)
/* Its unclear whether this should be identical to the above, or 	*/
/* whether it should apply to non-X86 architectures.			*/
/* For now we don't assume that there is always an empty page after	*/
/* etext.  But in some cases there actually seems to be slightly more.  */
/* This also deals with holes between read-only data and writable data.	*/
ptr_t GC_FreeBSDGetDataStart(size_t max_page_size, ptr_t etext_addr)
{
    word text_end = ((word)(etext_addr) + sizeof(word) - 1)
		     & ~(sizeof(word) - 1);
	/* etext rounded to word boundary	*/
    volatile word next_page = (text_end + (word)max_page_size - 1)
			      & ~((word)max_page_size - 1);
    volatile ptr_t result = (ptr_t)text_end;
    GC_setup_temporary_fault_handler();
    if (SETJMP(GC_jmp_buf) == 0) {
	/* Try reading at the address.				*/
	/* This should happen before there is another thread.	*/
	for (; next_page < (word)(DATAEND); next_page += (word)max_page_size)
	    *(volatile char *)next_page;
	GC_reset_fault_handler();
    } else {
	GC_reset_fault_handler();
	/* As above, we go to plan B	*/
	result = GC_find_limit((ptr_t)(DATAEND), FALSE);
    }
    return(result);
}

# endif


#ifdef AMIGA

#  define GC_AMIGA_DS
#  include "AmigaOS.c"
#  undef GC_AMIGA_DS

#else /* !OS2 && !Windows && !AMIGA */

void GC_register_data_segments(void)
{
#   if !defined(PCR) && !defined(MACOS)
#     if defined(REDIRECT_MALLOC) && defined(GC_SOLARIS_THREADS)
	/* As of Solaris 2.3, the Solaris threads implementation	*/
	/* allocates the data structure for the initial thread with	*/
	/* sbrk at process startup.  It needs to be scanned, so that	*/
	/* we don't lose some malloc allocated data structures		*/
	/* hanging from it.  We're on thin ice here ...			*/
        extern caddr_t sbrk();

	GC_add_roots_inner(DATASTART, (ptr_t)sbrk(0), FALSE);
#     else
	GC_add_roots_inner(DATASTART, (ptr_t)(DATAEND), FALSE);
#       if defined(DATASTART2)
          GC_add_roots_inner(DATASTART2, (ptr_t)(DATAEND2), FALSE);
#       endif
#     endif
#   endif
#   if defined(MACOS)
    {
#   if defined(THINK_C)
	extern void* GC_MacGetDataStart(void);
	/* globals begin above stack and end at a5. */
	GC_add_roots_inner((ptr_t)GC_MacGetDataStart(),
			   (ptr_t)LMGetCurrentA5(), FALSE);
#   else
#     if defined(__MWERKS__)
#       if !__POWERPC__
	  extern void* GC_MacGetDataStart(void);
	  /* MATTHEW: Function to handle Far Globals (CW Pro 3) */
#         if __option(far_data)
	  extern void* GC_MacGetDataEnd(void);
#         endif
	  /* globals begin above stack and end at a5. */
	  GC_add_roots_inner((ptr_t)GC_MacGetDataStart(),
          		     (ptr_t)LMGetCurrentA5(), FALSE);
	  /* MATTHEW: Handle Far Globals */          		     
#         if __option(far_data)
      /* Far globals follow he QD globals: */
	  GC_add_roots_inner((ptr_t)LMGetCurrentA5(),
          		     (ptr_t)GC_MacGetDataEnd(), FALSE);
#         endif
#       else
	  extern char __data_start__[], __data_end__[];
	  GC_add_roots_inner((ptr_t)&__data_start__,
	  		     (ptr_t)&__data_end__, FALSE);
#       endif /* __POWERPC__ */
#     endif /* __MWERKS__ */
#   endif /* !THINK_C */
    }
#   endif /* MACOS */

    /* Dynamic libraries are added at every collection, since they may  */
    /* change.								*/
}

# endif  /* ! AMIGA */
# endif  /* ! MSWIN32 && ! MSWINCE*/
# endif  /* ! OS2 */

/*
 * Auxiliary routines for obtaining memory from OS.
 */

# if !defined(OS2) && !defined(PCR) && !defined(AMIGA) \
	&& !defined(MSWIN32) && !defined(MSWINCE) \
	&& !defined(MACOS) && !defined(DOS4GW) && !defined(NONSTOP)

# define SBRK_ARG_T ptrdiff_t

#if defined(MMAP_SUPPORTED)

#ifdef USE_MMAP_FIXED
#   define GC_MMAP_FLAGS MAP_FIXED | MAP_PRIVATE
	/* Seems to yield better performance on Solaris 2, but can	*/
	/* be unreliable if something is already mapped at the address.	*/
#else
#   define GC_MMAP_FLAGS MAP_PRIVATE
#endif

#ifdef USE_MMAP_ANON
# define zero_fd -1
# if defined(MAP_ANONYMOUS)
#   define OPT_MAP_ANON MAP_ANONYMOUS
# else
#   define OPT_MAP_ANON MAP_ANON
# endif
#else
  static int zero_fd;
# define OPT_MAP_ANON 0
#endif 

#ifndef HEAP_START
#   define HEAP_START 0
#endif

ptr_t GC_unix_mmap_get_mem(word bytes)
{
    void *result;
    static ptr_t last_addr = HEAP_START;

#   ifndef USE_MMAP_ANON
      static GC_bool initialized = FALSE;

      if (!initialized) {
	  zero_fd = open("/dev/zero", O_RDONLY);
	  fcntl(zero_fd, F_SETFD, FD_CLOEXEC);
	  initialized = TRUE;
      }
#   endif

    if (bytes & (GC_page_size -1)) ABORT("Bad GET_MEM arg");
    result = mmap(last_addr, bytes, PROT_READ | PROT_WRITE | OPT_PROT_EXEC,
		  GC_MMAP_FLAGS | OPT_MAP_ANON, zero_fd, 0/* offset */);
    if (result == MAP_FAILED) return(0);
    last_addr = (ptr_t)result + bytes + GC_page_size - 1;
    last_addr = (ptr_t)((word)last_addr & ~(GC_page_size - 1));
#   if !defined(LINUX)
      if (last_addr == 0) {
        /* Oops.  We got the end of the address space.  This isn't	*/
	/* usable by arbitrary C code, since one-past-end pointers	*/
	/* don't work, so we discard it and try again.			*/
	munmap(result, (size_t)(-GC_page_size) - (size_t)result);
			/* Leave last page mapped, so we can't repeat. */
	return GC_unix_mmap_get_mem(bytes);
      }
#   else
      GC_ASSERT(last_addr != 0);
#   endif
    return((ptr_t)result);
}

# endif  /* MMAP_SUPPORTED */

#if defined(USE_MMAP)

ptr_t GC_unix_get_mem(word bytes)
{
    return GC_unix_mmap_get_mem(bytes);
}

#else /* Not USE_MMAP */

ptr_t GC_unix_sbrk_get_mem(word bytes)
{
  ptr_t result;
# ifdef IRIX5
    /* Bare sbrk isn't thread safe.  Play by malloc rules.	*/
    /* The equivalent may be needed on other systems as well. 	*/
    __LOCK_MALLOC();
# endif
  {
    ptr_t cur_brk = (ptr_t)sbrk(0);
    SBRK_ARG_T lsbs = (word)cur_brk & (GC_page_size-1);
    
    if ((SBRK_ARG_T)bytes < 0) {
	result = 0; /* too big */
	goto out;
    }
    if (lsbs != 0) {
        if((ptr_t)sbrk(GC_page_size - lsbs) == (ptr_t)(-1)) {
	    result = 0;
	    goto out;
	}
    }
#   ifdef ADD_HEAP_GUARD_PAGES
      /* This is useful for catching severe memory overwrite problems that */
      /* span heap sections.  It shouldn't otherwise be turned on.	   */
      {
	ptr_t guard = (ptr_t)sbrk((SBRK_ARG_T)GC_page_size);
	if (mprotect(guard, GC_page_size, PROT_NONE) != 0)
	    ABORT("ADD_HEAP_GUARD_PAGES: mprotect failed");
      }
#   endif /* ADD_HEAP_GUARD_PAGES */
    result = (ptr_t)sbrk((SBRK_ARG_T)bytes);
    if (result == (ptr_t)(-1)) result = 0;
  }
 out:
# ifdef IRIX5
    __UNLOCK_MALLOC();
# endif
  return(result);
}

#if defined(MMAP_SUPPORTED)

/* By default, we try both sbrk and mmap, in that order. */
ptr_t GC_unix_get_mem(word bytes)
{
    static GC_bool sbrk_failed = FALSE;
    ptr_t result = 0;

    if (!sbrk_failed) result = GC_unix_sbrk_get_mem(bytes);
    if (0 == result) {
	sbrk_failed = TRUE;
	result = GC_unix_mmap_get_mem(bytes);
    }
    if (0 == result) {
	/* Try sbrk again, in case sbrk memory became available. */
	result = GC_unix_sbrk_get_mem(bytes);
    }
    return result;
}

#else /* !MMAP_SUPPORTED */

ptr_t GC_unix_get_mem(word bytes)
{
    return GC_unix_sbrk_get_mem(bytes);
}

#endif

#endif /* Not USE_MMAP */

# endif /* UN*X */

# ifdef OS2

void * os2_alloc(size_t bytes)
{
    void * result;

    if (DosAllocMem(&result, bytes, PAG_EXECUTE | PAG_READ |
    				    PAG_WRITE | PAG_COMMIT)
		    != NO_ERROR) {
	return(0);
    }
    if (result == 0) return(os2_alloc(bytes));
    return(result);
}

# endif /* OS2 */


# if defined(MSWIN32) || defined(MSWINCE)
SYSTEM_INFO GC_sysinfo;
# endif

# ifdef MSWIN32

# ifdef USE_GLOBAL_ALLOC
#   define GLOBAL_ALLOC_TEST 1
# else
#   define GLOBAL_ALLOC_TEST GC_no_win32_dlls
# endif

word GC_n_heap_bases = 0;

word GC_mem_top_down = 0;  /* Change to MEM_TOP_DOWN  for better 64-bit */
			   /* testing.  Otherwise all addresses tend to */
			   /* end up in first 4GB, hiding bugs.		*/

ptr_t GC_win32_get_mem(word bytes)
{
    ptr_t result;

    if (GLOBAL_ALLOC_TEST) {
    	/* VirtualAlloc doesn't like PAGE_EXECUTE_READWRITE.	*/
    	/* There are also unconfirmed rumors of other		*/
    	/* problems, so we dodge the issue.			*/
        result = (ptr_t) GlobalAlloc(0, bytes + HBLKSIZE);
        result = (ptr_t)(((word)result + HBLKSIZE - 1) & ~(HBLKSIZE-1));
    } else {
	/* VirtualProtect only works on regions returned by a	*/
	/* single VirtualAlloc call.  Thus we allocate one 	*/
	/* extra page, which will prevent merging of blocks	*/
	/* in separate regions, and eliminate any temptation	*/
	/* to call VirtualProtect on a range spanning regions.	*/
	/* This wastes a small amount of memory, and risks	*/
	/* increased fragmentation.  But better alternatives	*/
	/* would require effort.				*/
        /* Pass the MEM_WRITE_WATCH only if GetWriteWatch-based */
        /* VDBs are enabled and the GetWriteWatch function is   */
        /* available.  Otherwise we waste resources or possibly */
        /* cause VirtualAlloc to fail (observed in Windows 2000 */
        /* SP2).                                                */
        result = (ptr_t) VirtualAlloc(NULL, bytes + 1,
#                                     ifdef GWW_VDB
                                        GetWriteWatch_alloc_flag |
#                                     endif
    				      MEM_COMMIT | MEM_RESERVE
				      | GC_mem_top_down,
    				      PAGE_EXECUTE_READWRITE);
    }
    if (HBLKDISPL(result) != 0) ABORT("Bad VirtualAlloc result");
    	/* If I read the documentation correctly, this can	*/
    	/* only happen if HBLKSIZE > 64k or not a power of 2.	*/
    if (GC_n_heap_bases >= MAX_HEAP_SECTS) ABORT("Too many heap sections");
    GC_heap_bases[GC_n_heap_bases++] = result;
    return(result);			  
}

void GC_win32_free_heap(void)
{
    if (GC_no_win32_dlls) {
 	while (GC_n_heap_bases > 0) {
 	    GlobalFree (GC_heap_bases[--GC_n_heap_bases]);
 	    GC_heap_bases[GC_n_heap_bases] = 0;
 	}
    }
}
# endif

#ifdef AMIGA
# define GC_AMIGA_AM
# include "AmigaOS.c"
# undef GC_AMIGA_AM
#endif


# ifdef MSWINCE
word GC_n_heap_bases = 0;

ptr_t GC_wince_get_mem(word bytes)
{
    ptr_t result;
    word i;

    /* Round up allocation size to multiple of page size */
    bytes = (bytes + GC_page_size-1) & ~(GC_page_size-1);

    /* Try to find reserved, uncommitted pages */
    for (i = 0; i < GC_n_heap_bases; i++) {
	if (((word)(-(signed_word)GC_heap_lengths[i])
	     & (GC_sysinfo.dwAllocationGranularity-1))
	    >= bytes) {
	    result = GC_heap_bases[i] + GC_heap_lengths[i];
	    break;
	}
    }

    if (i == GC_n_heap_bases) {
	/* Reserve more pages */
	word res_bytes = (bytes + GC_sysinfo.dwAllocationGranularity-1)
			 & ~(GC_sysinfo.dwAllocationGranularity-1);
	/* If we ever support MPROTECT_VDB here, we will probably need to    */
	/* ensure that res_bytes is strictly > bytes, so that VirtualProtect */
	/* never spans regions.  It seems to be OK for a VirtualFree	     */
	/* argument to span regions, so we should be OK for now.	     */
	result = (ptr_t) VirtualAlloc(NULL, res_bytes,
    				      MEM_RESERVE | MEM_TOP_DOWN,
    				      PAGE_EXECUTE_READWRITE);
	if (HBLKDISPL(result) != 0) ABORT("Bad VirtualAlloc result");
    	    /* If I read the documentation correctly, this can	*/
    	    /* only happen if HBLKSIZE > 64k or not a power of 2.	*/
	if (GC_n_heap_bases >= MAX_HEAP_SECTS) ABORT("Too many heap sections");
	GC_heap_bases[GC_n_heap_bases] = result;
	GC_heap_lengths[GC_n_heap_bases] = 0;
	GC_n_heap_bases++;
    }

    /* Commit pages */
    result = (ptr_t) VirtualAlloc(result, bytes,
				  MEM_COMMIT,
    				  PAGE_EXECUTE_READWRITE);
    if (result != NULL) {
	if (HBLKDISPL(result) != 0) ABORT("Bad VirtualAlloc result");
	GC_heap_lengths[i] += bytes;
    }

    return(result);			  
}
# endif

#ifdef USE_MUNMAP

/* For now, this only works on Win32/WinCE and some Unix-like	*/
/* systems.  If you have something else, don't define		*/
/* USE_MUNMAP.							*/
/* We assume ANSI C to support this feature.			*/

#if !defined(MSWIN32) && !defined(MSWINCE)

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#endif

/* Compute a page aligned starting address for the unmap 	*/
/* operation on a block of size bytes starting at start.	*/
/* Return 0 if the block is too small to make this feasible.	*/
ptr_t GC_unmap_start(ptr_t start, size_t bytes)
{
    ptr_t result = start;
    /* Round start to next page boundary.       */
        result += GC_page_size - 1;
        result = (ptr_t)((word)result & ~(GC_page_size - 1));
    if (result + GC_page_size > start + bytes) return 0;
    return result;
}

/* Compute end address for an unmap operation on the indicated	*/
/* block.							*/
ptr_t GC_unmap_end(ptr_t start, size_t bytes)
{
    ptr_t end_addr = start + bytes;
    end_addr = (ptr_t)((word)end_addr & ~(GC_page_size - 1));
    return end_addr;
}

/* Under Win32/WinCE we commit (map) and decommit (unmap)	*/
/* memory using	VirtualAlloc and VirtualFree.  These functions	*/
/* work on individual allocations of virtual memory, made	*/
/* previously using VirtualAlloc with the MEM_RESERVE flag.	*/
/* The ranges we need to (de)commit may span several of these	*/
/* allocations; therefore we use VirtualQuery to check		*/
/* allocation lengths, and split up the range as necessary.	*/

/* We assume that GC_remap is called on exactly the same range	*/
/* as a previous call to GC_unmap.  It is safe to consistently	*/
/* round the endpoints in both places.				*/
void GC_unmap(ptr_t start, size_t bytes)
{
    ptr_t start_addr = GC_unmap_start(start, bytes);
    ptr_t end_addr = GC_unmap_end(start, bytes);
    word len = end_addr - start_addr;
    if (0 == start_addr) return;
#   if defined(MSWIN32) || defined(MSWINCE)
      while (len != 0) {
          MEMORY_BASIC_INFORMATION mem_info;
	  GC_word free_len;
	  if (VirtualQuery(start_addr, &mem_info, sizeof(mem_info))
	      != sizeof(mem_info))
	      ABORT("Weird VirtualQuery result");
	  free_len = (len < mem_info.RegionSize) ? len : mem_info.RegionSize;
	  if (!VirtualFree(start_addr, free_len, MEM_DECOMMIT))
	      ABORT("VirtualFree failed");
	  GC_unmapped_bytes += free_len;
	  start_addr += free_len;
	  len -= free_len;
      }
#   else
      /* We immediately remap it to prevent an intervening mmap from	*/
      /* accidentally grabbing the same address space.			*/
      {
	void * result;
        result = mmap(start_addr, len, PROT_NONE,
		      MAP_PRIVATE | MAP_FIXED | OPT_MAP_ANON,
		      zero_fd, 0/* offset */);
        if (result != (void *)start_addr) ABORT("mmap(...PROT_NONE...) failed");
      }
      GC_unmapped_bytes += len;
#   endif
}


void GC_remap(ptr_t start, size_t bytes)
{
    ptr_t start_addr = GC_unmap_start(start, bytes);
    ptr_t end_addr = GC_unmap_end(start, bytes);
    word len = end_addr - start_addr;

#   if defined(MSWIN32) || defined(MSWINCE)
      ptr_t result;

      if (0 == start_addr) return;
      while (len != 0) {
          MEMORY_BASIC_INFORMATION mem_info;
	  GC_word alloc_len;
	  if (VirtualQuery(start_addr, &mem_info, sizeof(mem_info))
	      != sizeof(mem_info))
	      ABORT("Weird VirtualQuery result");
	  alloc_len = (len < mem_info.RegionSize) ? len : mem_info.RegionSize;
	  result = VirtualAlloc(start_addr, alloc_len,
				MEM_COMMIT,
				PAGE_EXECUTE_READWRITE);
	  if (result != start_addr) {
	      ABORT("VirtualAlloc remapping failed");
	  }
	  GC_unmapped_bytes -= alloc_len;
	  start_addr += alloc_len;
	  len -= alloc_len;
      }
#   else
      /* It was already remapped with PROT_NONE. */
      int result; 

      if (0 == start_addr) return;
      result = mprotect(start_addr, len,
		        PROT_READ | PROT_WRITE | OPT_PROT_EXEC);
      if (result != 0) {
	  GC_err_printf(
		"Mprotect failed at %p (length %ld) with errno %d\n",
	        start_addr, (unsigned long)len, errno);
	  ABORT("Mprotect remapping failed");
      }
      GC_unmapped_bytes -= len;
#   endif
}

/* Two adjacent blocks have already been unmapped and are about to	*/
/* be merged.  Unmap the whole block.  This typically requires		*/
/* that we unmap a small section in the middle that was not previously	*/
/* unmapped due to alignment constraints.				*/
void GC_unmap_gap(ptr_t start1, size_t bytes1, ptr_t start2, size_t bytes2)
{
    ptr_t start1_addr = GC_unmap_start(start1, bytes1);
    ptr_t end1_addr = GC_unmap_end(start1, bytes1);
    ptr_t start2_addr = GC_unmap_start(start2, bytes2);
    ptr_t end2_addr = GC_unmap_end(start2, bytes2);
    ptr_t start_addr = end1_addr;
    ptr_t end_addr = start2_addr;
    size_t len;
    GC_ASSERT(start1 + bytes1 == start2);
    if (0 == start1_addr) start_addr = GC_unmap_start(start1, bytes1 + bytes2);
    if (0 == start2_addr) end_addr = GC_unmap_end(start1, bytes1 + bytes2);
    if (0 == start_addr) return;
    len = end_addr - start_addr;
#   if defined(MSWIN32) || defined(MSWINCE)
      while (len != 0) {
          MEMORY_BASIC_INFORMATION mem_info;
	  GC_word free_len;
	  if (VirtualQuery(start_addr, &mem_info, sizeof(mem_info))
	      != sizeof(mem_info))
	      ABORT("Weird VirtualQuery result");
	  free_len = (len < mem_info.RegionSize) ? len : mem_info.RegionSize;
	  if (!VirtualFree(start_addr, free_len, MEM_DECOMMIT))
	      ABORT("VirtualFree failed");
	  GC_unmapped_bytes += free_len;
	  start_addr += free_len;
	  len -= free_len;
      }
#   else
      if (len != 0 && munmap(start_addr, len) != 0) ABORT("munmap failed");
      GC_unmapped_bytes += len;
#   endif
}

#endif /* USE_MUNMAP */

/* Routine for pushing any additional roots.  In THREADS 	*/
/* environment, this is also responsible for marking from 	*/
/* thread stacks. 						*/
#ifndef THREADS
void (*GC_push_other_roots)(void) = 0;
#else /* THREADS */

# ifdef PCR
PCR_ERes GC_push_thread_stack(PCR_Th_T *t, PCR_Any dummy)
{
    struct PCR_ThCtl_TInfoRep info;
    PCR_ERes result;
    
    info.ti_stkLow = info.ti_stkHi = 0;
    result = PCR_ThCtl_GetInfo(t, &info);
    GC_push_all_stack((ptr_t)(info.ti_stkLow), (ptr_t)(info.ti_stkHi));
    return(result);
}

/* Push the contents of an old object. We treat this as stack	*/
/* data only becasue that makes it robust against mark stack	*/
/* overflow.							*/
PCR_ERes GC_push_old_obj(void *p, size_t size, PCR_Any data)
{
    GC_push_all_stack((ptr_t)p, (ptr_t)p + size);
    return(PCR_ERes_okay);
}


void GC_default_push_other_roots(void)
{
    /* Traverse data allocated by previous memory managers.		*/
	{
	  extern struct PCR_MM_ProcsRep * GC_old_allocator;
	  
	  if ((*(GC_old_allocator->mmp_enumerate))(PCR_Bool_false,
	  					   GC_push_old_obj, 0)
	      != PCR_ERes_okay) {
	      ABORT("Old object enumeration failed");
	  }
	}
    /* Traverse all thread stacks. */
	if (PCR_ERes_IsErr(
                PCR_ThCtl_ApplyToAllOtherThreads(GC_push_thread_stack,0))
              || PCR_ERes_IsErr(GC_push_thread_stack(PCR_Th_CurrThread(), 0))) {
              ABORT("Thread stack marking failed\n");
	}
}

# endif /* PCR */


# if defined(GC_PTHREADS) || defined(GC_WIN32_THREADS)

extern void GC_push_all_stacks(void);

void GC_default_push_other_roots(void)
{
    GC_push_all_stacks();
}

# endif /* GC_WIN32_THREADS || GC_PTHREADS */

void (*GC_push_other_roots)(void) = GC_default_push_other_roots;

#endif /* THREADS */

/*
 * Routines for accessing dirty  bits on virtual pages.
 * There are six ways to maintain this information:
 * DEFAULT_VDB:	A simple dummy implementation that treats every page
 *		as possibly dirty.  This makes incremental collection
 *		useless, but the implementation is still correct.
 * MANUAL_VDB:  Stacks and static data are always considered dirty.
 * 		Heap pages are considered dirty if GC_dirty(p) has been
 * 		called on some pointer p pointing to somewhere inside
 * 		an object on that page.  A GC_dirty() call on a large
 * 		object directly dirties only a single page, but for
 * 		MANUAL_VDB we are careful to treat an object with a dirty
 * 		page as completely dirty.
 * 		In order to avoid races, an object must be marked dirty
 * 		after it is written, and a reference to the object
 * 		must be kept on a stack or in a register in the interim.
 * 		With threads enabled, an object directly reachable from the
 * 		stack at the time of a collection is treated as dirty.
 * 		In single-threaded mode, it suffices to ensure that no
 * 		collection can take place between the pointer assignment
 * 		and the GC_dirty() call.
 * PCR_VDB:	Use PPCRs virtual dirty bit facility.
 * PROC_VDB:	Use the /proc facility for reading dirty bits.  Only
 *		works under some SVR4 variants.  Even then, it may be
 *		too slow to be entirely satisfactory.  Requires reading
 *		dirty bits for entire address space.  Implementations tend
 *		to assume that the client is a (slow) debugger.
 * MPROTECT_VDB:Protect pages and then catch the faults to keep track of
 *		dirtied pages.  The implementation (and implementability)
 *		is highly system dependent.  This usually fails when system
 *		calls write to a protected page.  We prevent the read system
 *		call from doing so.  It is the clients responsibility to
 *		make sure that other system calls are similarly protected
 *		or write only to the stack.
 * GWW_VDB:     Use the Win32 GetWriteWatch functions, if available, to
 *              read dirty bits.  In case it is not available (because we
 *              are running on Windows 95, Windows 2000 or earlier),
 *              MPROTECT_VDB may be defined as a fallback strategy.
 */
GC_bool GC_dirty_maintained = FALSE;

#if defined(PROC_VDB) || defined(GWW_VDB)

/* Add all pages in pht2 to pht1 */
void GC_or_pages(page_hash_table pht1, page_hash_table pht2)
{
    register int i;
    
    for (i = 0; i < PHT_SIZE; i++) pht1[i] |= pht2[i];
}

#endif

#ifdef GWW_VDB

# define GC_GWW_BUF_LEN 1024
  static PVOID gww_buf[GC_GWW_BUF_LEN];

# ifdef MPROTECT_VDB
    GC_bool GC_gww_dirty_init(void)
    {
      detect_GetWriteWatch();
      return GC_GWW_AVAILABLE();
    }
# else
    void GC_dirty_init(void)
    {
      detect_GetWriteWatch();
      GC_dirty_maintained = GC_GWW_AVAILABLE();
    }
# endif

# ifdef MPROTECT_VDB
    static void GC_gww_read_dirty(void)
# else
    void GC_read_dirty(void)
# endif
  {
    word i;

    BZERO(GC_grungy_pages, sizeof(GC_grungy_pages));

    for (i = 0; i != GC_n_heap_sects; ++i) {
      ULONG_PTR count;

      do {
        PVOID * pages, * pages_end;
        DWORD page_size;

        pages = gww_buf;
        count = GC_GWW_BUF_LEN;
        /*
        * GetWriteWatch is documented as returning non-zero when it fails,
        * but the documentation doesn't explicitly say why it would fail or
        * what its behaviour will be if it fails.
	* It does appear to fail, at least on recent W2K instances, if
	* the underlying memory was not allocated with the appropriate
	* flag.  This is common if GC_enable_incremental is called
	* shortly after GC initialization.  To avoid modifying the
	* interface, we silently work around such a failure, it it only
	* affects the initial (small) heap allocation.
	* If there are more dirty
        * pages than will fit in the buffer, this is not treated as a
        * failure; we must check the page count in the loop condition.
	* Since each partial call will reset the status of some
	* pages, this should eventually terminate even in the overflow
	* case.
        */
        if (GetWriteWatch_func(WRITE_WATCH_FLAG_RESET,
                               GC_heap_sects[i].hs_start,
                               GC_heap_sects[i].hs_bytes,
                               pages,
                               &count,
                               &page_size) != 0) {
          static int warn_count = 0;
          unsigned j;
          struct hblk * start = (struct hblk *)GC_heap_sects[i].hs_start;
          static struct hblk *last_warned = 0;
          size_t nblocks = divHBLKSZ(GC_heap_sects[i].hs_bytes);

          if ( i != 0 && last_warned != start && warn_count++ < 5) {
            last_warned = start;
            WARN(
              "GC_gww_read_dirty unexpectedly failed at %ld: "
              "Falling back to marking all pages dirty\n", start);
          }
          for (j = 0; j < nblocks; ++j) {
              word hash = PHT_HASH(start + j);
              set_pht_entry_from_index(GC_grungy_pages, hash);
          }
          count = 1;  /* Done with this section. */
        } else /* succeeded */{
          pages_end = pages + count;
          while (pages != pages_end) {
            struct hblk * h = (struct hblk *) *pages++;
            struct hblk * h_end = (struct hblk *) ((char *) h + page_size);
            do
              set_pht_entry_from_index(GC_grungy_pages, PHT_HASH(h));
            while (++h < h_end);
          }
        }
      } while (count == GC_GWW_BUF_LEN);
    }

    GC_or_pages(GC_written_pages, GC_grungy_pages);
  }

# ifdef MPROTECT_VDB
    static GC_bool GC_gww_page_was_dirty(struct hblk * h)
# else
    GC_bool GC_page_was_dirty(struct hblk * h)
# endif
  {
    return HDR(h) == 0 || get_pht_entry_from_index(GC_grungy_pages, PHT_HASH(h));
  }

# ifdef MPROTECT_VDB
    static GC_bool GC_gww_page_was_ever_dirty(struct hblk * h)
# else
    GC_bool GC_page_was_ever_dirty(struct hblk * h)
# endif
  {
    return HDR(h) == 0 || get_pht_entry_from_index(GC_written_pages, PHT_HASH(h));
  }

# ifndef MPROTECT_VDB
    void GC_remove_protection(struct hblk *h, word nblocks, GC_bool is_ptrfree)
    {}
# endif

# endif /* GWW_VDB */

# ifdef DEFAULT_VDB

/* All of the following assume the allocation lock is held, and	*/
/* signals are disabled.					*/

/* The client asserts that unallocated pages in the heap are never	*/
/* written.								*/

/* Initialize virtual dirty bit implementation.			*/
void GC_dirty_init(void)
{
    if (GC_print_stats == VERBOSE)
      GC_log_printf("Initializing DEFAULT_VDB...\n");
    GC_dirty_maintained = TRUE;
}

/* Retrieve system dirty bits for heap to a local buffer.	*/
/* Restore the systems notion of which pages are dirty.		*/
void GC_read_dirty(void)
{}

/* Is the HBLKSIZE sized page at h marked dirty in the local buffer?	*/
/* If the actual page size is different, this returns TRUE if any	*/
/* of the pages overlapping h are dirty.  This routine may err on the	*/
/* side of labelling pages as dirty (and this implementation does).	*/
/*ARGSUSED*/
GC_bool GC_page_was_dirty(struct hblk *h)
{
    return(TRUE);
}

/*
 * The following two routines are typically less crucial.  They matter
 * most with large dynamic libraries, or if we can't accurately identify
 * stacks, e.g. under Solaris 2.X.  Otherwise the following default
 * versions are adequate.
 */
 
/* Could any valid GC heap pointer ever have been written to this page?	*/
/*ARGSUSED*/
GC_bool GC_page_was_ever_dirty(struct hblk *h)
{
    return(TRUE);
}

/* A call that:						*/
/* I) hints that [h, h+nblocks) is about to be written.	*/
/* II) guarantees that protection is removed.		*/
/* (I) may speed up some dirty bit implementations.	*/
/* (II) may be essential if we need to ensure that	*/
/* pointer-free system call buffers in the heap are 	*/
/* not protected.					*/
/*ARGSUSED*/
void GC_remove_protection(struct hblk *h, word nblocks, GC_bool is_ptrfree)
{
}

# endif /* DEFAULT_VDB */

# ifdef MANUAL_VDB

/* Initialize virtual dirty bit implementation.			*/
void GC_dirty_init(void)
{
    if (GC_print_stats == VERBOSE)
      GC_log_printf("Initializing MANUAL_VDB...\n");
    /* GC_dirty_pages and GC_grungy_pages are already cleared. */
    GC_dirty_maintained = TRUE;
}

/* Retrieve system dirty bits for heap to a local buffer.	*/
/* Restore the systems notion of which pages are dirty.		*/
void GC_read_dirty(void)
{
    BCOPY((word *)GC_dirty_pages, GC_grungy_pages,
          (sizeof GC_dirty_pages));
    BZERO((word *)GC_dirty_pages, (sizeof GC_dirty_pages));
}

/* Is the HBLKSIZE sized page at h marked dirty in the local buffer?	*/
/* If the actual page size is different, this returns TRUE if any	*/
/* of the pages overlapping h are dirty.  This routine may err on the	*/
/* side of labelling pages as dirty (and this implementation does).	*/
/*ARGSUSED*/
GC_bool GC_page_was_dirty(struct hblk *h)
{
    register word index;
    
    index = PHT_HASH(h);
    return(HDR(h) == 0 || get_pht_entry_from_index(GC_grungy_pages, index));
}
 
/* Could any valid GC heap pointer ever have been written to this page?	*/
/*ARGSUSED*/
GC_bool GC_page_was_ever_dirty(struct hblk *h)
{
    /* FIXME - implement me.	*/
    return(TRUE);
}

/* Mark the page containing p as dirty.  Logically, this dirties the	*/
/* entire object.							*/
void GC_dirty(ptr_t p)
{
    word index = PHT_HASH(p);
    async_set_pht_entry_from_index(GC_dirty_pages, index);
}

/*ARGSUSED*/
void GC_remove_protection(struct hblk *h, word nblocks, GC_bool is_ptrfree)
{
}

# endif /* MANUAL_VDB */


# ifdef MPROTECT_VDB

/*
 * See DEFAULT_VDB for interface descriptions.
 */

/*
 * This implementation maintains dirty bits itself by catching write
 * faults and keeping track of them.  We assume nobody else catches
 * SIGBUS or SIGSEGV.  We assume no write faults occur in system calls.
 * This means that clients must ensure that system calls don't write
 * to the write-protected heap.  Probably the best way to do this is to
 * ensure that system calls write at most to POINTERFREE objects in the
 * heap, and do even that only if we are on a platform on which those
 * are not protected.  Another alternative is to wrap system calls
 * (see example for read below), but the current implementation holds
 * applications. 
 * We assume the page size is a multiple of HBLKSIZE.
 * We prefer them to be the same.  We avoid protecting POINTERFREE
 * objects only if they are the same.
 */

# if !defined(MSWIN32) && !defined(MSWINCE) && !defined(DARWIN)

#   include <sys/mman.h>
#   include <signal.h>
#   include <sys/syscall.h>

#   define PROTECT(addr, len) \
    	  if (mprotect((caddr_t)(addr), (size_t)(len), \
    	      	       PROT_READ | OPT_PROT_EXEC) < 0) { \
    	    ABORT("mprotect failed"); \
    	  }
#   define UNPROTECT(addr, len) \
    	  if (mprotect((caddr_t)(addr), (size_t)(len), \
    	  	       PROT_WRITE | PROT_READ | OPT_PROT_EXEC ) < 0) { \
    	    ABORT("un-mprotect failed"); \
    	  }
    	  
# else

# ifdef DARWIN
    /* Using vm_protect (mach syscall) over mprotect (BSD syscall) seems to
       decrease the likelihood of some of the problems described below. */
    #include <mach/vm_map.h>
    static mach_port_t GC_task_self;
    #define PROTECT(addr,len) \
        if(vm_protect(GC_task_self,(vm_address_t)(addr),(vm_size_t)(len), \
                FALSE,VM_PROT_READ) != KERN_SUCCESS) { \
            ABORT("vm_portect failed"); \
        }
    #define UNPROTECT(addr,len) \
        if(vm_protect(GC_task_self,(vm_address_t)(addr),(vm_size_t)(len), \
                FALSE,VM_PROT_READ|VM_PROT_WRITE) != KERN_SUCCESS) { \
            ABORT("vm_portect failed"); \
        }
# else
    
#   ifndef MSWINCE
#     include <signal.h>
#   endif

    static DWORD protect_junk;
#   define PROTECT(addr, len) \
	  if (!VirtualProtect((addr), (len), PAGE_EXECUTE_READ, \
	  		      &protect_junk)) { \
	    DWORD last_error = GetLastError(); \
	    GC_printf("Last error code: %lx\n", last_error); \
	    ABORT("VirtualProtect failed"); \
	  }
#   define UNPROTECT(addr, len) \
	  if (!VirtualProtect((addr), (len), PAGE_EXECUTE_READWRITE, \
	  		      &protect_junk)) { \
	    ABORT("un-VirtualProtect failed"); \
	  }
# endif /* !DARWIN */
# endif /* MSWIN32 || MSWINCE || DARWIN */

#if defined(MSWIN32)
    typedef LPTOP_LEVEL_EXCEPTION_FILTER SIG_HNDLR_PTR;
#   undef SIG_DFL
#   define SIG_DFL (LPTOP_LEVEL_EXCEPTION_FILTER) (-1)
#elif defined(MSWINCE)
    typedef LONG (WINAPI *SIG_HNDLR_PTR)(struct _EXCEPTION_POINTERS *);
#   undef SIG_DFL
#   define SIG_DFL (SIG_HNDLR_PTR) (-1)
#elif defined(DARWIN)
    typedef void (* SIG_HNDLR_PTR)();
#else
    typedef void (* SIG_HNDLR_PTR)(int, siginfo_t *, void *);
    typedef void (* PLAIN_HNDLR_PTR)(int);
#endif

#if defined(__GLIBC__)
#   if __GLIBC__ < 2 || __GLIBC__ == 2 && __GLIBC_MINOR__ < 2
#	error glibc too old?
#   endif
#endif

#ifndef DARWIN
SIG_HNDLR_PTR GC_old_bus_handler;
GC_bool GC_old_bus_handler_used_si;
SIG_HNDLR_PTR GC_old_segv_handler;
			/* Also old MSWIN32 ACCESS_VIOLATION filter */
GC_bool GC_old_segv_handler_used_si;
#endif /* !DARWIN */

#if defined(THREADS)
/* We need to lock around the bitmap update in the write fault handler	*/
/* in order to avoid the risk of losing a bit.  We do this with a 	*/
/* test-and-set spin lock if we know how to do that.  Otherwise we	*/
/* check whether we are already in the handler and use the dumb but	*/
/* safe fallback algorithm of setting all bits in the word.		*/
/* Contention should be very rare, so we do the minimum to handle it	*/
/* correctly.								*/
#ifdef AO_HAVE_test_and_set_acquire
  static volatile AO_TS_t fault_handler_lock = 0;
  void async_set_pht_entry_from_index(volatile page_hash_table db, size_t index) {
    while (AO_test_and_set_acquire(&fault_handler_lock) == AO_TS_SET) {}
    /* Could also revert to set_pht_entry_from_index_safe if initial	*/
    /* GC_test_and_set fails.						*/
    set_pht_entry_from_index(db, index);
    AO_CLEAR(&fault_handler_lock);
  }
#else /* !AO_have_test_and_set_acquire */
# error No test_and_set operation: Introduces a race.
  /* THIS WOULD BE INCORRECT!						*/
  /* The dirty bit vector may be temporarily wrong,			*/
  /* just before we notice the conflict and correct it. We may end up   */
  /* looking at it while it's wrong.  But this requires contention	*/
  /* exactly when a GC is triggered, which seems far less likely to	*/
  /* fail than the old code, which had no reported failures.  Thus we	*/
  /* leave it this way while we think of something better, or support	*/
  /* GC_test_and_set on the remaining platforms.			*/
  static volatile word currently_updating = 0;
  void async_set_pht_entry_from_index(volatile page_hash_table db, size_t index) {
    unsigned int update_dummy;
    currently_updating = (word)(&update_dummy);
    set_pht_entry_from_index(db, index);
    /* If we get contention in the 10 or so instruction window here,	*/
    /* and we get stopped by a GC between the two updates, we lose!	*/
    if (currently_updating != (word)(&update_dummy)) {
	set_pht_entry_from_index_safe(db, index);
	/* We claim that if two threads concurrently try to update the	*/
	/* dirty bit vector, the first one to execute UPDATE_START 	*/
	/* will see it changed when UPDATE_END is executed.  (Note that	*/
	/* &update_dummy must differ in two distinct threads.)  It	*/
	/* will then execute set_pht_entry_from_index_safe, thus 	*/
	/* returning us to a safe state, though not soon enough.	*/
    }
  }
#endif /* !AO_HAVE_test_and_set_acquire */
#else /* !THREADS */
# define async_set_pht_entry_from_index(db, index) \
	set_pht_entry_from_index(db, index)
#endif /* !THREADS */

#if !defined(DARWIN)
#   include <errno.h>
#   if defined(FREEBSD)
#     define SIG_OK TRUE
#     define CODE_OK (code == BUS_PAGE_FAULT)
#   elif defined(OSF1)
#     define SIG_OK (sig == SIGSEGV)
#     define CODE_OK (code == 2 /* experimentally determined */)
#   elif defined(IRIX5)
#     define SIG_OK (sig == SIGSEGV)
#     define CODE_OK (code == EACCES)
#   elif defined(HURD)
#     define SIG_OK (sig == SIGBUS || sig == SIGSEGV) 	
#     define CODE_OK  TRUE
#   elif defined(LINUX)
#     define SIG_OK (sig == SIGSEGV)
#     define CODE_OK TRUE
	/* Empirically c.trapno == 14, on IA32, but is that useful?     */
	/* Should probably consider alignment issues on other 		*/
	/* architectures.						*/
#   elif defined(HPUX)
#     define SIG_OK (sig == SIGSEGV || sig == SIGBUS)
#     define CODE_OK (si -> si_code == SEGV_ACCERR) \
		     || (si -> si_code == BUS_ADRERR) \
		     || (si -> si_code == BUS_UNKNOWN) \
		     || (si -> si_code == SEGV_UNKNOWN) \
		     || (si -> si_code == BUS_OBJERR)
#   elif defined(FREEBSD)
#     define SIG_OK (sig == SIGBUS)
#     define CODE_OK (si -> si_code == BUS_PAGE_FAULT)
#   elif defined(SUNOS5SIGS)
#     define SIG_OK (sig == SIGSEGV)
#     define CODE_OK (si -> si_code == SEGV_ACCERR)
#   elif defined(MSWIN32) || defined(MSWINCE)
#     define SIG_OK (exc_info -> ExceptionRecord -> ExceptionCode \
		     == STATUS_ACCESS_VIOLATION)
#     define CODE_OK (exc_info -> ExceptionRecord -> ExceptionInformation[0] \
		      == 1) /* Write fault */
#   endif    

# if defined(MSWIN32) || defined(MSWINCE)
    LONG WINAPI GC_write_fault_handler(struct _EXCEPTION_POINTERS *exc_info)
# else
#   include <ucontext.h>
    /*ARGSUSED*/
    void GC_write_fault_handler(int sig, siginfo_t *si, void *raw_sc)
# endif /* MSWIN32 || MSWINCE */
{
#   if !defined(MSWIN32) && !defined(MSWINCE)
      int code = si -> si_code;  /* Ignore gcc unused var. warning. */
      ucontext_t * scp = (ucontext_t *)raw_sc;
      				/* Ignore gcc unused var. warning. */
      char *addr = si -> si_addr;
#   endif
#   if defined(MSWIN32) || defined(MSWINCE)
	char * addr = (char *) (exc_info -> ExceptionRecord
				-> ExceptionInformation[1]);
#	define sig SIGSEGV
#   endif
    unsigned i;
    
    if (SIG_OK && CODE_OK) {
        register struct hblk * h =
        		(struct hblk *)((word)addr & ~(GC_page_size-1));
        GC_bool in_allocd_block;
        
#	ifdef SUNOS5SIGS
	    /* Address is only within the correct physical page.	*/
	    in_allocd_block = FALSE;
            for (i = 0; i < divHBLKSZ(GC_page_size); i++) {
              if (HDR(h+i) != 0) {
                in_allocd_block = TRUE;
              }
            }
#	else
	    in_allocd_block = (HDR(addr) != 0);
#	endif
        if (!in_allocd_block) {
	    /* FIXME - We should make sure that we invoke the	*/
	    /* old handler with the appropriate calling 	*/
	    /* sequence, which often depends on SA_SIGINFO.	*/

	    /* Heap blocks now begin and end on page boundaries */
            SIG_HNDLR_PTR old_handler;
	    GC_bool used_si;
            
            if (sig == SIGSEGV) {
            	old_handler = GC_old_segv_handler;
		used_si = GC_old_segv_handler_used_si;
            } else {
                old_handler = GC_old_bus_handler;
		used_si = GC_old_bus_handler_used_si;
            }
            if (old_handler == (SIG_HNDLR_PTR)SIG_DFL) {
#		if !defined(MSWIN32) && !defined(MSWINCE)
		    GC_err_printf("Segfault at %p\n", addr);
                    ABORT("Unexpected bus error or segmentation fault");
#		else
		    return(EXCEPTION_CONTINUE_SEARCH);
#		endif
            } else {
                /*
                 * FIXME: This code should probably check if the 
                 * old signal handler used the traditional style and
                 * if so call it using that style.
                 */
#		ifdef MSWIN32
		    return((*old_handler)(exc_info));
#		else
		    if (used_si)
		      ((SIG_HNDLR_PTR)old_handler) (sig, si, raw_sc);
		    else
		      /* FIXME: should pass nonstandard args as well. */
		      ((PLAIN_HNDLR_PTR)old_handler) (sig);
		    return;
#		endif
            }
        }
        UNPROTECT(h, GC_page_size);
	/* We need to make sure that no collection occurs between	*/
	/* the UNPROTECT and the setting of the dirty bit.  Otherwise	*/
	/* a write by a third thread might go unnoticed.  Reversing	*/
	/* the order is just as bad, since we would end up unprotecting	*/
	/* a page in a GC cycle during which it's not marked.		*/
	/* Currently we do this by disabling the thread stopping	*/
	/* signals while this handler is running.  An alternative might	*/
	/* be to record the fact that we're about to unprotect, or	*/
	/* have just unprotected a page in the GC's thread structure,	*/
	/* and then to have the thread stopping code set the dirty	*/
	/* flag, if necessary.						*/
        for (i = 0; i < divHBLKSZ(GC_page_size); i++) {
            size_t index = PHT_HASH(h+i);
            
            async_set_pht_entry_from_index(GC_dirty_pages, index);
        }
    	/* The write may not take place before dirty bits are read.	*/
    	/* But then we'll fault again ...				*/
#	if defined(MSWIN32) || defined(MSWINCE)
	    return(EXCEPTION_CONTINUE_EXECUTION);
#	else
	    return;
#	endif
    }
#if defined(MSWIN32) || defined(MSWINCE)
    return EXCEPTION_CONTINUE_SEARCH;
#else
    GC_err_printf("Segfault at %p\n", addr);
    ABORT("Unexpected bus error or segmentation fault");
#endif
}
#endif /* !DARWIN */

/*
 * We hold the allocation lock.  We expect block h to be written
 * shortly.  Ensure that all pages containing any part of the n hblks
 * starting at h are no longer protected.  If is_ptrfree is false,
 * also ensure that they will subsequently appear to be dirty.
 */
void GC_remove_protection(struct hblk *h, word nblocks, GC_bool is_ptrfree)
{
    struct hblk * h_trunc;  /* Truncated to page boundary */
    struct hblk * h_end;    /* Page boundary following block end */
    struct hblk * current;
    GC_bool found_clean;
    
#   if defined(GWW_VDB)
      if (GC_GWW_AVAILABLE()) return;
#   endif
    if (!GC_dirty_maintained) return;
    h_trunc = (struct hblk *)((word)h & ~(GC_page_size-1));
    h_end = (struct hblk *)(((word)(h + nblocks) + GC_page_size-1)
	                    & ~(GC_page_size-1));
    found_clean = FALSE;
    for (current = h_trunc; current < h_end; ++current) {
        size_t index = PHT_HASH(current);
            
        if (!is_ptrfree || current < h || current >= h + nblocks) {
            async_set_pht_entry_from_index(GC_dirty_pages, index);
        }
    }
    UNPROTECT(h_trunc, (ptr_t)h_end - (ptr_t)h_trunc);
}

#if !defined(DARWIN)
void GC_dirty_init(void)
{
#   if !defined(MSWIN32) && !defined(MSWINCE)
      struct sigaction	act, oldact;
      act.sa_flags	= SA_RESTART | SA_SIGINFO;
      act.sa_sigaction = GC_write_fault_handler;
      (void)sigemptyset(&act.sa_mask);
#     ifdef SIG_SUSPEND
        /* Arrange to postpone SIG_SUSPEND while we're in a write fault	*/
        /* handler.  This effectively makes the handler atomic w.r.t.	*/
        /* stopping the world for GC.					*/
        (void)sigaddset(&act.sa_mask, SIG_SUSPEND);
#     endif /* SIG_SUSPEND */
#   endif
    if (GC_print_stats == VERBOSE)
	GC_log_printf(
		"Initializing mprotect virtual dirty bit implementation\n");
    GC_dirty_maintained = TRUE;
    if (GC_page_size % HBLKSIZE != 0) {
        GC_err_printf("Page size not multiple of HBLKSIZE\n");
        ABORT("Page size not multiple of HBLKSIZE");
    }
#   if !defined(MSWIN32) && !defined(MSWINCE)
#     if defined(GC_IRIX_THREADS)
      	sigaction(SIGSEGV, 0, &oldact);
      	sigaction(SIGSEGV, &act, 0);
#     else 
	{
	  int res = sigaction(SIGSEGV, &act, &oldact);
	  if (res != 0) ABORT("Sigaction failed");
 	}
#     endif
      if (oldact.sa_flags & SA_SIGINFO) {
        GC_old_segv_handler = oldact.sa_sigaction;
	GC_old_segv_handler_used_si = TRUE;
      } else {
        GC_old_segv_handler = (SIG_HNDLR_PTR)oldact.sa_handler;
	GC_old_segv_handler_used_si = FALSE;
      }
      if (GC_old_segv_handler == (SIG_HNDLR_PTR)SIG_IGN) {
	GC_err_printf("Previously ignored segmentation violation!?");
	GC_old_segv_handler = (SIG_HNDLR_PTR)SIG_DFL;
      }
      if (GC_old_segv_handler != (SIG_HNDLR_PTR)SIG_DFL) {
	if (GC_print_stats == VERBOSE)
	  GC_log_printf("Replaced other SIGSEGV handler\n");
      }
#   endif /* ! MS windows */
#   if defined(HPUX) || defined(LINUX) || defined(HURD) \
      || (defined(FREEBSD) && defined(SUNOS5SIGS))
      sigaction(SIGBUS, &act, &oldact);
      if (oldact.sa_flags & SA_SIGINFO) {
        GC_old_bus_handler = oldact.sa_sigaction;
	GC_old_bus_handler_used_si = TRUE;
      } else {
        GC_old_bus_handler = (SIG_HNDLR_PTR)oldact.sa_handler;
	GC_old_bus_handler_used_si = FALSE;
      }
      if (GC_old_bus_handler == (SIG_HNDLR_PTR)SIG_IGN) {
	     GC_err_printf("Previously ignored bus error!?");
	     GC_old_bus_handler = (SIG_HNDLR_PTR)SIG_DFL;
      }
      if (GC_old_bus_handler != (SIG_HNDLR_PTR)SIG_DFL) {
	if (GC_print_stats == VERBOSE)
	  GC_log_printf("Replaced other SIGBUS handler\n");
      }
#   endif /* HPUX || LINUX || HURD || (FREEBSD && SUNOS5SIGS) */
#   if defined(MSWIN32)
#     if defined(GWW_VDB)
        if (GC_gww_dirty_init())
          return;
#     endif
      GC_old_segv_handler = SetUnhandledExceptionFilter(GC_write_fault_handler);
      if (GC_old_segv_handler != NULL) {
	if (GC_print_stats)
          GC_log_printf("Replaced other UnhandledExceptionFilter\n");
      } else {
          GC_old_segv_handler = SIG_DFL;
      }
#   endif
}
#endif /* !DARWIN */

int GC_incremental_protection_needs(void)
{
    if (GC_page_size == HBLKSIZE) {
	return GC_PROTECTS_POINTER_HEAP;
    } else {
	return GC_PROTECTS_POINTER_HEAP | GC_PROTECTS_PTRFREE_HEAP;
    }
}

#define HAVE_INCREMENTAL_PROTECTION_NEEDS

#define IS_PTRFREE(hhdr) ((hhdr)->hb_descr == 0)

#define PAGE_ALIGNED(x) !((word)(x) & (GC_page_size - 1))
void GC_protect_heap(void)
{
    ptr_t start;
    size_t len;
    struct hblk * current;
    struct hblk * current_start;  /* Start of block to be protected. */
    struct hblk * limit;
    unsigned i;
    GC_bool protect_all = 
	  (0 != (GC_incremental_protection_needs() & GC_PROTECTS_PTRFREE_HEAP));
    for (i = 0; i < GC_n_heap_sects; i++) {
        start = GC_heap_sects[i].hs_start;
        len = GC_heap_sects[i].hs_bytes;
	if (protect_all) {
          PROTECT(start, len);
	} else {
	  GC_ASSERT(PAGE_ALIGNED(len))
	  GC_ASSERT(PAGE_ALIGNED(start))
	  current_start = current = (struct hblk *)start;
	  limit = (struct hblk *)(start + len);
	  while (current < limit) {
            hdr * hhdr;
	    word nhblks;
	    GC_bool is_ptrfree;

	    GC_ASSERT(PAGE_ALIGNED(current));
	    GET_HDR(current, hhdr);
	    if (IS_FORWARDING_ADDR_OR_NIL(hhdr)) {
	      /* This can happen only if we're at the beginning of a 	*/
	      /* heap segment, and a block spans heap segments.		*/
	      /* We will handle that block as part of the preceding	*/
	      /* segment.						*/
	      GC_ASSERT(current_start == current);
	      current_start = ++current;
	      continue;
	    }
	    if (HBLK_IS_FREE(hhdr)) {
	      GC_ASSERT(PAGE_ALIGNED(hhdr -> hb_sz));
	      nhblks = divHBLKSZ(hhdr -> hb_sz);
	      is_ptrfree = TRUE;	/* dirty on alloc */
	    } else {
	      nhblks = OBJ_SZ_TO_BLOCKS(hhdr -> hb_sz);
	      is_ptrfree = IS_PTRFREE(hhdr);
	    }
	    if (is_ptrfree) {
	      if (current_start < current) {
		PROTECT(current_start, (ptr_t)current - (ptr_t)current_start);
	      }
	      current_start = (current += nhblks);
	    } else {
	      current += nhblks;
	    }
	  } 
	  if (current_start < current) {
	    PROTECT(current_start, (ptr_t)current - (ptr_t)current_start);
	  }
	}
    }
}

/* We assume that either the world is stopped or its OK to lose dirty	*/
/* bits while this is happenning (as in GC_enable_incremental).		*/
void GC_read_dirty(void)
{
#   if defined(GWW_VDB)
      if (GC_GWW_AVAILABLE()) {
        GC_gww_read_dirty();
        return;
      }
#   endif
    BCOPY((word *)GC_dirty_pages, GC_grungy_pages,
          (sizeof GC_dirty_pages));
    BZERO((word *)GC_dirty_pages, (sizeof GC_dirty_pages));
    GC_protect_heap();
}

GC_bool GC_page_was_dirty(struct hblk *h)
{
    register word index;
    
#   if defined(GWW_VDB)
      if (GC_GWW_AVAILABLE())
        return GC_gww_page_was_dirty(h);
#   endif

    index = PHT_HASH(h);
    return(HDR(h) == 0 || get_pht_entry_from_index(GC_grungy_pages, index));
}

/*
 * Acquiring the allocation lock here is dangerous, since this
 * can be called from within GC_call_with_alloc_lock, and the cord
 * package does so.  On systems that allow nested lock acquisition, this
 * happens to work.
 * On other systems, SET_LOCK_HOLDER and friends must be suitably defined.
 */

static GC_bool syscall_acquired_lock = FALSE;	/* Protected by GC lock. */
 
#if 0
void GC_begin_syscall(void)
{
    /* FIXME: Resurrecting this code would require fixing the	*/
    /* test, which can spuriously return TRUE.			*/
    if (!I_HOLD_LOCK()) {
	LOCK();
	syscall_acquired_lock = TRUE;
    }
}

void GC_end_syscall(void)
{
    if (syscall_acquired_lock) {
	syscall_acquired_lock = FALSE;
	UNLOCK();
    }
}

void GC_unprotect_range(ptr_t addr, word len)
{
    struct hblk * start_block;
    struct hblk * end_block;
    register struct hblk *h;
    ptr_t obj_start;
    
    if (!GC_dirty_maintained) return;
    obj_start = GC_base(addr);
    if (obj_start == 0) return;
    if (GC_base(addr + len - 1) != obj_start) {
        ABORT("GC_unprotect_range(range bigger than object)");
    }
    start_block = (struct hblk *)((word)addr & ~(GC_page_size - 1));
    end_block = (struct hblk *)((word)(addr + len - 1) & ~(GC_page_size - 1));
    end_block += GC_page_size/HBLKSIZE - 1;
    for (h = start_block; h <= end_block; h++) {
        register word index = PHT_HASH(h);
        
        async_set_pht_entry_from_index(GC_dirty_pages, index);
    }
    UNPROTECT(start_block,
    	      ((ptr_t)end_block - (ptr_t)start_block) + HBLKSIZE);
}


/* We no longer wrap read by default, since that was causing too many	*/
/* problems.  It is preferred that the client instead avoids writing	*/
/* to the write-protected heap with a system call.			*/
/* This still serves as sample code if you do want to wrap system calls.*/

#if !defined(MSWIN32) && !defined(MSWINCE) && !defined(GC_USE_LD_WRAP)
/* Replacement for UNIX system call.					  */
/* Other calls that write to the heap should be handled similarly.	  */
/* Note that this doesn't work well for blocking reads:  It will hold	  */
/* the allocation lock for the entire duration of the call. Multithreaded */
/* clients should really ensure that it won't block, either by setting 	  */
/* the descriptor nonblocking, or by calling select or poll first, to	  */
/* make sure that input is available.					  */
/* Another, preferred alternative is to ensure that system calls never 	  */
/* write to the protected heap (see above).				  */
# include <unistd.h>
# include <sys/uio.h>
ssize_t read(int fd, void *buf, size_t nbyte)
{
    int result;
    
    GC_begin_syscall();
    GC_unprotect_range(buf, (word)nbyte);
#   if defined(IRIX5) || defined(GC_LINUX_THREADS)
	/* Indirect system call may not always be easily available.	*/
	/* We could call _read, but that would interfere with the	*/
	/* libpthread interception of read.				*/
	/* On Linux, we have to be careful with the linuxthreads	*/
	/* read interception.						*/
	{
	    struct iovec iov;

	    iov.iov_base = buf;
	    iov.iov_len = nbyte;
	    result = readv(fd, &iov, 1);
	}
#   else
#     if defined(HURD)	
	result = __read(fd, buf, nbyte);
#     else
 	/* The two zero args at the end of this list are because one
 	   IA-64 syscall() implementation actually requires six args
 	   to be passed, even though they aren't always used. */
     	result = syscall(SYS_read, fd, buf, nbyte, 0, 0);
#     endif /* !HURD */
#   endif
    GC_end_syscall();
    return(result);
}
#endif /* !MSWIN32 && !MSWINCE && !GC_LINUX_THREADS */

#if defined(GC_USE_LD_WRAP) && !defined(THREADS)
    /* We use the GNU ld call wrapping facility.			*/
    /* This requires that the linker be invoked with "--wrap read".	*/
    /* This can be done by passing -Wl,"--wrap read" to gcc.		*/
    /* I'm not sure that this actually wraps whatever version of read	*/
    /* is called by stdio.  That code also mentions __read.		*/
#   include <unistd.h>
    ssize_t __wrap_read(int fd, void *buf, size_t nbyte)
    {
 	int result;

	GC_begin_syscall();
    	GC_unprotect_range(buf, (word)nbyte);
	result = __real_read(fd, buf, nbyte);
	GC_end_syscall();
	return(result);
    }

    /* We should probably also do this for __read, or whatever stdio	*/
    /* actually calls.							*/
#endif

#endif /* 0 */

/*ARGSUSED*/
GC_bool GC_page_was_ever_dirty(struct hblk *h)
{
#   if defined(GWW_VDB)
      if (GC_GWW_AVAILABLE())
        return GC_gww_page_was_ever_dirty(h);
#   endif
    return(TRUE);
}

# endif /* MPROTECT_VDB */

# ifdef PROC_VDB

/*
 * See DEFAULT_VDB for interface descriptions.
 */
 
/*
 * This implementaion assumes a Solaris 2.X like /proc pseudo-file-system
 * from which we can read page modified bits.  This facility is far from
 * optimal (e.g. we would like to get the info for only some of the
 * address space), but it avoids intercepting system calls.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/procfs.h>
#include <sys/stat.h>

#define INITIAL_BUF_SZ 16384
word GC_proc_buf_size = INITIAL_BUF_SZ;
char *GC_proc_buf;

int GC_proc_fd;

void GC_dirty_init(void)
{
    int fd;
    char buf[30];

    GC_dirty_maintained = TRUE;
    if (GC_bytes_allocd != 0 || GC_bytes_allocd_before_gc != 0) {
    	register int i;
    
        for (i = 0; i < PHT_SIZE; i++) GC_written_pages[i] = (word)(-1);
	if (GC_print_stats == VERBOSE)
	    GC_log_printf(
		      "Allocated bytes:%lu:all pages may have been written\n",
	    	      (unsigned long)
	    	      		(GC_bytes_allocd + GC_bytes_allocd_before_gc));
    }
    sprintf(buf, "/proc/%d", getpid());
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
    	ABORT("/proc open failed");
    }
    GC_proc_fd = syscall(SYS_ioctl, fd, PIOCOPENPD, 0);
    close(fd);
    syscall(SYS_fcntl, GC_proc_fd, F_SETFD, FD_CLOEXEC);
    if (GC_proc_fd < 0) {
    	ABORT("/proc ioctl failed");
    }
    GC_proc_buf = GC_scratch_alloc(GC_proc_buf_size);
}

/* Ignore write hints. They don't help us here.	*/
/*ARGSUSED*/
void GC_remove_protection(h, nblocks, is_ptrfree)
struct hblk *h;
word nblocks;
GC_bool is_ptrfree;
{
}

# define READ(fd,buf,nbytes) read(fd, buf, nbytes)

void GC_read_dirty(void)
{
    unsigned long ps, np;
    int nmaps;
    ptr_t vaddr;
    struct prasmap * map;
    char * bufp;
    ptr_t current_addr, limit;
    int i;

    BZERO(GC_grungy_pages, (sizeof GC_grungy_pages));
    
    bufp = GC_proc_buf;
    if (READ(GC_proc_fd, bufp, GC_proc_buf_size) <= 0) {
	if (GC_print_stats)
            GC_log_printf("/proc read failed: GC_proc_buf_size = %lu\n",
            	          (unsigned long)GC_proc_buf_size);
        {
            /* Retry with larger buffer. */
            word new_size = 2 * GC_proc_buf_size;
            char * new_buf = GC_scratch_alloc(new_size);
            
            if (new_buf != 0) {
                GC_proc_buf = bufp = new_buf;
                GC_proc_buf_size = new_size;
            }
            if (READ(GC_proc_fd, bufp, GC_proc_buf_size) <= 0) {
                WARN("Insufficient space for /proc read\n", 0);
                /* Punt:	*/
        	memset(GC_grungy_pages, 0xff, sizeof (page_hash_table));
		memset(GC_written_pages, 0xff, sizeof(page_hash_table));
		return;
            }
        }
    }
    /* Copy dirty bits into GC_grungy_pages */
    	nmaps = ((struct prpageheader *)bufp) -> pr_nmap;
	/* printf( "nmaps = %d, PG_REFERENCED = %d, PG_MODIFIED = %d\n",
		     nmaps, PG_REFERENCED, PG_MODIFIED); */
	bufp = bufp + sizeof(struct prpageheader);
	for (i = 0; i < nmaps; i++) {
	    map = (struct prasmap *)bufp;
	    vaddr = (ptr_t)(map -> pr_vaddr);
	    ps = map -> pr_pagesize;
	    np = map -> pr_npage;
	    /* printf("vaddr = 0x%X, ps = 0x%X, np = 0x%X\n", vaddr, ps, np); */
	    limit = vaddr + ps * np;
	    bufp += sizeof (struct prasmap);
	    for (current_addr = vaddr;
	         current_addr < limit; current_addr += ps){
	        if ((*bufp++) & PG_MODIFIED) {
	            register struct hblk * h = (struct hblk *) current_addr;
	            
	            while ((ptr_t)h < current_addr + ps) {
	                register word index = PHT_HASH(h);
	                
	                set_pht_entry_from_index(GC_grungy_pages, index);
	                h++;
	            }
	        }
	    }
	    bufp += sizeof(long) - 1;
	    bufp = (char *)((unsigned long)bufp & ~(sizeof(long)-1));
	}
    /* Update GC_written_pages. */
        GC_or_pages(GC_written_pages, GC_grungy_pages);
}

#undef READ

GC_bool GC_page_was_dirty(struct hblk *h)
{
    register word index = PHT_HASH(h);
    register GC_bool result;
    
    result = get_pht_entry_from_index(GC_grungy_pages, index);
    return(result);
}

GC_bool GC_page_was_ever_dirty(struct hblk *h)
{
    register word index = PHT_HASH(h);
    register GC_bool result;
    
    result = get_pht_entry_from_index(GC_written_pages, index);
    return(result);
}

# endif /* PROC_VDB */


# ifdef PCR_VDB

# include "vd/PCR_VD.h"

# define NPAGES (32*1024)	/* 128 MB */

PCR_VD_DB  GC_grungy_bits[NPAGES];

ptr_t GC_vd_base;	/* Address corresponding to GC_grungy_bits[0]	*/
			/* HBLKSIZE aligned.				*/

void GC_dirty_init(void)
{
    GC_dirty_maintained = TRUE;
    /* For the time being, we assume the heap generally grows up */
    GC_vd_base = GC_heap_sects[0].hs_start;
    if (GC_vd_base == 0) {
   	ABORT("Bad initial heap segment");
    }
    if (PCR_VD_Start(HBLKSIZE, GC_vd_base, NPAGES*HBLKSIZE)
	!= PCR_ERes_okay) {
	ABORT("dirty bit initialization failed");
    }
}

void GC_read_dirty(void)
{
    /* lazily enable dirty bits on newly added heap sects */
    {
        static int onhs = 0;
        int nhs = GC_n_heap_sects;
        for( ; onhs < nhs; onhs++ ) {
            PCR_VD_WriteProtectEnable(
                    GC_heap_sects[onhs].hs_start,
                    GC_heap_sects[onhs].hs_bytes );
        }
    }


    if (PCR_VD_Clear(GC_vd_base, NPAGES*HBLKSIZE, GC_grungy_bits)
        != PCR_ERes_okay) {
	ABORT("dirty bit read failed");
    }
}

GC_bool GC_page_was_dirty(struct hblk *h)
{
    if((ptr_t)h < GC_vd_base || (ptr_t)h >= GC_vd_base + NPAGES*HBLKSIZE) {
	return(TRUE);
    }
    return(GC_grungy_bits[h - (struct hblk *)GC_vd_base] & PCR_VD_DB_dirtyBit);
}

/*ARGSUSED*/
void GC_remove_protection(struct hblk *h, word nblocks, GC_bool is_ptrfree)
{
    PCR_VD_WriteProtectDisable(h, nblocks*HBLKSIZE);
    PCR_VD_WriteProtectEnable(h, nblocks*HBLKSIZE);
}

# endif /* PCR_VDB */

#if defined(MPROTECT_VDB) && defined(DARWIN)
/* The following sources were used as a *reference* for this exception handling
   code:
      1. Apple's mach/xnu documentation
      2. Timothy J. Wood's "Mach Exception Handlers 101" post to the
         omnigroup's macosx-dev list.
         www.omnigroup.com/mailman/archive/macosx-dev/2000-June/014178.html
      3. macosx-nat.c from Apple's GDB source code.
*/

/* The bug that caused all this trouble should now be fixed. This should
   eventually be removed if all goes well. */

/* #define BROKEN_EXCEPTION_HANDLING */

#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/thread_status.h>
#include <mach/exception.h>
#include <mach/task.h>
#include <pthread.h>

extern void GC_darwin_register_mach_handler_thread(mach_port_t);

/* These are not defined in any header, although they are documented */
extern boolean_t
exc_server(mach_msg_header_t *, mach_msg_header_t *);

extern kern_return_t
exception_raise(mach_port_t, mach_port_t, mach_port_t, exception_type_t,
		exception_data_t, mach_msg_type_number_t);

extern kern_return_t
exception_raise_state(mach_port_t, mach_port_t, mach_port_t, exception_type_t,
		      exception_data_t, mach_msg_type_number_t,
		      thread_state_flavor_t*, thread_state_t,
		      mach_msg_type_number_t, thread_state_t,
		      mach_msg_type_number_t*);

extern kern_return_t
exception_raise_state_identity(mach_port_t, mach_port_t, mach_port_t,
			       exception_type_t, exception_data_t,
			       mach_msg_type_number_t, thread_state_flavor_t*,
			       thread_state_t, mach_msg_type_number_t,
			       thread_state_t, mach_msg_type_number_t*);


#define MAX_EXCEPTION_PORTS 16

static struct {
    mach_msg_type_number_t count;
    exception_mask_t      masks[MAX_EXCEPTION_PORTS];
    exception_handler_t   ports[MAX_EXCEPTION_PORTS];
    exception_behavior_t  behaviors[MAX_EXCEPTION_PORTS];
    thread_state_flavor_t flavors[MAX_EXCEPTION_PORTS];
} GC_old_exc_ports;

static struct {
    mach_port_t exception;
#if defined(THREADS)
    mach_port_t reply;
#endif
} GC_ports;

typedef struct {
    mach_msg_header_t head;
} GC_msg_t;

typedef enum {
    GC_MP_NORMAL, GC_MP_DISCARDING, GC_MP_STOPPED
} GC_mprotect_state_t;

/* FIXME: 1 and 2 seem to be safe to use in the msgh_id field,
   but it isn't  documented. Use the source and see if they
   should be ok. */
#define ID_STOP 1
#define ID_RESUME 2

/* These values are only used on the reply port */
#define ID_ACK 3

#if defined(THREADS)

GC_mprotect_state_t GC_mprotect_state;

/* The following should ONLY be called when the world is stopped  */
static void GC_mprotect_thread_notify(mach_msg_id_t id)
{

  struct {
    GC_msg_t msg;
    mach_msg_trailer_t trailer;
  } buf;

  mach_msg_return_t r;
  /* remote, local */
  buf.msg.head.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND, 0);
  buf.msg.head.msgh_size = sizeof(buf.msg);
  buf.msg.head.msgh_remote_port = GC_ports.exception;
  buf.msg.head.msgh_local_port = MACH_PORT_NULL;
  buf.msg.head.msgh_id = id;

  r = mach_msg(&buf.msg.head, MACH_SEND_MSG | MACH_RCV_MSG | MACH_RCV_LARGE,
	       sizeof(buf.msg), sizeof(buf), GC_ports.reply,
	       MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
  if(r != MACH_MSG_SUCCESS)
    ABORT("mach_msg failed in GC_mprotect_thread_notify");
  if(buf.msg.head.msgh_id != ID_ACK)
    ABORT("invalid ack in GC_mprotect_thread_notify");
}

/* Should only be called by the mprotect thread */
static void GC_mprotect_thread_reply(void)
{

  GC_msg_t msg;
  mach_msg_return_t r;
  /* remote, local */
  msg.head.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND, 0);
  msg.head.msgh_size = sizeof(msg);
  msg.head.msgh_remote_port = GC_ports.reply;
  msg.head.msgh_local_port = MACH_PORT_NULL;
  msg.head.msgh_id = ID_ACK;

  r = mach_msg(&msg.head, MACH_SEND_MSG, sizeof(msg), 0, MACH_PORT_NULL,
	       MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
  if(r != MACH_MSG_SUCCESS)
    ABORT("mach_msg failed in GC_mprotect_thread_reply");
}

void GC_mprotect_stop(void)
{
  GC_mprotect_thread_notify(ID_STOP);
}
void GC_mprotect_resume(void)
{
  GC_mprotect_thread_notify(ID_RESUME);
}

#else /* !THREADS */
/* The compiler should optimize away any GC_mprotect_state computations */
#define GC_mprotect_state GC_MP_NORMAL
#endif

static void *GC_mprotect_thread(void *arg)
{
  mach_msg_return_t r;
  /* These two structures contain some private kernel data. We don't need to
     access any of it so we don't bother defining a proper struct. The
     correct definitions are in the xnu source code. */
  struct {
    mach_msg_header_t head;
    char data[256];
  } reply;
  struct {
    mach_msg_header_t head;
    mach_msg_body_t msgh_body;
    char data[1024];
  } msg;

  mach_msg_id_t id;

  GC_darwin_register_mach_handler_thread(mach_thread_self());

  for(;;) {
    r = mach_msg(&msg.head, MACH_RCV_MSG | MACH_RCV_LARGE |
		 (GC_mprotect_state == GC_MP_DISCARDING ? MACH_RCV_TIMEOUT : 0),
		 0, sizeof(msg), GC_ports.exception,
		 GC_mprotect_state == GC_MP_DISCARDING ? 0
		 : MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

    id = r == MACH_MSG_SUCCESS ? msg.head.msgh_id : -1;

#   if defined(THREADS)
      if(GC_mprotect_state == GC_MP_DISCARDING) {
	if(r == MACH_RCV_TIMED_OUT) {
	  GC_mprotect_state = GC_MP_STOPPED;
	  GC_mprotect_thread_reply();
	  continue;
	}
	if(r == MACH_MSG_SUCCESS && (id == ID_STOP || id == ID_RESUME))
	  ABORT("out of order mprotect thread request");
      }
#   endif /* THREADS */

    if(r != MACH_MSG_SUCCESS) {
      GC_err_printf("mach_msg failed with %d %s\n", (int)r,
		    mach_error_string(r));
      ABORT("mach_msg failed");
    }

    switch(id) {
#     if defined(THREADS)
        case ID_STOP:
	  if(GC_mprotect_state != GC_MP_NORMAL)
	    ABORT("Called mprotect_stop when state wasn't normal");
	  GC_mprotect_state = GC_MP_DISCARDING;
	  break;
        case ID_RESUME:
	  if(GC_mprotect_state != GC_MP_STOPPED)
	    ABORT("Called mprotect_resume when state wasn't stopped");
	  GC_mprotect_state = GC_MP_NORMAL;
	  GC_mprotect_thread_reply();
	  break;
#     endif /* THREADS */
        default:
	  /* Handle the message (calls catch_exception_raise) */
	  if(!exc_server(&msg.head, &reply.head))
	    ABORT("exc_server failed");
	  /* Send the reply */
	  r = mach_msg(&reply.head, MACH_SEND_MSG, reply.head.msgh_size, 0,
		       MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
		       MACH_PORT_NULL);
	  if(r != MACH_MSG_SUCCESS) {
	    /* This will fail if the thread dies, but the thread */
	    /* shouldn't die... */
#           ifdef BROKEN_EXCEPTION_HANDLING
	      GC_err_printf("mach_msg failed with %d %s while sending"
			    "exc reply\n", (int)r,mach_error_string(r));
#           else
	      ABORT("mach_msg failed while sending exception reply");
#           endif
	  }
    } /* switch */
  } /* for(;;) */
    /* NOT REACHED */
  return NULL;
}

/* All this SIGBUS code shouldn't be necessary. All protection faults should
   be going throught the mach exception handler. However, it seems a SIGBUS is
   occasionally sent for some unknown reason. Even more odd, it seems to be
   meaningless and safe to ignore. */
#ifdef BROKEN_EXCEPTION_HANDLING

static SIG_HNDLR_PTR GC_old_bus_handler;

/* Updates to this aren't atomic, but the SIGBUSs seem pretty rare.
   Even if this doesn't get updated property, it isn't really a problem */
static int GC_sigbus_count;

static void GC_darwin_sigbus(int num, siginfo_t *sip, void *context)
{
  if(num != SIGBUS)
    ABORT("Got a non-sigbus signal in the sigbus handler");

  /* Ugh... some seem safe to ignore, but too many in a row probably means
     trouble. GC_sigbus_count is reset for each mach exception that is
     handled */
  if(GC_sigbus_count >= 8) {
    ABORT("Got more than 8 SIGBUSs in a row!");
  } else {
    GC_sigbus_count++;
    WARN("Ignoring SIGBUS.\n", 0);
  }
}
#endif /* BROKEN_EXCEPTION_HANDLING */

void GC_dirty_init(void)
{
  kern_return_t r;
  mach_port_t me;
  pthread_t thread;
  pthread_attr_t attr;
  exception_mask_t mask;

  if (GC_print_stats == VERBOSE)
    GC_log_printf("Inititalizing mach/darwin mprotect virtual dirty bit "
		  "implementation\n");
# ifdef BROKEN_EXCEPTION_HANDLING
    WARN("Enabling workarounds for various darwin "
	 "exception handling bugs.\n", 0);
# endif
  GC_dirty_maintained = TRUE;
  if (GC_page_size % HBLKSIZE != 0) {
    GC_err_printf("Page size not multiple of HBLKSIZE\n");
    ABORT("Page size not multiple of HBLKSIZE");
  }

  GC_task_self = me = mach_task_self();

  r = mach_port_allocate(me, MACH_PORT_RIGHT_RECEIVE, &GC_ports.exception);
  if(r != KERN_SUCCESS)
    ABORT("mach_port_allocate failed (exception port)");

  r = mach_port_insert_right(me, GC_ports.exception, GC_ports.exception,
			     MACH_MSG_TYPE_MAKE_SEND);
  if(r != KERN_SUCCESS)
    ABORT("mach_port_insert_right failed (exception port)");

#  if defined(THREADS)
     r = mach_port_allocate(me, MACH_PORT_RIGHT_RECEIVE, &GC_ports.reply);
     if(r != KERN_SUCCESS)
       ABORT("mach_port_allocate failed (reply port)");
#  endif

  /* The exceptions we want to catch */
  mask = EXC_MASK_BAD_ACCESS;

  r = task_get_exception_ports(me, mask, GC_old_exc_ports.masks,
			       &GC_old_exc_ports.count, GC_old_exc_ports.ports,
			       GC_old_exc_ports.behaviors,
			       GC_old_exc_ports.flavors);
  if(r != KERN_SUCCESS)
    ABORT("task_get_exception_ports failed");

  r = task_set_exception_ports(me, mask, GC_ports.exception, EXCEPTION_DEFAULT,
			       GC_MACH_THREAD_STATE);
  if(r != KERN_SUCCESS)
    ABORT("task_set_exception_ports failed");
  if(pthread_attr_init(&attr) != 0)
    ABORT("pthread_attr_init failed");
  if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
    ABORT("pthread_attr_setdetachedstate failed");

# undef pthread_create
  /* This will call the real pthread function, not our wrapper */
  if(pthread_create(&thread, &attr, GC_mprotect_thread, NULL) != 0)
    ABORT("pthread_create failed");
  pthread_attr_destroy(&attr);

  /* Setup the sigbus handler for ignoring the meaningless SIGBUSs */
# ifdef BROKEN_EXCEPTION_HANDLING
    {
      struct sigaction sa, oldsa;
      sa.sa_handler = (SIG_HNDLR_PTR)GC_darwin_sigbus;
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = SA_RESTART|SA_SIGINFO;
      if(sigaction(SIGBUS, &sa, &oldsa) < 0)
	ABORT("sigaction");
      GC_old_bus_handler = (SIG_HNDLR_PTR)oldsa.sa_handler;
      if (GC_old_bus_handler != SIG_DFL) {
	if (GC_print_stats == VERBOSE)
	  GC_err_printf("Replaced other SIGBUS handler\n");
      }
    }
#  endif /* BROKEN_EXCEPTION_HANDLING  */
}

/* The source code for Apple's GDB was used as a reference for the exception
   forwarding code. This code is similar to be GDB code only because there is
   only one way to do it. */
static kern_return_t GC_forward_exception(mach_port_t thread, mach_port_t task,
					  exception_type_t exception,
					  exception_data_t data,
					  mach_msg_type_number_t data_count)
{
  unsigned int i;
  kern_return_t r;
  mach_port_t port;
  exception_behavior_t behavior;
  thread_state_flavor_t flavor;

  thread_state_t thread_state = NULL;
  mach_msg_type_number_t thread_state_count = THREAD_STATE_MAX;

  for(i=0; i < GC_old_exc_ports.count; i++)
    if(GC_old_exc_ports.masks[i] & (1 << exception))
      break;
  if(i==GC_old_exc_ports.count)
    ABORT("No handler for exception!");

  port = GC_old_exc_ports.ports[i];
  behavior = GC_old_exc_ports.behaviors[i];
  flavor = GC_old_exc_ports.flavors[i];

  if(behavior != EXCEPTION_DEFAULT) {
    r = thread_get_state(thread, flavor, thread_state, &thread_state_count);
    if(r != KERN_SUCCESS)
      ABORT("thread_get_state failed in forward_exception");
    }

  switch(behavior) {
    case EXCEPTION_DEFAULT:
      r = exception_raise(port, thread, task, exception, data, data_count);
      break;
    case EXCEPTION_STATE:
      r = exception_raise_state(port, thread, task, exception, data, data_count,
				&flavor, thread_state, thread_state_count,
				thread_state, &thread_state_count);
      break;
    case EXCEPTION_STATE_IDENTITY:
      r = exception_raise_state_identity(port, thread, task, exception, data,
					 data_count, &flavor, thread_state,
					 thread_state_count, thread_state,
					 &thread_state_count);
      break;
    default:
      r = KERN_FAILURE; /* make gcc happy */
      ABORT("forward_exception: unknown behavior");
      break;
  }

  if(behavior != EXCEPTION_DEFAULT) {
    r = thread_set_state(thread, flavor, thread_state, thread_state_count);
    if(r != KERN_SUCCESS)
      ABORT("thread_set_state failed in forward_exception");
  }

  return r;
}

#define FWD() GC_forward_exception(thread, task, exception, code, code_count)

/* This violates the namespace rules but there isn't anything that can be done
   about it. The exception handling stuff is hard coded to call this */
kern_return_t
catch_exception_raise(mach_port_t exception_port, mach_port_t thread,
		      mach_port_t task, exception_type_t exception,
		      exception_data_t code, mach_msg_type_number_t code_count)
{
  kern_return_t r;
  char *addr;
  struct hblk *h;
  unsigned int i;
# if defined(POWERPC)
#   if CPP_WORDSZ == 32
      thread_state_flavor_t flavor = PPC_EXCEPTION_STATE;
      mach_msg_type_number_t exc_state_count = PPC_EXCEPTION_STATE_COUNT;
      ppc_exception_state_t exc_state;
#   else
      thread_state_flavor_t flavor = PPC_EXCEPTION_STATE64;
      mach_msg_type_number_t exc_state_count = PPC_EXCEPTION_STATE64_COUNT;
      ppc_exception_state64_t exc_state;
#   endif
# elif defined(I386) || defined(X86_64)
#   if CPP_WORDSZ == 32
      thread_state_flavor_t flavor = x86_EXCEPTION_STATE32;
      mach_msg_type_number_t exc_state_count = x86_EXCEPTION_STATE32_COUNT;
      x86_exception_state32_t exc_state;
#   else
      thread_state_flavor_t flavor = x86_EXCEPTION_STATE64;
      mach_msg_type_number_t exc_state_count = x86_EXCEPTION_STATE64_COUNT;
      x86_exception_state64_t exc_state;
#   endif
# else
#   error FIXME for non-ppc/x86 darwin
# endif


  if(exception != EXC_BAD_ACCESS || code[0] != KERN_PROTECTION_FAILURE) {
#   ifdef DEBUG_EXCEPTION_HANDLING
      /* We aren't interested, pass it on to the old handler */
      GC_printf("Exception: 0x%x Code: 0x%x 0x%x in catch....\n", exception,
		code_count > 0 ? code[0] : -1, code_count > 1 ? code[1] : -1);
#   endif
    return FWD();
  }

  r = thread_get_state(thread, flavor, (natural_t*)&exc_state,
		       &exc_state_count);
  if(r != KERN_SUCCESS) {
    /* The thread is supposed to be suspended while the exception handler
       is called. This shouldn't fail. */
#   ifdef BROKEN_EXCEPTION_HANDLING
      GC_err_printf("thread_get_state failed in catch_exception_raise\n");
      return KERN_SUCCESS;
#   else
      ABORT("thread_get_state failed in catch_exception_raise");
#   endif
  }

    /* This is the address that caused the fault */
# if defined(POWERPC)
    addr = (char*) exc_state. THREAD_FLD(dar);
# elif defined (I386) || defined (X86_64)
    addr = (char*) exc_state. THREAD_FLD(faultvaddr);
# else
#   error FIXME for non POWERPC/I386
# endif

    if((HDR(addr)) == 0) {
      /* Ugh... just like the SIGBUS problem above, it seems we get a bogus
	 KERN_PROTECTION_FAILURE every once and a while. We wait till we get
	 a bunch in a row before doing anything about it. If a "real" fault
	 ever occurres it'll just keep faulting over and over and we'll hit
	 the limit pretty quickly. */
#     ifdef BROKEN_EXCEPTION_HANDLING
        static char *last_fault;
	static int last_fault_count;

	if(addr != last_fault) {
	  last_fault = addr;
	  last_fault_count = 0;
	}
	if(++last_fault_count < 32) {
	  if(last_fault_count == 1)
	    WARN("Ignoring KERN_PROTECTION_FAILURE at %lx\n", (GC_word)addr);
	  return KERN_SUCCESS;
	}

	GC_err_printf("Unexpected KERN_PROTECTION_FAILURE at %p\n",addr);
	/* Can't pass it along to the signal handler because that is
	   ignoring SIGBUS signals. We also shouldn't call ABORT here as
	   signals don't always work too well from the exception handler. */
	GC_err_printf("Aborting\n");
	exit(EXIT_FAILURE);
#     else /* BROKEN_EXCEPTION_HANDLING */
	/* Pass it along to the next exception handler
	   (which should call SIGBUS/SIGSEGV) */
	return FWD();
#     endif /* !BROKEN_EXCEPTION_HANDLING */
    }

#   ifdef BROKEN_EXCEPTION_HANDLING
      /* Reset the number of consecutive SIGBUSs */
      GC_sigbus_count = 0;
#   endif

    if(GC_mprotect_state == GC_MP_NORMAL) { /* common case */
      h = (struct hblk*)((word)addr & ~(GC_page_size-1));
      UNPROTECT(h, GC_page_size);
      for (i = 0; i < divHBLKSZ(GC_page_size); i++) {
	register int index = PHT_HASH(h+i);
	async_set_pht_entry_from_index(GC_dirty_pages, index);
      }
    } else if(GC_mprotect_state == GC_MP_DISCARDING) {
      /* Lie to the thread for now. No sense UNPROTECT()ing the memory
	 when we're just going to PROTECT() it again later. The thread
	 will just fault again once it resumes */
    } else {
      /* Shouldn't happen, i don't think */
      GC_printf("KERN_PROTECTION_FAILURE while world is stopped\n");
      return FWD();
    }
    return KERN_SUCCESS;
}
#undef FWD

/* These should never be called, but just in case...  */
kern_return_t
catch_exception_raise_state(mach_port_name_t exception_port, int exception,
			    exception_data_t code,
			    mach_msg_type_number_t codeCnt, int flavor,
			    thread_state_t old_state, int old_stateCnt,
			    thread_state_t new_state, int new_stateCnt)
{
  ABORT("catch_exception_raise_state");
  return(KERN_INVALID_ARGUMENT);
}

kern_return_t
catch_exception_raise_state_identity(mach_port_name_t exception_port,
				     mach_port_t thread, mach_port_t task,
				     int exception, exception_data_t code,
				     mach_msg_type_number_t codeCnt, int flavor,
				     thread_state_t old_state, int old_stateCnt,
				     thread_state_t new_state, int new_stateCnt)
{
  ABORT("catch_exception_raise_state_identity");
  return(KERN_INVALID_ARGUMENT);
}


#endif /* DARWIN && MPROTECT_VDB */

# ifndef HAVE_INCREMENTAL_PROTECTION_NEEDS
  int GC_incremental_protection_needs()
  {
    return GC_PROTECTS_NONE;
  }
# endif /* !HAVE_INCREMENTAL_PROTECTION_NEEDS */

/*
 * Call stack save code for debugging.
 * Should probably be in mach_dep.c, but that requires reorganization.
 */

/* I suspect the following works for most X86 *nix variants, so 	*/
/* long as the frame pointer is explicitly stored.  In the case of gcc,	*/
/* compiler flags (e.g. -fomit-frame-pointer) determine whether it is.	*/
#if defined(I386) && defined(LINUX) && defined(SAVE_CALL_CHAIN)
#   include <features.h>

    struct frame {
	struct frame *fr_savfp;
	long	fr_savpc;
        long	fr_arg[NARGS];  /* All the arguments go here.	*/
    };
#endif

#if defined(SPARC)
#  if defined(LINUX)
#    include <features.h>

     struct frame {
	long	fr_local[8];
	long	fr_arg[6];
	struct frame *fr_savfp;
	long	fr_savpc;
#       ifndef __arch64__
	  char	*fr_stret;
#       endif
	long	fr_argd[6];
	long	fr_argx[0];
     };
#  elif defined (DRSNX)
#    include <sys/sparc/frame.h>
#  elif defined(OPENBSD)
#    include <frame.h>
#  elif defined(FREEBSD) || defined(NETBSD)
#    include <machine/frame.h>
#  else
#    include <sys/frame.h>
#  endif
#  if NARGS > 6
#    error We only know how to to get the first 6 arguments
#  endif
#endif /* SPARC */

#ifdef  NEED_CALLINFO
/* Fill in the pc and argument information for up to NFRAMES of my	*/
/* callers.  Ignore my frame and my callers frame.			*/

#ifdef LINUX
#   include <unistd.h>
#endif

#endif /* NEED_CALLINFO */

#if defined(GC_HAVE_BUILTIN_BACKTRACE)
# ifdef _MSC_VER
#  include "private/msvc_dbg.h"
# else
#  include <execinfo.h>
# endif
#endif

#ifdef SAVE_CALL_CHAIN

#if NARGS == 0 && NFRAMES % 2 == 0 /* No padding */ \
    && defined(GC_HAVE_BUILTIN_BACKTRACE)

#ifdef REDIRECT_MALLOC
  /* Deal with possible malloc calls in backtrace by omitting	*/
  /* the infinitely recursing backtrace.			*/
# ifdef THREADS
    __thread 	/* If your compiler doesn't understand this */
    		/* you could use something like pthread_getspecific.	*/
# endif
  GC_in_save_callers = FALSE;
#endif

void GC_save_callers (struct callinfo info[NFRAMES]) 
{
  void * tmp_info[NFRAMES + 1];
  int npcs, i;
# define IGNORE_FRAMES 1
  
  /* We retrieve NFRAMES+1 pc values, but discard the first, since it	*/
  /* points to our own frame.						*/
# ifdef REDIRECT_MALLOC
    if (GC_in_save_callers) {
      info[0].ci_pc = (word)(&GC_save_callers);
      for (i = 1; i < NFRAMES; ++i) info[i].ci_pc = 0;
      return;
    }
    GC_in_save_callers = TRUE;
# endif
  GC_ASSERT(sizeof(struct callinfo) == sizeof(void *));
  npcs = backtrace((void **)tmp_info, NFRAMES + IGNORE_FRAMES);
  BCOPY(tmp_info+IGNORE_FRAMES, info, (npcs - IGNORE_FRAMES) * sizeof(void *));
  for (i = npcs - IGNORE_FRAMES; i < NFRAMES; ++i) info[i].ci_pc = 0;
# ifdef REDIRECT_MALLOC
    GC_in_save_callers = FALSE;
# endif
}

#else /* No builtin backtrace; do it ourselves */

#if (defined(OPENBSD) || defined(NETBSD) || defined(FREEBSD)) && defined(SPARC)
#  define FR_SAVFP fr_fp
#  define FR_SAVPC fr_pc
#else
#  define FR_SAVFP fr_savfp
#  define FR_SAVPC fr_savpc
#endif

#if defined(SPARC) && (defined(__arch64__) || defined(__sparcv9))
#   define BIAS 2047
#else
#   define BIAS 0
#endif

void GC_save_callers (struct callinfo info[NFRAMES]) 
{
  struct frame *frame;
  struct frame *fp;
  int nframes = 0;
# ifdef I386
    /* We assume this is turned on only with gcc as the compiler. */
    asm("movl %%ebp,%0" : "=r"(frame));
    fp = frame;
# else
    frame = (struct frame *) GC_save_regs_in_stack ();
    fp = (struct frame *)((long) frame -> FR_SAVFP + BIAS);
#endif
  
   for (; (!(fp HOTTER_THAN frame) && !(GC_stackbottom HOTTER_THAN (ptr_t)fp)
	   && (nframes < NFRAMES));
       fp = (struct frame *)((long) fp -> FR_SAVFP + BIAS), nframes++) {
      register int i;
      
      info[nframes].ci_pc = fp->FR_SAVPC;
#     if NARGS > 0
        for (i = 0; i < NARGS; i++) {
	  info[nframes].ci_arg[i] = ~(fp->fr_arg[i]);
        }
#     endif /* NARGS > 0 */
  }
  if (nframes < NFRAMES) info[nframes].ci_pc = 0;
}

#endif /* No builtin backtrace */

#endif /* SAVE_CALL_CHAIN */

#ifdef NEED_CALLINFO

/* Print info to stderr.  We do NOT hold the allocation lock */
void GC_print_callers (struct callinfo info[NFRAMES])
{
    register int i;
    static int reentry_count = 0;
    GC_bool stop = FALSE;

    /* FIXME: This should probably use a different lock, so that we	*/
    /* become callable with or without the allocation lock.		*/
    LOCK();
      ++reentry_count;
    UNLOCK();
    
#   if NFRAMES == 1
      GC_err_printf("\tCaller at allocation:\n");
#   else
      GC_err_printf("\tCall chain at allocation:\n");
#   endif
    for (i = 0; i < NFRAMES && !stop ; i++) {
     	if (info[i].ci_pc == 0) break;
#	if NARGS > 0
	{
	  int j;

     	  GC_err_printf("\t\targs: ");
     	  for (j = 0; j < NARGS; j++) {
     	    if (j != 0) GC_err_printf(", ");
     	    GC_err_printf("%d (0x%X)", ~(info[i].ci_arg[j]),
     	    				~(info[i].ci_arg[j]));
     	  }
	  GC_err_printf("\n");
	}
# 	endif
        if (reentry_count > 1) {
	    /* We were called during an allocation during	*/
	    /* a previous GC_print_callers call; punt.		*/
     	    GC_err_printf("\t\t##PC##= 0x%lx\n", info[i].ci_pc);
	    continue;
	}
	{
#	  ifdef LINUX
	    FILE *pipe;
#	  endif
#	  if defined(GC_HAVE_BUILTIN_BACKTRACE) \
	     && !defined(GC_BACKTRACE_SYMBOLS_BROKEN)
	    char **sym_name =
	      backtrace_symbols((void **)(&(info[i].ci_pc)), 1);
	    char *name = sym_name[0];
#	  else
	    char buf[40];
	    char *name = buf;
     	    sprintf(buf, "##PC##= 0x%lx", info[i].ci_pc);
#	  endif
#	  if defined(LINUX) && !defined(SMALL_CONFIG)
	    /* Try for a line number. */
	    {
#	        define EXE_SZ 100
		static char exe_name[EXE_SZ];
#		define CMD_SZ 200
		char cmd_buf[CMD_SZ];
#		define RESULT_SZ 200
		static char result_buf[RESULT_SZ];
		size_t result_len;
		char *old_preload;
#		define PRELOAD_SZ 200
    		char preload_buf[PRELOAD_SZ];
		static GC_bool found_exe_name = FALSE;
		static GC_bool will_fail = FALSE;
		int ret_code;
		/* Try to get it via a hairy and expensive scheme.	*/
		/* First we get the name of the executable:		*/
		if (will_fail) goto out;
		if (!found_exe_name) { 
		  ret_code = readlink("/proc/self/exe", exe_name, EXE_SZ);
		  if (ret_code < 0 || ret_code >= EXE_SZ
		      || exe_name[0] != '/') {
		    will_fail = TRUE;	/* Dont try again. */
		    goto out;
		  }
		  exe_name[ret_code] = '\0';
		  found_exe_name = TRUE;
		}
		/* Then we use popen to start addr2line -e <exe> <addr>	*/
		/* There are faster ways to do this, but hopefully this	*/
		/* isn't time critical.					*/
		sprintf(cmd_buf, "/usr/bin/addr2line -f -e %s 0x%lx", exe_name,
				 (unsigned long)info[i].ci_pc);
		old_preload = getenv ("LD_PRELOAD");
	        if (0 != old_preload) {
		  if (strlen (old_preload) >= PRELOAD_SZ) {
		    will_fail = TRUE;
		    goto out;
		  }
		  strcpy (preload_buf, old_preload);
		  unsetenv ("LD_PRELOAD");
	        }
		pipe = popen(cmd_buf, "r");
		if (0 != old_preload
		    && 0 != setenv ("LD_PRELOAD", preload_buf, 0)) {
		  WARN("Failed to reset LD_PRELOAD\n", 0);
      		}
		if (pipe == NULL
		    || (result_len = fread(result_buf, 1, RESULT_SZ - 1, pipe))
		       == 0) {
		  if (pipe != NULL) pclose(pipe);
		  will_fail = TRUE;
		  goto out;
		}
		if (result_buf[result_len - 1] == '\n') --result_len;
		result_buf[result_len] = 0;
		if (result_buf[0] == '?'
		    || (result_buf[result_len-2] == ':' 
		        && result_buf[result_len-1] == '0')) {
		    pclose(pipe);
		    goto out;
		}
		/* Get rid of embedded newline, if any.  Test for "main" */
		{
		   char * nl = strchr(result_buf, '\n');
		   if (nl != NULL && nl < result_buf + result_len) {
		     *nl = ':';
		   }
		   if (strncmp(result_buf, "main", nl - result_buf) == 0) {
		     stop = TRUE;
		   }
		}
		if (result_len < RESULT_SZ - 25) {
		  /* Add in hex address	*/
		    sprintf(result_buf + result_len, " [0x%lx]",
			  (unsigned long)info[i].ci_pc);
		}
		name = result_buf;
		pclose(pipe);
		out:;
	    }
#	  endif /* LINUX */
	  GC_err_printf("\t\t%s\n", name);
#	  if defined(GC_HAVE_BUILTIN_BACKTRACE) \
	     && !defined(GC_BACKTRACE_SYMBOLS_BROKEN)
	    free(sym_name);  /* May call GC_free; that's OK */
#         endif
	}
    }
    LOCK();
      --reentry_count;
    UNLOCK();
}

#endif /* NEED_CALLINFO */



#if defined(LINUX) && defined(__ELF__) && !defined(SMALL_CONFIG)

/* Dump /proc/self/maps to GC_stderr, to enable looking up names for
   addresses in FIND_LEAK output. */

static word dump_maps(char *maps)
{
    GC_err_write(maps, strlen(maps));
    return 1;
}

void GC_print_address_map(void)
{
    GC_err_printf("---------- Begin address map ----------\n");
    dump_maps(GC_get_maps());
    GC_err_printf("---------- End address map ----------\n");
}

#endif


