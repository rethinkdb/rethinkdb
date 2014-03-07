AM_CPPFLAGS = -I$(top_srcdir)/include

EXTRA_DIST =	run-ia64-test-dyn1 run-ptrace-mapper run-ptrace-misc	\
		run-check-namespace run-coredump-unwind \
		run-coredump-unwind-mdi check-namespace.sh.in \
		Gtest-nomalloc.c

MAINTAINERCLEANFILES = Makefile.in

noinst_PROGRAMS_common =
check_PROGRAMS_common = test-proc-info test-static-link \
			test-strerror
check_SCRIPTS_common =	run-check-namespace

if REMOTE_ONLY
 check_SCRIPTS_cdep =
 check_PROGRAMS_cdep =
 noinst_PROGRAMS_cdep = $(noinst_PROGRAMS_common)

perf:

else
 LIBUNWIND_local = $(top_builddir)/src/libunwind.la
if ARCH_IA64
 noinst_PROGRAMS_arch = ia64-test-dyn1
 check_SCRIPTS_arch =	run-ia64-test-dyn1
 check_PROGRAMS_arch =	Gia64-test-stack Lia64-test-stack		\
			Gia64-test-nat Lia64-test-nat			\
			Gia64-test-rbs Lia64-test-rbs			\
			Gia64-test-readonly Lia64-test-readonly		\
			ia64-test-setjmp ia64-test-sig
else #!ARCH_IA64
if ARCH_PPC64
if USE_ALTIVEC
 noinst_PROGRAMS_arch = ppc64-test-altivec
endif #USE_ALTIVEC
endif #ARCH_PPC64
endif #!ARCH_IA64
 check_SCRIPTS_cdep =	run-ptrace-mapper run-ptrace-misc
 check_PROGRAMS_cdep =	Gtest-bt Ltest-bt Gtest-exc Ltest-exc		 \
			Gtest-init Ltest-init				 \
			Gtest-concurrent Ltest-concurrent		 \
			Gtest-resume-sig Ltest-resume-sig		 \
			Gtest-resume-sig-rt Ltest-resume-sig-rt		 \
			Gtest-dyn1 Ltest-dyn1				 \
			Gtest-trace Ltest-trace				 \
			test-async-sig test-flush-cache test-init-remote \
			test-mem test-setjmp test-ptrace Ltest-varargs	 \
			Ltest-nomalloc Ltest-nocalloc Lrs-race
 noinst_PROGRAMS_cdep = forker crasher mapper test-ptrace-misc		 \
			Gperf-simple Lperf-simple

 noinst_PROGRAMS_cdep += Gperf-trace Lperf-trace

if SUPPORT_CXX_EXCEPTIONS
 check_PROGRAMS_cdep += Ltest-cxx-exceptions
endif

if OS_LINUX
if BUILD_COREDUMP
 check_SCRIPTS_cdep += run-coredump-unwind
 noinst_PROGRAMS_cdep += test-coredump-unwind

if HAVE_LZMA
 check_SCRIPTS_cdep += run-coredump-unwind-mdi
endif # HAVE_LZMA
endif # BUILD_COREDUMP
endif # OS_LINUX

perf: perf-startup Gperf-simple Lperf-simple Lperf-trace
	@echo "########## Basic performance of generic libunwind:"
	@./Gperf-simple
	@echo "########## Basic performance of local-only libunwind:"
	@./Lperf-simple
	@echo "########## Performance of fast unwind:"
	@./Lperf-trace
	@echo "########## Startup overhead:"
	@$(srcdir)/perf-startup @arch@

endif

check_PROGRAMS = $(check_PROGRAMS_common) $(check_PROGRAMS_cdep) \
		 $(check_PROGRAMS_arch)
check_SCRIPTS =	$(check_SCRIPTS_common) $(check_SCRIPTS_cdep) \
		$(check_SCRIPTS_arch)


TESTS = $(check_PROGRAMS) $(check_SCRIPTS)

noinst_PROGRAMS = $(noinst_PROGRAMS_common) $(noinst_PROGRAMS_cdep) \
	$(noinst_PROGRAMS_arch)

Lia64_test_readonly_SOURCES = Lia64-test-readonly.c ia64-test-readonly-asm.S
Gia64_test_readonly_SOURCES = Gia64-test-readonly.c ia64-test-readonly-asm.S
Lia64_test_stack_SOURCES = Lia64-test-stack.c ia64-test-stack-asm.S \
				ia64-test-stack.h
Gia64_test_stack_SOURCES = Gia64-test-stack.c ia64-test-stack-asm.S \
				ia64-test-stack.h
Lia64_test_rbs_SOURCES = Lia64-test-rbs.c ia64-test-rbs-asm.S ia64-test-rbs.h
Gia64_test_rbs_SOURCES = Gia64-test-rbs.c ia64-test-rbs-asm.S ia64-test-rbs.h
Lia64_test_nat_SOURCES = Lia64-test-nat.c ia64-test-nat-asm.S
Gia64_test_nat_SOURCES = Gia64-test-nat.c ia64-test-nat-asm.S
ia64_test_dyn1_SOURCES = ia64-test-dyn1.c ia64-dyn-asm.S flush-cache.S \
				flush-cache.h
