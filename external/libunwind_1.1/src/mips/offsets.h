/* Linux-specific definitions: */

/* Define various structure offsets to simplify cross-compilation.  */

/* FIXME: Currently these are only used in getcontext.S, which is only used
   for a local unwinder, so we can use the compile-time ABI.  At a later date
   we will want all three here, to use for signal handlers.  Also, because
   of the three ABIs, gen-offsets.c can not quite generate this file.  */

/* Offsets for MIPS Linux "ucontext_t":  */

#if _MIPS_SIM == _ABIO32

# define LINUX_UC_FLAGS_OFF	0x0
# define LINUX_UC_LINK_OFF	0x4
# define LINUX_UC_STACK_OFF	0x8
# define LINUX_UC_MCONTEXT_OFF	0x18
# define LINUX_UC_SIGMASK_OFF	0x268
# define LINUX_UC_MCONTEXT_PC	0x20
# define LINUX_UC_MCONTEXT_GREGS	0x28

#elif _MIPS_SIM == _ABIN32

# define LINUX_UC_FLAGS_OFF	0x0
# define LINUX_UC_LINK_OFF	0x4
# define LINUX_UC_STACK_OFF	0x8
# define LINUX_UC_MCONTEXT_OFF	0x18
# define LINUX_UC_SIGMASK_OFF	0x270
# define LINUX_UC_MCONTEXT_PC	0x258
# define LINUX_UC_MCONTEXT_GREGS	0x18

#elif _MIPS_SIM == _ABI64

# define LINUX_UC_FLAGS_OFF	0x0
# define LINUX_UC_LINK_OFF	0x8
# define LINUX_UC_STACK_OFF	0x10
# define LINUX_UC_MCONTEXT_OFF	0x28
# define LINUX_UC_SIGMASK_OFF	0x280
# define LINUX_UC_MCONTEXT_PC	0x268
# define LINUX_UC_MCONTEXT_GREGS	0x28

#endif