ppc64_test_altivec_SOURCES = ppc64-test-altivec.c  ppc64-test-altivec-utils.c
Gtest_init_SOURCES = Gtest-init.cxx
Ltest_init_SOURCES = Ltest-init.cxx
Ltest_cxx_exceptions_SOURCES = Ltest-cxx-exceptions.cxx

Gtest_dyn1_SOURCES = Gtest-dyn1.c flush-cache.S flush-cache.h
Ltest_dyn1_SOURCES = Ltest-dyn1.c flush-cache.S flush-cache.h
test_static_link_SOURCES = test-static-link-loc.c test-static-link-gen.c
test_static_link_LDFLAGS = -static
forker_LDFLAGS = -static
Gtest_bt_SOURCES = Gtest-bt.c ident.c
Ltest_bt_SOURCES = Ltest-bt.c ident.c
test_ptrace_misc_SOURCES = test-ptrace-misc.c ident.c
Ltest_nomalloc_SOURCES = Ltest-nomalloc.c
Ltest_nocalloc_SOURCES = Ltest-nocalloc.c
Gtest_trace_SOURCES = Gtest-trace.c ident.c
Ltest_trace_SOURCES = Ltest-trace.c ident.c

LIBUNWIND = $(top_builddir)/src/libunwind-$(arch).la
LIBUNWIND_ptrace = $(top_builddir)/src/libunwind-ptrace.la
LIBUNWIND_coredump = $(top_builddir)/src/libunwind-coredump.la

if USE_ELF32
LIBUNWIND_ELF = $(top_builddir)/src/libunwind-elf32.la
endif
if USE_ELF64
LIBUNWIND_ELF = $(top_builddir)/src/libunwind-elf64.la
endif
if USE_ELFXX
LIBUNWIND_ELF = $(top_builddir)/src/libunwind-elfxx.la
endif

LIBUNWIND_setjmp = $(top_builddir)/src/libunwind-setjmp.la	\
		   $(LIBUNWIND_ELF) $(LIBUNWIND)

test_async_sig_LDADD = $(LIBUNWIND_local) -lpthread
test_flush_cache_LDADD = $(LIBUNWIND_local)
test_init_remote_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
test_mem_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
test_ptrace_LDADD = $(LIBUNWIND_ptrace) $(LIBUNWIND)
test_proc_info_LDADD = $(LIBUNWIND)
test_static_link_LDADD = $(LIBUNWIND)
test_strerror_LDADD = $(LIBUNWIND)
Lrs_race_LDADD = $(LIBUNWIND_local) -lpthread
Ltest_varargs_LDADD = $(LIBUNWIND_local)

Gtest_bt_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Gtest_concurrent_LDADD = $(LIBUNWIND) $(LIBUNWIND_local) -lpthread
Gtest_dyn1_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Gtest_exc_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Gtest_init_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Gtest_resume_sig_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Gtest_resume_sig_rt_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Gperf_simple_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Gtest_trace_LDADD=$(LIBUNWIND) $(LIBUNWIND_local)
Gperf_trace_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)

Ltest_bt_LDADD = $(LIBUNWIND_local)
Ltest_concurrent_LDADD = $(LIBUNWIND_local) -lpthread
Ltest_dyn1_LDADD = $(LIBUNWIND_local)
Ltest_exc_LDADD = $(LIBUNWIND_local)
Ltest_init_LDADD = $(LIBUNWIND_local)
Ltest_nomalloc_LDADD = $(LIBUNWIND_local) @DLLIB@
Ltest_nocalloc_LDADD = $(LIBUNWIND_local) @DLLIB@ -lpthread
Ltest_resume_sig_LDADD = $(LIBUNWIND_local)
Ltest_resume_sig_rt_LDADD = $(LIBUNWIND_local)
Lperf_simple_LDADD = $(LIBUNWIND_local)
Ltest_trace_LDADD = $(LIBUNWIND_local)
Lperf_trace_LDADD = $(LIBUNWIND_local)

test_setjmp_LDADD = $(LIBUNWIND_setjmp)
ia64_test_setjmp_LDADD = $(LIBUNWIND_setjmp)

if BUILD_COREDUMP
test_coredump_unwind_LDADD = $(LIBUNWIND_coredump) $(LIBUNWIND)
endif

Gia64_test_nat_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Gia64_test_stack_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Gia64_test_rbs_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Gia64_test_readonly_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
Lia64_test_nat_LDADD = $(LIBUNWIND_local)
Lia64_test_stack_LDADD = $(LIBUNWIND_local)
Lia64_test_rbs_LDADD = $(LIBUNWIND_local)
Lia64_test_readonly_LDADD = $(LIBUNWIND_local)
ia64_test_dyn1_LDADD = $(LIBUNWIND)
ia64_test_sig_LDADD = $(LIBUNWIND)
