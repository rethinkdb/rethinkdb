# Copyright 2010-2012 RethinkDB, all rights reserved.
space=$(eval) $(eval)
comma=,


##### Pretty-printing

ANSI_BOLD_ON:=[1m
ANSI_BOLD_OFF:=[0m
ANSI_UL_ON:=[4m
ANSI_UL_OFF:=[0m

##### Configuring stuff

UNAME:=$(shell uname)
ifeq ($(UNAME),Darwin)
OSX?=1
else
OSX?=0
endif

IGNORE_MAKEFILE_CHANGES?=0

ifeq ($(IGNORE_MAKEFILE_CHANGES),1)
MAKEFILE_DEPENDENCY:=
else
MAKEFILE_DEPENDENCY:=Makefile
endif

GCC_ARCH=$(shell uname -m | grep '^[A-Za-z0-9_]*$$' | head -n 1)
GCC_ARCH_REDUCED=$(shell echo "$(GCC_ARCH)" | sed -e 's/^i[56]86$$/i486/g')
DEB_ARCH=$(shell echo "$(GCC_ARCH_REDUCED)" | sed -e 's/^x86_64$$/amd64/g')

# Set SOLO to 0 to build RethinkDB Cache (otherwise, RethinkDB Solo is built)
SOLO?=1
ifeq ($(SOLO),1)
RETHINKDB_CACHE_PRODUCT:=0
RETHINKDB_SOLO_PRODUCT:=1
PRODUCT_NAME:=RethinkDB
VANILLA_PACKAGE_NAME?=rethinkdb
SERVER_EXEC_NAME?=rethinkdb
else
RETHINKDB_CACHE_PRODUCT:=1
RETHINKDB_SOLO_PRODUCT:=0
PRODUCT_NAME:=RethinkDB Cache
VANILLA_PACKAGE_NAME?=rethinkdb-cache
SERVER_EXEC_NAME?=rethinkdb-cache
endif

STATIC?=0

ifeq ($(STATIC),1)
VANILLA_PACKAGE_NAME+=-static
STATICFORCE?=1
endif

TRIAL_PACKAGE_NAME:=$(VANILLA_PACKAGE_NAME)-trial
PACKAGE_NAME:=$(VANILLA_PACKAGE_NAME)
SERVER_UNIT_TEST_NAME:=$(SERVER_EXEC_NAME)-unittest

PREFIX?=

SCRIPTS_DIR:=../scripts
START_DB_NAME:=start_rethinkdb
GDB_FUNCTIONS_NAME:=rethinkdb-gdb.py
CTAGSPROG:=ctags
ETAGSPROG:=$(CTAGSPROG) -e
TAGSFILE:=.tags
ETAGSFILE:=TAGS
CSCOPE_XREF:=.cscope
RUN_PORT?=11211
RUN_FLAGS?=-s 3 -c 2 -m 10
RUN_FILE?=rethinkdb_data
VALGRIND_FLAGS?=--leak-check=full --db-attach=yes --show-reachable=yes --suppressions=../scripts/rethinkdb-valgrind-suppressions.supp
CALLGRIND_FLAGS?=--simulate-cache=yes --simulate-wb=yes --simulate-hwpref=yes --cacheuse=yes --collect-jumps=yes

COMPILER?=gcc
override COMPILER:=$(shell echo $(COMPILER) | tr '[:lower:]' '[:upper:]')
SUPPORTED_COMPILERS:=GCC CLANG INTEL
ifneq ($(COMPILER),$(filter $(COMPILER),$(SUPPORTED_COMPILERS)))
$(error Unknown compiler: $(COMPILER). The supported compilers are: $(SUPPORTED_COMPILERS))
endif

# Set SYMBOLS to 1 to enable symbols, even in release mode
SYMBOLS?=0

ifeq ($(GPERF),1)
DEBUG=0
NO_TCMALLOC:=1
SYMBOLS:=1
NO_OMIT_FRAME_POINTER:=1
endif

JSON_SHORTCUTS?=0
DEBUG?=1
VALGRIND?=0
MCHECK?=0
ifeq ($(OSX),1)
NO_TCMALLOC?=1
else
NO_TCMALLOC?=0
endif
SEMANTIC_SERIALIZER_CHECK?=0
MOCK_CACHE_CHECK?=0
VERBOSE?=0
UNIT_TESTS?=0
AIOSUPPORT?=0
BUILD_DRIVERS?=1
LINT?=0

# Set TRIAL to 1 to build a 21-day trial version. If you want a different number of days, set TIMEBOMB to the number of days instead.
TRIAL?=0
TIMEBOMB?=0

ifeq ($(OSX),0)
LDPTHREADFLAG:=-pthread
else
LDPTHREADFLAG:=
endif

# Choose our directories
# This makefile is meant to be run from the 'src' directory.
SOURCE_DIR:=.
BUILD_ROOT_DIR:=../build
ifeq (${DEBUG},1)
BUILD_DIR:=$(BUILD_ROOT_DIR)/debug
else
BUILD_DIR:=$(BUILD_ROOT_DIR)/release
endif

ifeq ($(COMPILER), CLANG)
BUILD_DIR:=$(BUILD_DIR)_clang
else
ifeq ($(COMPILER), INTEL)
BUILD_DIR:=$(BUILD_DIR)_intel
endif
endif
DESTDIR?=/tmp/

PACKAGES_DIR:=$(BUILD_ROOT_DIR)/packages
RPM_PACKAGE_DIR:=$(PACKAGES_DIR)/rpm
DEB_PACKAGE_DIR:=$(PACKAGES_DIR)/deb

RT_CXX?=$(CXX)

ifeq ($(COMPILER),CLANG)
RT_CXX:=clang
else
ifeq ($(COMPILER),INTEL)
RT_CXX:=icc
endif
endif

# We assemble path directives.
LDPATHDS:=
CXXPATHDS:=
CPATHDS:=

STATICFORCE?=0

RT_LDFLAGS:=

ifeq ($(COMPILER),CLANG)
ifeq ($(OSX),0)
RT_LDFLAGS+=-Wl,--no-as-needed
endif

ifeq ($(STATICFORCE),1)
# TODO(OSX)
ifeq ($(OSX),0)
RT_LDFLAGS+= -static
endif
ifeq ($(OSX),0)
STATIC_LIBGCC=1
endif
endif

RT_LDFLAGS+=$(LDPATHDS) $(LDPTHREADFLAG) -lstdc++ -lm

else  # ifeq ($(COMPILER),CLANG)

ifeq ($(COMPILER),INTEL)
RT_LDFLAGS+= -B/opt/intel/bin

ifeq ($(STATICFORCE),1)
# TODO(OSX)
ifeq ($(OSX),0)
RT_LDFLAGS+= -static
endif
ifeq ($(OSX),0)
STATIC_LIBGCC=1
endif
endif

RT_LDFLAGS+=$(LDPATHDS) $(LDPTHREADFLAG) -lstdc++
else  # ifeq ($(COMPILER),INTEL)
# GCC
ifeq ($(OSX),0)
RT_LDFLAGS+=-Wl,--no-as-needed
endif

ifeq ($(STATICFORCE),1)
# TODO(OSX)
ifeq ($(OSX),0)
RT_LDFLAGS+= -static
endif
ifeq ($(OSX),0)
STATIC_LIBGCC=1
endif
endif

RT_LDFLAGS+=$(LDPATHDS) $(LDPTHREADFLAG)
endif  # ifeq ($(COMPILER),INTEL)
endif  # ifeq ($(COMPILER),CLANG)

ifeq ($(OSX),0)
RT_LDFLAGS+=-lrt
endif

ifeq ($(BUILD_PORTABLE),1)
ifeq ($(OSX),0)
RT_LDFLAGS+=-lgcc
endif
endif

ifeq ($(STATICFORCE),1)
# TODO(OSX)
ifeq ($(OSX),0)
RT_LDFLAGS+= -Wl,-Bdynamic
endif
endif

ifeq ($(BUILD_PORTABLE),1)
ifeq ($(OSX),0)
RT_LDFLAGS+=-lgcc_s
endif
endif

BUILD_PORTABLE?=0
LEGACY_LINUX?=0

ifeq ($(BUILD_PORTABLE),1)
LEGACY_PACKAGE?=1
else
ifeq ($(LEGACY_LINUX),1)
LEGACY_PACKAGE?=1
else
LEGACY_PACKAGE?=0
endif
endif

V8_SRC_DIR:=../support/src/v8
V8_DIR:=../support/build/v8
V8_LIB:=$(V8_DIR)/libv8.a

ifeq ($(BUILD_PORTABLE),1)
V8_CHAIN?=1
STATIC_V8?=2
else
V8_CHAIN?=0
PROTOC_CHAIN?=1
STATIC_V8?=$(STATICFORCE)
endif

OBJ_SUPPS:=

ifneq ($(V8_CHAIN),0)
ifeq ($(V8_CHAIN),2)
RT_LDFLAGS+= -L $(V8_DIR)
endif
CXXPATHDS+= -isystem $(V8_DIR)/include
CPATHDS+= -isystem $(V8_DIR)/include
ifeq ($(STATIC_V8),2)
OBJ_SUPPS+=$(V8_LIB)
endif
endif

ifeq ($(STATIC_V8),0)
RT_LDFLAGS+=-lv8
else
ifeq ($(STATIC_V8),1)

# TODO(OSX) Remove commented code
# ifeq ($(OSX),1)
# TODO(OSX) Support static linking v8?  Or meh, make everybody build using homebrew?
# RT_LDFLAGS+=-lv8
# else
# TODO(OSX) How the heck does -dynamic work?  According to the gcc man
# page (on Linux), that's just passed through to the linker on Darwin.
ifeq ($(OSX),1)
RT_LDFLAGS+=-lv8
else
RT_LDFLAGS+=-static -lv8 -dynamic
endif
# endif
endif
endif

# We have support for chain-building tools needed at compile-time. We handle V8 elsewhere for now.

SUPPORT_DIR:=../support
SUPPORT_DIR_ABS:=$(shell pwd ;)/$(SUPPORT_DIR)
SUPPORT_INST_DIR_ABS:=$(SUPPORT_DIR_ABS)/usr
EXTERNAL_DIR:=../external
EXTERNAL_DIR_ABS:=$(shell pwd ;)/$(EXTERNAL_DIR)
COLONIZE_SCRIPT:=$(EXTERNAL_DIR)/colonist/colonize.sh
COLONIZE_SCRIPT_ABS:=$(EXTERNAL_DIR_ABS)/colonist/colonize.sh
AUTOCHAIN?=0
ALLOW_INTERNAL_TOOLS?=1
FORCE_INTERNAL_TOOLS?=0
FETCH_INTERNAL_TOOLS?=0
TC_BUILD_DIR:=$(SUPPORT_DIR_ABS)/build
TC_SRC_DIR:=$(SUPPORT_DIR_ABS)/src
NODE_DIR:=$(TC_BUILD_DIR)/node
NODE_SRC_DIR:=$(TC_SRC_DIR)/node
PROTOC_DIR:=$(TC_BUILD_DIR)/protobuf
PROTOC_SRC_DIR:=$(TC_SRC_DIR)/protobuf
GPERFTOOLS_DIR:=$(TC_BUILD_DIR)/gperftools
GPERFTOOLS_SRC_DIR:=$(TC_SRC_DIR)/gperftools
LIBUNWIND_DIR:=$(TC_BUILD_DIR)/libunwind
LIBUNWIND_SRC_DIR:=$(TC_SRC_DIR)/libunwind
TCMALLOC_MINIMAL_INT_LIB:=$(SUPPORT_INST_DIR_ABS)/lib/libtcmalloc_minimal.a
TC_PROTOC_INT_EXE:=$(SUPPORT_INST_DIR_ABS)/bin/protoc
TC_PROTOC_INT_BIN_DIR:=$(SUPPORT_INST_DIR_ABS)/bin
TC_PROTOC_INT_LIB_DIR:=$(SUPPORT_INST_DIR_ABS)/lib
TC_PROTOC_INT_INC_DIR:=$(SUPPORT_INST_DIR_ABS)/include
TC_NODE_INT_EXE:=$(SUPPORT_DIR_ABS)/usr/bin/node
TC_NPM_INT_EXE:=$(SUPPORT_DIR_ABS)/usr/bin/npm
TC_LESSC_INT_EXE:=$(SUPPORT_DIR_ABS)/toolchain/node_modules/less/bin/lessc
TC_COFFEE_INT_EXE:=$(SUPPORT_DIR_ABS)/toolchain/node_modules/coffee-script/bin/coffee
TC_HANDLEBARS_INT_EXE:=$(SUPPORT_DIR_ABS)/toolchain/node_modules/handlebars/bin/handlebars

TC_PROTOC_CFLAGS:=
INT_CHAIN_NODE:=0
INT_CHAIN_PROTOC:=0
TCMALLOC_CHAIN_IF_NECESSARY:=0

ifeq ($(FORCE_INTERNAL_TOOLS),1)
TC_PROTOC_EXE:=$(TC_PROTOC_INT_EXE)
# This is how we run TC_PROTOC_EXE.
TC_PROTOC_RUN:=env LD_LIBRARY_PATH=$(TC_PROTOC_INT_LIB_DIR):$(LD_LIBRARY_PATH) PATH=$(TC_PROTOC_INT_BIN_DIR):$(PATH) $(TC_PROTOC_EXE)
TC_PROTOC_CFLAGS:= -isystem $(TC_PROTOC_INT_INC_DIR)
CXXPATHDS+=$(TC_PROTOC_CFLAGS)
CPATHDS+=$(TC_PROTOC_CFLAGS)
# RT_LDFLAGS+= -L $(TC_PROTOC_INT_LIB_DIR)
# We do static linking explicitly now since things get messy otherwise.
INT_CHAIN_PROTOC:=1
INT_CHAIN_NODE:=1
TC_NODE_EXE:=$(TC_NODE_INT_EXE)
TC_NPM_EXE:=$(TC_NPM_INT_EXE)
TC_LESSC_EXE:=$(TC_LESSC_INT_EXE)
TC_COFFEE_EXE:=$(TC_COFFEE_INT_EXE)
TC_HANDLEBARS_EXE:=$(TC_HANDLEBARS_INT_EXE)
TCMALLOC_CHAIN_IF_NECESSARY:=1
else
ifeq ($(ALLOW_INTERNAL_TOOLS),1)
ifeq ($(shell which protoc || true),)
TC_PROTOC_EXE:=$(TC_PROTOC_INT_EXE)
TC_PROTOC_RUN:=env LD_LIBRARY_PATH=$(TC_PROTOC_INT_LIB_DIR):$(LD_LIBRARY_PATH) PATH=$(TC_PROTOC_INT_BIN_DIR):$(PATH) $(TC_PROTOC_EXE)
TC_PROTOC_CFLAGS:= -isystem $(TC_PROTOC_INT_INC_DIR)
CXXPATHDS+=$(TC_PROTOC_CFLAGS)
CPATHDS+=$(TC_PROTOC_CFLAGS)
# RT_LDFLAGS+= -L $(TC_PROTOC_INT_LIB_DIR)
# We do static linking explicitly now since things get messy otherwise.
INT_CHAIN_PROTOC:=1
else
TC_PROTOC_EXE:=$(shell which protoc || true)
TC_PROTOC_RUN:=$(TC_PROTOC_EXE)
endif
ifeq ($(shell which npm || true),)
# If npm is not present, node.js is likely too old.
INT_CHAIN_NODE:=1
TC_NODE_EXE:=$(TC_NODE_INT_EXE)
TC_NPM_EXE:=$(TC_NPM_INT_EXE)	
else
TC_NODE_EXE:=$(shell which node || true)
TC_NPM_EXE:=$(shell which npm || true)
endif
ifeq ($(shell which lessc || true),)
TC_LESSC_EXE:=$(TC_LESSC_INT_EXE)	
else
TC_LESSC_EXE:=$(shell which lessc || true)
endif
ifeq ($(shell which coffee || true),)
TC_COFFEE_EXE:=$(TC_COFFEE_INT_EXE)
else
TC_COFFEE_EXE:=$(shell which coffee || true)
endif
ifeq ($(shell which handlebars || true),)
TC_HANDLEBARS_EXE:=$(TC_HANDLEBARS_INT_EXE)
else
TC_HANDLEBARS_EXE:=$(shell which handlebars || true)
endif
ifeq ($(OSX),0)
ifeq ($(shell /sbin/ldconfig -p | awk '/libtcmalloc_minimal.so /'),)
TCMALLOC_CHAIN_IF_NECESSARY:=1
endif
endif
else # ALLOW_INTERNAL_TOOLS
TC_PROTOC_EXE:=$(shell which protoc || true)
TC_PROTOC_RUN:=$(TC_PROTOC_EXE)
TC_LESSC_EXE:=$(shell which lessc || true)
TC_COFFEE_EXE:=$(shell which coffee || true)
TC_HANDLEBARS_EXE:=$(shell which which handlebars || true)
TC_NODE_EXE:=$(shell which node || true)
TC_NPM_EXE:=$(shell which npm || true)
endif # ALLOW_INTERNAL_TOOLS
endif # FORCE_INTERNAL_TOOLS

ifeq ($(INT_CHAIN_NODE),0)
NODE_EXE_SPEC:=
else
NODE_EXE_SPEC:=$(TC_NODE_EXE)
NODE_EXE_SPEC+=
endif

ifeq ($(V8_CHAIN),1)
V8_DEP:=$(V8_DIR)
else
V8_DEP:=
endif

ifeq ($(LIBCRYPTO),1)
RT_LDFLAGS+=-lcrypto 
endif

PROTOC_BASE:=$(shell dirname $(shell dirname $(TC_PROTOC_EXE)))

LIB_SEARCH_PATHS?=
RT_LDFLAGS+=$(foreach TLIB, $(LIB_SEARCH_PATHS), -L $(TLIB))

STATIC_LIBRARY_PATHS:=

STATIC_LIBRARIES:=

STATIC_RECOMMENDS_INDIFFERENT:=boost_serialization boost_program_options

ifeq ($(INT_CHAIN_PROTOC),1)
STATIC_LIBRARY_PATHS+=$(TC_PROTOC_INT_LIB_DIR)/libprotobuf.a
else
STATIC_RECOMMENDS_INDIFFERENT+=protobuf
endif

STATIC_RECOMMENDS_IMPORTANT:=

ifeq ($(NO_TCMALLOC),0)
ifeq ($(TCMALLOC_CHAIN_IF_NECESSARY),0)
# If we need TCMALLOC and we are not already obligated to chain-building the static library, we put it in the recommended list.
STATIC_RECOMMENDS_IMPORTANT+=tcmalloc_minimal
else
OBJ_SUPPS+=$(TCMALLOC_MINIMAL_INT_LIB)
STATIC_LIBRARY_PATHS+=$(TCMALLOC_MINIMAL_INT_LIB)
endif
endif

RECOMMENDED_STATIC?=1
HIGHLY_RECOMMENDED_STATIC?=1

ifeq ($(RECOMMENDED_STATIC),1)
STATIC_LIBRARIES+=$(STATIC_RECOMMENDS_INDIFFERENT)
else
RT_LDFLAGS+=$(foreach TLIB, $(STATIC_RECOMMENDS_INDIFFERENT), -l$(TLIB))
endif

ifeq ($(HIGHLY_RECOMMENDED_STATIC),1)
STATIC_LIBRARIES+=$(STATIC_RECOMMENDS_IMPORTANT)
else
RT_LDFLAGS+=$(foreach TLIB, $(STATIC_RECOMMENDS_IMPORTANT), -l$(TLIB))
endif

ifeq ($(STATIC_V8),2)
ifeq ($(V8_CHAIN),0)
STATIC_LIBRARIES+=libv8
else
STATIC_LIBRARY_PATHS+=$(V8_LIB)
endif
endif

ifeq ($(OSX),1)
# /usr/local/lib is where Homebrew will put these libraries.  Will
# they be named differently if you don't use Homebrew?  (No -mt on
# program options?)
# TODO(OSX)
RT_LDFLAGS+=/usr/local/lib/libboost_program_options-mt.a /usr/local/lib/libprotobuf.a
#RT_LDFLAGS+=-L/usr/local/lib -lboost_program_options-mt -lprotobuf
else

# look for the static library in the same directory as the .so file
# On older platforms, certain libraries such as tcmalloc_minimal may not be available unversioned (tcmalloc_minimal.so). If we are on such a platform, we settle for something versioned.
STATIC_LIBRARY_PATHS+=$(foreach lib,$(STATIC_LIBRARIES),$(shell /sbin/ldconfig -p | awk '/lib$(lib).so / { gsub("\\.so$$", ".a", $$NF); print $$NF; exit 0; }'))
endif

ifeq ($(GPERF),1)
STATIC_LIBRARY_PATHS+= # path to patched libtcmalloc_and_profiler.a
endif

RT_CXXFLAGS?=

RT_CXXFLAGS+=-I$(SOURCE_DIR) -pthread
RT_CXXFLAGS+="-DPRODUCT_NAME=\"$(PRODUCT_NAME)\""

RT_CXXFLAGS+=$(CXXPATHDS)
RT_CFLAGS+=$(CPATHDS)

ifeq ($(SOLO),1)
RT_CXXFLAGS+=-DRETHINKDB_SOLO_PRODUCT
else
RT_CXXFLAGS+=-DRETHINKDB_CACHE_PRODUCT
endif

RT_CXXFLAGS+=-Wall -Wextra -Werror -Wnon-virtual-dtor

ifeq ($(COMPILER), INTEL)
RT_CXXFLAGS+=-std=gnu++98 -w1 -ftls-model=local-dynamic
else
ifeq ($(COMPILER), CLANG)
RT_CXXFLAGS+=-Wformat=2 -Wswitch-enum -Wswitch-default -Wno-unneeded-internal-declaration -Wused-but-marked-unused -Wunused-macros -Wundef -Wvla -Wshadow -Wconditional-uninitialized -Wmissing-noreturn
else  # ($(COMPILER), CLANG)
ifeq ($(LEGACY_GCC), 1)
RT_CXXFLAGS+=-Wformat=2 -Wswitch-enum -Wswitch-default
else  # ($(LEGACY_GCC), 1)
RT_CXXFLAGS+=-Wformat=2 -Wswitch-enum -Wswitch-default -Wno-array-bounds 

endif  # ($(LEGACY_GCC), 1)
endif  # ($(COMPILER), CLANG)
endif  # ($(COMPILER), INTEL)

ifeq ($(AGRESSIVE_BUF_UNLOADING),1)
RT_CXXFLAGS+=-DAGRESSIVE_BUF_UNLOADING=1
endif

ifneq ($(PREFIX),)
RT_CXXFLAGS+=-DCPREFIX='"$(PREFIX)"'
endif

ifneq ($(WEBRESDIR),)
RT_CXXFLAGS+=-DWEBRESDIR='"$(WEBRESDIR)"'
endif

ifeq ($(COMPILER),INTEL)
IGNORE_SOME_ICC_LD_WARNINGS?=1
else
IGNORE_SOME_ICC_LD_WARNINGS?=0
endif

STRIP_ON_INSTALL?=1

NO_OMIT_FRAME_POINTER?=0

RT_FORCE_NATIVE?=0
RT_COPY_NATIVE?=0
RT_REDUCE_NATIVE?=0

ifeq ($(RT_FORCE_NATIVE),1)
RT_CXXFLAGS+=-march=native
endif
ifeq ($(RT_COPY_NATIVE),1)
RT_CXXFLAGS+=-march="$(GCC_ARCH)"
endif
ifeq ($(RT_REDUCE_NATIVE),1)
RT_CXXFLAGS+=-march="$(GCC_ARCH_REDUCED)"
endif

# Configure debug vs. release
ifeq ($(DEBUG),1)
SYMBOLS:=1
RT_CXXFLAGS+=-O0

ifeq ($(KEEP_INLINE),1)
RT_CXXFLAGS+=-fkeep-inline-functions
endif

else  # ifeq ($(DEBUG),1)
# use -fno-strict-aliasing to not break things
# march=native used to break the serializer
RT_CXXFLAGS+=-O3 -DNDEBUG -fno-strict-aliasing # -march=native
# TODO: remove this once memcached is added back in the release (disables memcached from showing up in the admin CLI help or tab-completion)
RT_CXXFLAGS+=-DNO_MEMCACHE
ifeq ($(NO_OMIT_FRAME_POINTER),1)
RT_CXXFLAGS+=-fno-omit-frame-pointer
endif
endif  # ifeq ($(DEBUG),1)

ifeq ($(DISABLE_BREAKPOINTS),1)
RT_CXXFLAGS+=-DDISABLE_BREAKPOINTS
endif

ifneq ($(TRIAL),0)
ifeq ($(TIMEBOMB),0)
TIMEBOMB:=21
endif
endif

ifneq ($(TIMEBOMB),0)
TRIAL:=1
RT_CXXFLAGS+=-DTIMEBOMB_DAYS=$(TIMEBOMB)
endif

ifneq ($(TRIAL),0)
BUILD_DIR:=$(BUILD_DIR)-trial
PACKAGE_NAME:=$(TRIAL_PACKAGE_NAME)
SERVER_EXEC_NAME:=$(SERVER_EXEC_NAME)-trial
endif

ifeq (${STATIC_LIBGCC},1)
RT_LDFLAGS+=-static-libgcc -static-libstdc++
endif

ifeq ($(OPROFILE),1)
SYMBOLS=1
endif

ifeq ($(CALLGRIND),1)
SYMBOLS=1
endif

ifeq ($(SYMBOLS),1)
# -rdynamic is necessary so that backtrace_symbols() works properly
ifeq ($(OSX),0)
RT_LDFLAGS+=-rdynamic
endif
RT_CXXFLAGS+=-g
endif  # ($(SYMBOLS),1)

ifeq ($(SEMANTIC_SERIALIZER_CHECK),1)
RT_CXXFLAGS+=-DSEMANTIC_SERIALIZER_CHECK
BUILD_DIR:=$(BUILD_DIR)-scs
endif

ifeq ($(MOCK_CACHE_CHECK),1)
RT_CXXFLAGS+=-DMOCK_CACHE_CHECK
BUILD_DIR:=$(BUILD_DIR)-mockcache
endif

ifeq ($(BTREE_DEBUG),1)
RT_CXXFLAGS+=-DBTREE_DEBUG
endif

ifeq ($(JSON_SHORTCUTS),1)
RT_CXXFLAGS+=-DJSON_SHORTCUTS
endif

ifeq ($(MALLOC_PROF),1)
RT_CXXFLAGS+=-DMALLOC_PROF
endif

ifeq ($(SERIALIZER_DEBUG),1)
RT_CXXFLAGS:=$(RT_CXXFLAGS) -DSERIALIZER_MARKERS
endif

ifneq ($(MEMCACHED_STRICT), 0)
RT_CXXFLAGS+=-DMEMCACHED_STRICT
endif

ifeq ($(LEGACY_LINUX),1)
#CXX=g++44
RT_CXXFLAGS+=-DLEGACY_LINUX -DNO_EPOLL -Wno-format
BUILD_DIR:=$(BUILD_DIR)-legacy
endif

ifeq ($(LEGACY_GCC),1)
RT_CXXFLAGS+=-Wno-switch-default -Wno-switch-enum
BUILD_DIR:=$(BUILD_DIR)-legacy-gcc
endif

ifeq ($(NO_EVENTFD),1)
RT_CXXFLAGS+=-DNO_EVENTFD
BUILD_DIR:=$(BUILD_DIR)-noeventfd
endif

ifeq ($(NO_EPOLL),1)
RT_CXXFLAGS+=-DNO_EPOLL
BUILD_DIR:=$(BUILD_DIR)-noepoll
endif

ifeq ($(MCHECK_PEDANTIC),1)
RT_CXXFLAGS+=-DMCHECK_PEDANTIC
MCHECK:=1
endif

ifeq ($(MCHECK),1)
NO_TCMALLOC:=1
RT_CXXFLAGS+=-DMCHECK
RT_LDFLAGS+=-lmcheck
endif

ifeq ($(VALGRIND),1)
RT_CXXFLAGS+=-DVALGRIND
BUILD_DIR:=$(BUILD_DIR)-valgrind
NO_TCMALLOC:=1
endif

ifeq ($(AIOSUPPORT),1)
BUILD_DIR:=$(BUILD_DIR)-aiosupport
RT_CXXFLAGS+=-DAIOSUPPORT
RT_LDFLAGS+=-laio
endif

ifeq ($(LEGACY_PROC_STAT),1)
RT_CXXFLAGS+=-DLEGACY_PROC_STAT
BUILD_DIR:=$(BUILD_DIR)-legacy-proc-stat
endif

PROTO_DIR:=$(BUILD_DIR)/proto
RT_CXXFLAGS += -I$(PROTO_DIR)

#  /usr/lib/x86_64-linux-gnu/libQtCore.so.4

# Should makefile be noisy?
ifeq ($(VERBOSE),1)
QUIET:=
else
ifeq ($(TIMINGS),1)
QUIET:=@ time
else
QUIET:=@
endif
endif

ifeq ($(UNIT_TESTS),1)
UNIT_STATIC_LIBRARIES+=gtest
UNIT_STATIC_LIBRARY_PATHS+=$(foreach lib,$(UNIT_STATIC_LIBRARIES),$(shell /sbin/ldconfig -p | awk '/lib$(lib).so / { gsub("\\.so$$", ".a", $$NF); print $$NF; exit 0; }'))
endif

UNIT_TEST_FILTER?=*

##### Finding what to build

DEP_DIR:=$(BUILD_DIR)/dep
OBJ_DIR:=$(BUILD_DIR)/obj

# clustering/administration/main/serve.cc is some of our slowest-compiling files, so we start it first.
ifeq ($(UNIT_TESTS),1)
SOURCES:=$(shell find $(SOURCE_DIR) -name '*.cc')
else
SOURCES:=$(shell find $(SOURCE_DIR) -name '*.cc' | grep -vF "`find $(SOURCE_DIR)/unittest`")
endif

PROTO_SOURCES:=$(shell find $(SOURCE_DIR) -name '*.proto')
PROTO_HEADERS:=$(patsubst $(SOURCE_DIR)/%.proto,$(PROTO_DIR)/%.pb.h,$(PROTO_SOURCES))
PROTO_CODE:=$(patsubst $(SOURCE_DIR)/%.proto,$(PROTO_DIR)/%.pb.cc,$(PROTO_SOURCES))
PROTO_NAMES+=$(patsubst $(PROTO_DIR)/%.cc,%,$(PROTO_CODE))
PROTO_OBJS:=$(patsubst %,$(OBJ_DIR)/%.o,$(PROTO_NAMES))
PROTOCFLAGS:= --proto_path=$(SOURCE_DIR)

NAMES:=$(patsubst $(SOURCE_DIR)/%.cc,%,$(SOURCES))
DEPS:=$(patsubst %,$(DEP_DIR)/%.d,$(NAMES))
OBJS:=$(PROTO_OBJS) $(patsubst %,$(OBJ_DIR)/%.o,$(NAMES))

SERVER_EXEC_OBJS:=$(PROTO_OBJS) $(patsubst %.cc,$(OBJ_DIR)/%.o,$(filter-out $(SOURCE_DIR)/unittest/%,$(SOURCES)))

SERVER_NOMAIN_OBJS:=$(PROTO_OBJS) $(patsubst %.cc,$(OBJ_DIR)/%.o,$(filter-out %/main.cc,$(SOURCES)))

SERVER_UNIT_TEST_OBJS:=$(SERVER_NOMAIN_OBJS) $(OBJ_DIR)/unittest/main.o

#### Web UI sources
WEB_SOURCE_DIR:=../admin
WEB_ASSETS_BUILD_DIR:=$(BUILD_DIR)/web
WEB_ASSETS_OBJ_DIR:=$(BUILD_DIR)/webobj
WEB_ASSETS:=$(WEB_ASSETS_BUILD_DIR)/cluster-min.js $(WEB_ASSETS_BUILD_DIR)/cluster.css $(WEB_ASSETS_BUILD_DIR)/index.html $(WEB_ASSETS_BUILD_DIR)/js $(WEB_ASSETS_BUILD_DIR)/fonts $(WEB_ASSETS_BUILD_DIR)/images $(WEB_ASSETS_BUILD_DIR)/favicon.ico $(WEB_ASSETS_BUILD_DIR)/js/rethinkdb.js $(WEB_ASSETS_BUILD_DIR)/js/template.js 

#This atrociousness comes from the fact... fuck it blah blah blah coffee script.
#Basically coffee script can't handle dependencies.
COFFEE_SOURCES:=$(patsubst %, $(WEB_SOURCE_DIR)/static/coffee/%,\
			util.coffee \
			loading.coffee \
			body.coffee \
			ui_components/modals.coffee ui_components/list.coffee ui_components/progressbar.coffee \
			namespaces/database.coffee \
			namespaces/index.coffee namespaces/replicas.coffee namespaces/shards.coffee namespaces/server_assignments.coffee namespaces/namespace.coffee \
			servers/index.coffee servers/machine.coffee servers/datacenter.coffee \
			dashboard.coffee \
			dataexplorer.coffee \
			sidebar.coffee \
			resolve_issues.coffee \
			log_view.coffee \
			vis.coffee \
			models.coffee \
			navbar.coffee \
			walkthrough.coffee \
			router.coffee \
			app.coffee)
HANDLEBARS_SOURCES:=$(shell find $(WEB_SOURCE_DIR)/static/coffee -name '*.html')
LESS_SOURCES:=$(shell find $(WEB_SOURCE_DIR)/static/less -name '*.less')
LESS_MAIN:=$(WEB_SOURCE_DIR)/static/less/styles.less
CLUSTER_HTML:=$(WEB_SOURCE_DIR)/templates/cluster.html
JS_EXTERNAL_DIR:=$(WEB_SOURCE_DIR)/static/js
FONTS_EXTERNAL_DIR:=$(WEB_SOURCE_DIR)/static/fonts
IMAGES_EXTERNAL_DIR:=$(WEB_SOURCE_DIR)/static/images
FAVICON:=$(WEB_SOURCE_DIR)/favicon.ico

#### Version number handling
FORCEVERSION?=0
ifeq ($(FORCEVERSION),1)
RETHINKDB_VERSION:=$(PVERSION)
RETHINKDB_SHORT_VERSION:=$(shell echo $(RETHINKDB_VERSION) | sed 's/\([^.]\+\.[^.]\+\).*$$/\1/')
# RETHINKDB_SHORT_VERSION:=$(shell ../scripts/gen-version.sh -s)
PACKAGING_ALTERNATIVES_PRIORITY:=0
else
RETHINKDB_FALLBACK_VERSION:=$(shell if [ -e ../NOTES ] ; then cat ../NOTES | grep '^. Release' | head -n 1 | awk '{ printf "%s" , $$3 ; }' ; fi ; )
RETHINKDB_VERSION:=$(shell env FALLBACK_VERSION=$(RETHINKDB_FALLBACK_VERSION) ../scripts/gen-version.sh)
RETHINKDB_SHORT_VERSION:=$(shell echo $(RETHINKDB_VERSION) | sed 's/\([^.]\+\.[^.]\+\).*$$/\1/')
# RETHINKDB_SHORT_VERSION:=$(shell ../scripts/gen-version.sh -s)
PACKAGING_ALTERNATIVES_PRIORITY:=$(shell expr $$(../scripts/gen-version.sh -r) / 100)
endif

# ifneq ($(TIMEBOMB),0)
# RETHINKDB_VERSION:=$(RETHINKDB_VERSION)-trial
# RETHINKDB_SHORT_VERSION:=$(RETHINKDB_SHORT_VERSION)-trial
# endif
RETHINKDB_PACKAGING_VERSION?=$(RETHINKDB_VERSION)
RT_CXXFLAGS+=-DRETHINKDB_VERSION=\"$(RETHINKDB_VERSION)\"

NAMEVERSIONED?=0
ifeq ($(NAMEVERSIONED),1)
SERVER_EXEC_NAME_VERSIONED:=$(SERVER_EXEC_NAME)-$(RETHINKDB_SHORT_VERSION)
else
# update-alternatives breaks if we ditch the distinguishing mark .
SERVER_EXEC_NAME_VERSIONED:=$(SERVER_EXEC_NAME)
endif

##### Build targets
.PHONY: all build-deb build-rpm callgrind callgrind clean cscope deb deb depclean drivers etags gdb install install-binaries install-deb install-docs install-manpages install-rpm install-tools prepare_deb_package_dirs prepare_rpm_package_dirs regdb rerun revalgrind rpm rpm rpm-suse10 run sembuild showdefines style tags unit valgrind web-assets coffeelint build-deb-src-control build-deb-support

# High level build targets

# Frank removed $(BUILD_DIR)/$(START_DB_NAME) from this list .
# Note that drivers, as a phony target, only gets rebuilt consistently because it is a prerequisite of real file $(WEB_ASSETS_BUILD_DIR)/js/rethinkdb.js.

all: $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(BUILD_DIR)/$(GDB_FUNCTIONS_NAME) $(WEB_ASSETS) drivers
	@echo "    Finished building RethinkDB key-value store server version '${RETHINKDB_VERSION}'"

ifeq ($(UNIT_TESTS),1)
all: $(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME)
endif

unit:
	$(MAKE) UNIT_TESTS=1
	$(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME) --gtest_filter=$(UNIT_TEST_FILTER)

# Packaging
ifeq ($(PACKAGING),1)
PACKAGE_FOR_SUSE_10?=0
RPM_SPEC_INPUT?=../packaging/rpm.spec
DEBIAN_PKG_DIR:=debian
SUPPRESSED_LINTIAN_TAGS:=new-package-should-close-itp-bug
RPM_BUILD_ROOT=$(RPM_PACKAGE_DIR)/BUILD
DEB_CONTROL_ROOT=$(DEB_PACKAGE_DIR)/DEBIAN
RPM_SPEC_FILE=$(RPM_PACKAGE_DIR)/SPECS/rethinkdb.spec

ASSETS_DIR:=../packaging/assets

RETHINKDB_VERSION_RPM=$(subst -,_,$(RETHINKDB_PACKAGING_VERSION))
DEB_PACKAGE_REVISION?=$(shell env UBUNTU_RELEASE="$(UBUNTU_RELEASE)" DEB_RELEASE="$(DEB_RELEASE)" DEB_RELEASE_NUM="$(DEB_RELEASE_NUM)" PACKAGE_VERSION="" ../scripts/gen-trailer.sh)
RETHINKDB_VERSION_DEB=$(RETHINKDB_PACKAGING_VERSION)$(DEB_PACKAGE_REVISION)

# We need a space for the subst rule later.
wspace:=
wspace+=

PACKAGE_NAME_QUALIFIER:=

ifeq ($(BUILD_PORTABLE),1)
PACKAGE_NAME_QUALIFIER+=-portable
endif

ifeq ($(TRIAL),1)
PACKAGE_NAME_QUALIFIER+=-trial
endif

PACKAGE_NAME_QUALIFIER:=$(subst $(wspace),,$(PACKAGE_NAME_QUALIFIER))

ifeq ($(NAMEVERSIONED),1)
VERSIONED_QUALIFIED_PACKAGE_NAME:=$(PACKAGE_NAME)$(PACKAGE_NAME_QUALIFIER)-$(RETHINKDB_SHORT_VERSION)
VERSIONED_PACKAGE_NAME:=$(PACKAGE_NAME)-$(RETHINKDB_SHORT_VERSION)
# VERSIONED_TRIAL_PACKAGE_NAME:=$(TRIAL_PACKAGE_NAME)-$(RETHINKDB_SHORT_VERSION)
# Remove this line if nothing breaks.
VERSIONED_PRODUCT_SHARE_DIR:=/usr/share/$(VERSIONED_PACKAGE_NAME)
else
VERSIONED_QUALIFIED_PACKAGE_NAME:=$(PACKAGE_NAME)$(PACKAGE_NAME_QUALIFIER)
VERSIONED_PACKAGE_NAME:=$(PACKAGE_NAME)
# VERSIONED_TRIAL_PACKAGE_NAME:=$(TRIAL_PACKAGE_NAME)
# Remove this line if nothing breaks.
VERSIONED_PRODUCT_SHARE_DIR:=/usr/share/$(VERSIONED_PACKAGE_NAME)
endif


prefix?=/usr
bin_dir:=$(prefix)/bin
doc_dir:=$(prefix)/share/doc/$(VERSIONED_PACKAGE_NAME)
man_dir:=$(prefix)/share/man
man1_dir:=$(man_dir)/man1
share_dir:=$(prefix)/share/$(VERSIONED_PACKAGE_NAME)
bash_completion_dir:=/etc/bash_completion.d
internal_bash_completion_dir:=$(share_dir)$(bash_completion_dir)
scripts_dir:=$(share_dir)/scripts
init_dir:=/etc/init.d
conf_dir:=/etc/rethinkdb
conf_instance_dir:=$(conf_dir)/instances.d
lib_dir:=$(prefix)/lib/rethinkdb
web_res_dir:=$(share_dir)/web
pidfile_dir:=/var/run/rethinkdb
data_dir:=/var/lib/rethinkdb
language_drivers_dir:=$(share_dir)/drivers

ASSET_SCRIPTS:=$(ASSETS_DIR)/scripts/rdb_migrate
INIT_SCRIPTS:=$(ASSETS_DIR)/init/rethinkdb

UBUNTU_RELEASE?=

# Ubuntu quantal and later require nodejs-legacy.
ifeq ($(shell echo $(UBUNTU_RELEASE) | grep '^[q-zQ-Z]'),)
NODEJS_NEW?=0
else
NODEJS_NEW?=1
endif

# We have facilities for converting between toy names and Debian release numbers.

DEB_NUM_TO_TOY:=sed -e 's/^1$$/buzz/g' -e 's/^2$$/rex/g' -e 's/^3$$/bo/g' -e 's/^4$$/hamm/g' -e 's/^5$$/slink/g' -e 's/^6$$/potato/g' -e 's/^7$$/woody/g' -e 's/^8$$/sarge/g' -e 's/^9$$/etch/g' -e 's/^10$$/lenny/g' -e 's/^11$$/squeeze/g' -e 's/^12$$/wheezy/g' -e 's/^13$$/jessie/g'
DEB_TOY_TO_NUM:=sed -e 's/^buzz$$/1/g' -e 's/^rex$$/2/g' -e 's/^bo$$/3/g' -e 's/^hamm$$/4/g' -e 's/^slink$$/5/g' -e 's/^potato$$/6/g' -e 's/^woody$$/7/g' -e 's/^sarge$$/8/g' -e 's/^etch$$/9/g' -e 's/^lenny$$/10/g' -e 's/^squeeze$$/11/g' -e 's/^wheezy$$/12/g' -e 's/^jessie$$/13/g'
DEB_NUM_MAX:=13

# Here are unescaped versions for use in shell scripts.
# sed -e 's/^1$/buzz/g' -e 's/^2$/rex/g' -e 's/^3$/bo/g' -e 's/^4$/hamm/g' -e 's/^5$/slink/g' -e 's/^6$/potato/g' -e 's/^7$/woody/g' -e 's/^8$/sarge/g' -e 's/^9$/etch/g' -e 's/^10$/lenny/g' -e 's/^11$/squeeze/g' -e 's/^12$/wheezy/g' -e 's/^13$/jessie/g'
# sed -e 's/^buzz$/1/g' -e 's/^rex$/2/g' -e 's/^bo$/3/g' -e 's/^hamm$/4/g' -e 's/^slink$/5/g' -e 's/^potato$/6/g' -e 's/^woody$/7/g' -e 's/^sarge$/8/g' -e 's/^etch$/9/g' -e 's/^lenny$/10/g' -e 's/^squeeze$/11/g' -e 's/^wheezy$/12/g' -e 's/^jessie$/13/g'

# We can accept a toy name via DEB_RELEASE or a number via DEB_NUM_RELEASE.
# Note that the numeric release does not correspond to the Debian version but to the sequence of major releases. Version 1.3 is number 3, and version 2 is number 4.
# DEB_REAL_NUM_RELEASE is a scrubbed version of DEB_NUM_RELEASE.
# In order to provide bijective mappings, we must reject numbers that are out of bounds. So we accept 1 (buzz) to 13 (jessie) at this time.

DEB_RELEASE?=
DEB_RELEASE_NUM?=$(shell echo "$(DEB_RELEASE)" | $(DEB_TOY_TO_NUM) | grep '^[0-9]*$$')
DEB_REAL_NUM_RELEASE:=$(shell echo "$(DEB_RELEASE_NUM)" | grep '^[0-9]*$$' | awk '{ if ( ( NF >= 1 ) && ( $$1 >=1 ) && ( $$1 <= 13 ) ) { printf "%d" , $$1 ; } }' )

ifneq ($(DEB_RELEASE_NUM),)
ifeq ($(DEB_REAL_NUM_RELEASE),)
	FOO:=$(warning The Debian version specification is invalid. We will ignore it.)
endif
ifneq ($(UBUNTU_RELEASE),)
	FOO:=$(warning We seem to have received an Ubuntu release specification and a Debian release specification. We will ignore the Debian release specification.)
endif
endif



RT_CXXFLAGS+=-DMIGRATION_SCRIPT_LOCATION=\"$(scripts_dir)/rdb_migrate\"

FULL_SERVER_EXEC_NAME=$(bin_dir)/$(SERVER_EXEC_NAME)
FULL_SERVER_EXEC_NAME_VERSIONED=$(bin_dir)/$(SERVER_EXEC_NAME_VERSIONED)

prepare_deb_package_dirs:
	$(QUIET) mkdir -p $(DEB_PACKAGE_DIR)
	$(QUIET) mkdir -p $(DEB_CONTROL_ROOT)

prepare_rpm_package_dirs:
	$(QUIET) mkdir -p $(RPM_PACKAGE_DIR)
	$(QUIET) for d in BUILD RPMS/$(GCC_ARCH_REDUCED) SOURCES SPECS SRPMS; do mkdir -p $(RPM_PACKAGE_DIR)/$$d; done

install-binaries: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(QUIET) install -m755 -d $(DESTDIR)$(bin_dir)
	$(QUIET) install -m755 -T $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(DESTDIR)$(FULL_SERVER_EXEC_NAME_VERSIONED)
ifeq ($(STRIP_ON_INSTALL),1)
	$(QUIET) strip --strip-unneeded $(DESTDIR)$(FULL_SERVER_EXEC_NAME_VERSIONED)
endif

install-manpages: $(ASSETS_DIR)/man/rethinkdb.1
	$(QUIET) install -m755 -d $(DESTDIR)$(man1_dir)
# TODO: support other man pages
	$(QUIET) m4 \
			-D "SHORT_VERSION=$(RETHINKDB_SHORT_VERSION)" \
			-D "CURRENT_DATE=$(shell date +%F)" \
			< $(ASSETS_DIR)/man/rethinkdb.1 | gzip -9 | install -m644 -T /dev/stdin $(DESTDIR)$(man1_dir)/$(VERSIONED_PACKAGE_NAME).1.gz;
#	$(QUIET) for manpage in $(MAN_PAGES); do \
#			gzip -9 < $$manpage | install -m644 -T /dev/stdin $(DESTDIR)$(man1_dir)/$$(basename $$manpage).gz; \
#		done

install-tools: $(ASSETS_DIR)/scripts/rethinkdb.bash $(ASSET_SCRIPTS)
	$(QUIET) install -m755 -d $(DESTDIR)$(internal_bash_completion_dir)
	$(QUIET) install -m755 -d $(DESTDIR)$(bash_completion_dir)
	$(QUIET) m4 																												\
			-D "SERVER_EXEC_NAME=$(SERVER_EXEC_NAME)" 											\
			-D "SERVER_EXEC_NAME_VERSIONED=$(SERVER_EXEC_NAME_VERSIONED)" 	\
			$(ASSETS_DIR)/scripts/rethinkdb.bash | install -m644 /dev/stdin $(DESTDIR)$(internal_bash_completion_dir)/$(SERVER_EXEC_NAME).bash
	$(QUIET) install -m755 -d $(DESTDIR)$(scripts_dir)
	$(QUIET) for s in $(ASSET_SCRIPTS); do install -m755 "$$s" $(DESTDIR)$(scripts_dir)/$$(basename $$s); done
	$(QUIET) install -m755 -d $(DESTDIR)$(init_dir) ;
	$(QUIET) for s in $(INIT_SCRIPTS); do install -m755 "$$s" $(DESTDIR)$(init_dir)/$$(basename $$s); done ;

install-config:
	$(QUIET) install -m755 -d $(DESTDIR)$(conf_dir) ;
	$(QUIET) install -m755 -d $(DESTDIR)$(conf_instance_dir) ;
	$(QUIET) install -m644 $(ASSETS_DIR)/config/default.conf.sample $(DESTDIR)$(conf_dir)/default.conf.sample ;

install-data:
	$(QUIET) install -m755 -d $(DESTDIR)$(data_dir)
	$(QUIET) install -m755 -d $(DESTDIR)$(data_dir)/instances.d
# We disabled the /var/run creation and added the functionality to the init script because Lintian observed that /var/run can get purged .
#	$(QUIET) install -m755 -d $(DESTDIR)$(pidfile_dir)
#	$(QUIET) install -m755 -d $(DESTDIR)$(pidfile_dir)/instances.d

install-web:
	$(QUIET) install -m755 -d $(DESTDIR)$(web_res_dir)
# This might break some ownership or permissions stuff.
	$(QUIET) cp -pRP $(BUILD_DIR)/web/* $(DESTDIR)$(web_res_dir)/

install-drivers:

$(libdir):
	$(QUIET) install -m755 -d $(DESTDIR)$(lib_dir)

install-docs:
	$(QUIET) install -m755 -d $(DESTDIR)$(doc_dir)
	$(QUIET) install -m644 -T $(ASSETS_DIR)/docs/LICENSE $(DESTDIR)$(doc_dir)/copyright

install: install-binaries install-manpages install-docs install-tools install-web install-data install-config
install-rpm: install
install-deb: install
	$(QUIET) install -m755 -d $(DESTDIR)$(doc_dir)
	$(QUIET) sed	-e 's/PACKAGING_VERSION/$(RETHINKDB_VERSION_DEB)/' $(ASSETS_DIR)/docs/changelog.Debian | \
		gzip -c9 | \
		install -m644 -T /dev/stdin $(DESTDIR)$(doc_dir)/changelog.Debian.gz

ifeq ($(BUILD_PORTABLE),1)
build-deb-support: $(TC_LESSC_INT_EXE) $(TC_COFFEE_INT_EXE) $(TC_HANDLEBARS_INT_EXE) $(V8_SRC_DIR) $(NODE_SRC_DIR) $(PROTOC_SRC_DIR) $(GPERFTOOLS_SRC_DIR) $(LIBUNWIND_SRC_DIR)
else
build-deb-support: $(TC_LESSC_INT_EXE) $(TC_COFFEE_INT_EXE)
endif

build-deb-src-control:
	$(QUIET) cd .. ; if [ -e debian ] ; then rm -rf debian ; fi ; cp -pRP packaging/debian.template debian ; env UBUNTU_RELEASE=$(UBUNTU_RELEASE) DEB_RELEASE=$(DEB_RELEASE) DEB_RELEASE_NUM=$(DEB_RELEASE_NUM) PACKAGE_NAME=$(PACKAGE_NAME) VERSIONED_PACKAGE_NAME=$(VERSIONED_PACKAGE_NAME) VERSIONED_QUALIFIED_PACKAGE_NAME=$(VERSIONED_QUALIFIED_PACKAGE_NAME) VANILLA_PACKAGE_NAME=$(VANILLA_PACKAGE_NAME) PACKAGE_VERSION_EXTENDED=$(RETHINKDB_VERSION_DEB) PACKAGE_VERSION=$(RETHINKDB_PACKAGING_VERSION) ./scripts/gen-changelog.sh ;
# Replace version/size fields in the deb control file
	$(QUIET) disk_size=0;	\
		m4																													\
				-D "SOLO=$(SOLO)"																				\
				-D "PACKAGE_NAME=$(PACKAGE_NAME)"												\
				-D "VERSIONED_PACKAGE_NAME=$(VERSIONED_PACKAGE_NAME)"		\
				-D "VANILLA_PACKAGE_NAME=$(VANILLA_PACKAGE_NAME)"				\
				-D "TRIAL_PACKAGE_NAME=$(TRIAL_PACKAGE_NAME)"						\
				-D "VERSIONED_TRIAL_PACKAGE_NAME=$(VERSIONED_TRIAL_PACKAGE_NAME)"						\
				-D "VERSIONED_QUALIFIED_PACKAGE_NAME=$(VERSIONED_QUALIFIED_PACKAGE_NAME)" \
				-D "PACKAGE_VERSION=$(RETHINKDB_VERSION_DEB)" \
				-D "LEGACY_PACKAGE=$(LEGACY_PACKAGE)" \
				-D "STATIC_V8=$(STATIC_V8)" \
				-D "TC_BUNDLED=$(BUILD_PORTABLE)" \
				-D "BUILD_DRIVERS=$(BUILD_DRIVERS)" \
				-D "TRIAL=$(TRIAL)" \
				-D "DISK_SIZE=$${disk_size}" \
				-D "SOURCEBUILD=1" \
				-D "NODEJS_NEW=$(NODEJS_NEW)" \
				-D "CURRENT_ARCH=$(DEB_ARCH)" \
			../packaging/$(DEBIAN_PKG_DIR)/control > ../debian/control
# Copy {pre,post}{inst,rm} scripts
	$(QUIET) for script in preinst postinst prerm postrm; do		                  		\
			m4 \
					-D "BIN_DIR=$(bin_dir)" \
					-D "MAN1_DIR=$(man1_dir)" \
					-D "BASH_COMPLETION_DIR=$(bash_completion_dir)" \
					-D "INTERNAL_BASH_COMPLETION_DIR=$(internal_bash_completion_dir)" \
					-D "SERVER_EXEC_NAME=$(SERVER_EXEC_NAME)" \
					-D "SERVER_EXEC_NAME_VERSIONED=$(SERVER_EXEC_NAME_VERSIONED)" \
					-D "UPDATE_ALTERNATIVES=$(NAMEVERSIONED)" \
					-D "PRIORITY=$(PACKAGING_ALTERNATIVES_PRIORITY)" \
				../packaging/$(DEBIAN_PKG_DIR)/$${script} > ../debian/$${script}; \
			chmod 0755 ../debian/$${script};																		\
		done


# Copy the license file.
	cat ../packaging/$(DEBIAN_PKG_DIR)/copyright > ../debian/copyright

build-deb: all prepare_deb_package_dirs
	@echo
	@echo "$(ANSI_BOLD_ON)Building DEB package for RethinkDB version $(RETHINKDB_PACKAGING_VERSION)$(ANSI_BOLD_OFF)"
	@echo
# Copy files to their correct places
	$(QUIET) $(MAKE) DESTDIR=$(DEB_PACKAGE_DIR) install-deb
# Produce md5sum file in control directory
	$(QUIET) find $(DEB_PACKAGE_DIR) -path $(DEB_CONTROL_ROOT) -prune -o -path $(DEB_PACKAGE_DIR)/etc -prune -o -type f -printf "%P\\0" | \
		(cd $(DEB_PACKAGE_DIR) && xargs -0 md5sum) \
		> $(DEB_CONTROL_ROOT)/md5sums
# List configuration files in conffiles
	$(QUIET) find $(DEB_PACKAGE_DIR) -type f -printf "/%P\n" | (grep '^/etc/' | grep -v '^/etc/init\.d' || true) > $(DEB_CONTROL_ROOT)/conffiles
# Copy {pre,post}{inst,rm} scripts
	$(QUIET) for script in preinst postinst prerm postrm; do		                  		\
			m4 \
					-D "BIN_DIR=$(bin_dir)" \
					-D "MAN1_DIR=$(man1_dir)" \
					-D "BASH_COMPLETION_DIR=$(bash_completion_dir)" \
					-D "INTERNAL_BASH_COMPLETION_DIR=$(internal_bash_completion_dir)" \
					-D "SERVER_EXEC_NAME=$(SERVER_EXEC_NAME)" \
					-D "SERVER_EXEC_NAME_VERSIONED=$(SERVER_EXEC_NAME_VERSIONED)" \
					-D "UPDATE_ALTERNATIVES=$(NAMEVERSIONED)" \
					-D "PRIORITY=$(PACKAGING_ALTERNATIVES_PRIORITY)" \
				../packaging/$(DEBIAN_PKG_DIR)/$${script} > $(DEB_CONTROL_ROOT)/$${script}; \
			chmod 0755 $(DEB_CONTROL_ROOT)/$${script};																		\
		done
# Replace version/size fields in the deb control file
	$(QUIET) disk_size=$$(du -s -k $(DEB_PACKAGE_DIR) | cut -f1);	\
		m4																													\
				-D "SOLO=$(SOLO)"																				\
				-D "PACKAGE_NAME=$(PACKAGE_NAME)"												\
				-D "VERSIONED_PACKAGE_NAME=$(VERSIONED_PACKAGE_NAME)"		\
				-D "VANILLA_PACKAGE_NAME=$(VANILLA_PACKAGE_NAME)"				\
				-D "TRIAL_PACKAGE_NAME=$(TRIAL_PACKAGE_NAME)"						\
				-D "VERSIONED_TRIAL_PACKAGE_NAME=$(VERSIONED_TRIAL_PACKAGE_NAME)"						\
				-D "PACKAGE_VERSION=$(RETHINKDB_VERSION_DEB)"						\
				-D "VERSIONED_QUALIFIED_PACKAGE_NAME=$(VERSIONED_QUALIFIED_PACKAGE_NAME)" \
				-D "LEGACY_PACKAGE=$(LEGACY_PACKAGE)"												\
				-D "STATIC_V8=$(STATIC_V8)" \
				-D "TC_BUNDLED=$(BUILD_PORTABLE)" \
				-D "BUILD_DRIVERS=$(BUILD_DRIVERS)" \
				-D "TRIAL=$(TRIAL)"																			\
				-D "DISK_SIZE=$${disk_size}"														\
				-D "SOURCEBUILD=0" \
				-D "NODEJS_NEW=$(NODEJS_NEW)" \
				-D "CURRENT_ARCH=$(DEB_ARCH)" \
			../packaging/$(DEBIAN_PKG_DIR)/control >$(DEB_CONTROL_ROOT)/control
# Copy the license file.
	cat ../packaging/$(DEBIAN_PKG_DIR)/copyright > $(DEB_CONTROL_ROOT)/copyright
# Finally, produce a debian package
	$(QUIET) fakeroot dpkg-deb -b $(DEB_PACKAGE_DIR) $(PACKAGES_DIR)

deb: prepare_deb_package_dirs
# We have disabled building of unstripped debs for now.
# $(QUIET) $(MAKE) STRIP_ON_INSTALL=0 RETHINKDB_PACKAGING_VERSION=$(RETHINKDB_VERSION)-unstripped build-deb
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=1 RETHINKDB_PACKAGING_VERSION=$(RETHINKDB_VERSION) WEBRESDIR=$(web_res_dir) BUILD_DRIVERS=0 build-deb
# Print a nice message with the location of the created package
	@echo
	@echo "$(ANSI_BOLD_ON)Your DEB packages are here (with lintian output following each package):$(ANSI_BOLD_OFF)"
ifeq ($(LINT),1)
	$(QUIET) for f in $(PACKAGES_DIR)/*.deb; do \
			deb_name=`readlink -f $(PACKAGES_DIR)`/$$(basename $$f); \
			echo "  $(ANSI_UL_ON)$${deb_name}$(ANSI_UL_OFF)"; \
			lintian --color auto --suppress-tags "no-copyright-file,$(subst $(space),$(comma),$(SUPPRESSED_LINTIAN_TAGS))" $${deb_name} || true; \
		done
else
	$(QUIET) for f in $(PACKAGES_DIR)/*.deb; do \
			deb_name=`readlink -f $(PACKAGES_DIR)`/$$(basename $$f); \
			echo "  $(ANSI_UL_ON)$${deb_name}$(ANSI_UL_OFF)"; \
		done
endif
	$(QUIET) echo

build-rpm: all prepare_rpm_package_dirs
	@echo
	@echo "$(ANSI_BOLD_ON)Building RPM package for RethinkDB version $(RETHINKDB_PACKAGING_VERSION)$(ANSI_BOLD_OFF)"
	@echo
# Replace version/topdir fields in the spec file
	$(QUIET) m4 \
				-D "SOLO=$(SOLO)" \
				-D "RPM_PACKAGE_DIR=`readlink -f $(RPM_PACKAGE_DIR)`" \
				-D "SERVER_EXEC_NAME=$(SERVER_EXEC_NAME)" \
				-D "SERVER_EXEC_NAME_VERSIONED=$(SERVER_EXEC_NAME_VERSIONED)" \
				-D "PACKAGE_NAME=$(PACKAGE_NAME)" \
				-D "VERSIONED_PACKAGE_NAME=$(VERSIONED_PACKAGE_NAME)" \
				-D "VERSIONED_QUALIFIED_PACKAGE_NAME=$(VERSIONED_QUALIFIED_PACKAGE_NAME)" \
				-D "VANILLA_PACKAGE_NAME=$(VANILLA_PACKAGE_NAME)" \
				-D "TRIAL_PACKAGE_NAME=$(TRIAL_PACKAGE_NAME)" \
				-D "VERSIONED_TRIAL_PACKAGE_NAME=$(VERSIONED_TRIAL_PACKAGE_NAME)" \
				-D "PACKAGE_VERSION=$(RETHINKDB_VERSION_DEB)" \
				-D "PACKAGE=$(PACKAGE)" \
				-D "PACKAGE_FOR_SUSE_10=$(PACKAGE_FOR_SUSE_10)" \
				-D "TRIAL=$(TRIAL)" \
				-D "BIN_DIR=$(bin_dir)" \
				-D "DOC_DIR=$(doc_dir)" \
				-D "MAN1_DIR=$(man1_dir)" \
				-D "SHARE_DIR=$(share_dir)" \
				-D "WEB_RES_DIR=$(web_res_dir)" \
				-D "BASH_COMPLETION_DIR=$(bash_completion_dir)" \
				-D "INTERNAL_BASH_COMPLETION_DIR=$(internal_bash_completion_dir)" \
				-D "SCRIPTS_DIR=$(scripts_dir)" \
				-D "PRIORITY=$(PACKAGING_ALTERNATIVES_PRIORITY)" \
			$(RPM_SPEC_INPUT) >$(RPM_SPEC_FILE)
# Copy files to their correct places
	$(QUIET) $(MAKE) DESTDIR=$(RPM_BUILD_ROOT) install-rpm
# Produce RPM package
# FIXME: write rpmbuild output out to stderr if there's an error
	$(QUIET) rpmbuild -bb --target=$(GCC_ARCH_REDUCED) --buildroot `readlink -f $(RPM_BUILD_ROOT)` $(RPM_SPEC_FILE) > $(RPM_PACKAGE_DIR)/rpmbuild.stdout

# RPM for redhat and centos
rpm: all prepare_rpm_package_dirs
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=0 WEBRESDIR=$(web_res_dir) RETHINKDB_VERSION=$(RETHINKDB_VERSION)-unstripped build-rpm
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=1 WEBRESDIR=$(web_res_dir) RETHINKDB_VERSION=$(RETHINKDB_VERSION) build-rpm
# Print a nice message with the location of the created package
	@echo
	@echo "$(ANSI_BOLD_ON)Your RPM packages are here:$(ANSI_BOLD_OFF)"
	$(QUIET) for f in $(RPM_PACKAGE_DIR)/RPMS/$(GCC_ARCH_REDUCED)/*.rpm; do	                \
			rpm_name=`readlink -f $(PACKAGES_DIR)`/$$(basename $$f);	\
			mv $$f $$rpm_name;						\
			echo "  $(ANSI_UL_ON)$${rpm_name}$(ANSI_UL_OFF)";		\
		done
	$(QUIET) echo

else
# Ubuntu/Debian
deb:
	$(QUIET) $(MAKE) PACKAGING=1 DEBUG=0 deb

# RPM for CentOS/RedHat
rpm: clean
	$(QUIET) $(MAKE) PACKAGING=1 DEBUG=0 rpm

# RPM for Suse 10
rpm-suse10:
	$(QUIET) $(MAKE) PACKAGE_FOR_SUSE_10=1 rpm

build-deb-src-control:
	$(QUIET) $(MAKE) PACKAGING=1 build-deb-src-control

endif

.PHONY: sense
sense:
	$(error Make does not make sense)

# Profiling
OPROF_TARGETS=oprof-start oprof-dump oprof-stop oprof-report oprof-build
.PHONY: $(OPROF_TARGETS)

oprof-build:
	$(MAKE) DEBUG=0 SYMBOLS=1 NO_OMIT_FRAME_POINTER=1

oprof-start: OPROF_NO_KERNEL:=0
oprof-start: OPROF_SESSION:=../oprofile.$(RETHINKDB_VERSION).$(shell date +%F-%T)
oprof-start:
	@echo no kernel: $(OPROF_NO_KERNEL)
ifeq ($(OPROF_NO_KERNEL),1)
	@echo "    OPROFILE[init] (no kernel profiling)"
	$(QUIET) opcontrol --no-vmlinux
else
	@echo "    OPROFILE[init] (with kernel profiling, set OPROF_NO_KERNEL to 1 to turn off)"
	$(QUIET) opcontrol --vmlinux=/usr/lib/debug/boot/vmlinux-`uname -r`
endif
	@echo "    OPROFILE[start] -> $(abspath $(OPROF_SESSION))"
	@echo "      (if you want to use a different location, set OPROF_SESSION make variable to a directory path"
	$(QUIET) opcontrol --start --callgraph=2 --event=CPU_CLK_UNHALTED:90000:0:1:1 --buffer-size=10485760 --buffer-watershed=524288 "--session-dir=$(abspath $(OPROF_SESSION))"

oprof-dump:
	@echo "    OPROFILE[dump]"
	$(QUIET) opcontrol --dump

oprof-stop: oprof-dump
	@echo "    OPROFILE[shutdown]"
	$(QUIET) opcontrol --shutdown

oprof-report: OPROF_SESSION:=$(shell ls -dt ../oprofile.* 2> /dev/null | head -1)
oprof-report: OPROF_BINARY:=../build/release/rethinkdb
oprof-report: OPROF_RESULT:=$(OPROF_SESSION)/report.txt
oprof-report:
	@if [ ! -d "$(OPROF_SESSION)" ]; then \
			echo "error: OPROF_SESSION is not set and no oprofile sessions could be found in the repository root."; \
			exit 1; \
		fi
	@if [ ! -x "$(OPROF_BINARY)" ]; then \
			echo "error: OPROF_BINARY is not set or '$(OPROF_BINARY)' is not an executable."; \
			exit 1; \
		fi
	@echo "    OPREPORT[$(OPROF_BINARY)] -> $(OPROF_RESULT)"
	$(QUIET) opreport --merge=lib,unitmask -a --symbols --callgraph --threshold 1 --sort sample "--session-dir=$(realpath $(OPROF_SESSION))" -l "$(realpath $(OPROF_BINARY))" > $(OPROF_RESULT)

drivers:
ifeq ($(BUILD_DRIVERS), 1)
	$(QUIET) cd ../drivers ; $(MAKE) PROTOC_BASE="$(PROTOC_BASE)" TC_PROTOC_CFLAGS="$(TC_PROTOC_CFLAGS)" ;
else
	$(QUIET) cd ../drivers/javascript ; $(MAKE) PROTOC_BASE="$(PROTOC_BASE)" TC_PROTOC_CFLAGS="$(TC_PROTOC_CFLAGS)" ;
endif #BUILD_DRIVERS


$(WEB_ASSETS_BUILD_DIR)/js/rethinkdb.js: drivers $(WEB_ASSETS_BUILD_DIR)/js
	$(QUIET) cp -pRP ../build/drivers/javascript/rethinkdb.js $(WEB_ASSETS_BUILD_DIR)/js/rethinkdb.js ;

rpc/semilattice/joins/macros.hpp: ../scripts/generate_join_macros.py
rpc/serialize_macros.hpp: ../scripts/generate_serialize_macros.py
rpc/mailbox/typed.hpp: ../scripts/generate_rpc_templates.py

rpc/semilattice/joins/macros.hpp rpc/serialize_macros.hpp rpc/mailbox/typed.hpp:
	$< > $@

web-assets: $(WEB_ASSETS)

$(WEB_ASSETS_BUILD_DIR)/js/template.js: $(WEB_SOURCE_DIR)/static/handlebars $(TC_HANDLEBARS_EXE)
	$(QUIET) env TC_HANDLEBARS_EXE=$(TC_HANDLEBARS_EXE) ../scripts/build_handlebars_templates.sh $(WEB_SOURCE_DIR)/static/handlebars $(BUILD_DIR) $(WEB_ASSETS_BUILD_DIR)/js

$(WEB_ASSETS_OBJ_DIR)/cluster-min.concat.coffee: $(COFFEE_SOURCES)
	$(QUIET) mkdir -p $(WEB_ASSETS_OBJ_DIR)
ifeq ($(VERBOSE),0)
	@echo "    CONCAT $@"
endif
	$(QUIET) cat $+ > $@ || ( echo "      Build failure." ; rm $@ ; false ; )

$(WEB_ASSETS_BUILD_DIR)/cluster-min.js: $(WEB_ASSETS_OBJ_DIR)/cluster-min.concat.coffee $(TC_COFFEE_EXE) $(TC_NODE_EXE)
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    COFFEE $@"
endif
	$(QUIET) $(NODE_EXE_SPEC)$(TC_COFFEE_EXE) -bp --stdio < $(WEB_ASSETS_OBJ_DIR)/cluster-min.concat.coffee > $@ || ( echo "      Build failure." ; rm $@ ; false ; )

$(WEB_ASSETS_BUILD_DIR)/cluster.css: $(LESS_MAIN) $(TC_LESSC_EXE) $(TC_NODE_EXE)
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    LESSC $@"
endif
	$(QUIET) $(NODE_EXE_SPEC)$(TC_LESSC_EXE) $(LESS_MAIN) > $@ || ( echo "      Build failure." ; rm $@ ; false ; )

$(WEB_ASSETS_BUILD_DIR)/index.html: $(CLUSTER_HTML) $(HANDLEBARS_SOURCES)
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
	cp $(CLUSTER_HTML) $@

$(WEB_ASSETS_BUILD_DIR)/js:
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    CP $(JS_EXTERNAL_DIR) -> $(WEB_ASSETS_BUILD_DIR)"
endif
	$(QUIET) cp -RP $(JS_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)

$(WEB_ASSETS_BUILD_DIR)/fonts:
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    CP $(FONTS_EXTERNAL_DIR) -> $(WEB_ASSETS_BUILD_DIR)"
endif
	$(QUIET) cp -RP $(FONTS_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)

$(WEB_ASSETS_BUILD_DIR)/images:
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    CP $(IMAGES_EXTERNAL_DIR) -> $(WEB_ASSETS_BUILD_DIR)"
endif
	$(QUIET) cp -RP $(IMAGES_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)

$(WEB_ASSETS_BUILD_DIR)/favicon.ico: $(FAVICON)
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    CP $(FAVICON) -> $(WEB_ASSETS_BUILD_DIR)"
endif
	$(QUIET) cp -P $(FAVICON) $(WEB_ASSETS_BUILD_DIR)

# Special recipe for the proto buffer generated code
# Need to use a dummy file to avoid calling protoc once for each input file.
$(PROTO_DIR)/dummy: $(PROTO_SOURCES) $(TC_PROTOC_EXE)
	$(QUIET) mkdir -p $(PROTO_DIR)
ifeq ($(VERBOSE),0)
	@echo "    PROTOC[CPP] $^"
endif
	$(QUIET) $(TC_PROTOC_RUN) $(PROTOCFLAGS) --cpp_out $(PROTO_DIR) $(PROTO_SOURCES) && touch $@
# The preceding line used $^ instead of $(PROTO_SOURCES) before.
# The touch command was on a separate line, but that caused problems since make ignored the failure of that line and touched the file anyway.

$(PROTO_HEADERS) $(PROTO_CODE): $(PROTO_DIR)/dummy

$(BUILD_DIR)/$(SERVER_EXEC_NAME): $(OBJS) $(BUILD_DIR) $(OBJ_SUPPS)
ifeq ($(VERBOSE),0)
	@echo "    LD $@"
endif
ifeq ($(IGNORE_SOME_ICC_LD_WARNINGS),0)
	$(QUIET) $(RT_CXX) $(RT_LDFLAGS) $(SERVER_EXEC_OBJS) $(STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_EXEC_NAME)
else
# The following line runs CXX in linking mode (supplying only the object files and libraries) and then
# pass the output through a filter which filters out bogus (?) icc-related problems.
# FIXME: figure out why we get these problems
	$(QUIET) ($(RT_CXX) $(RT_LDFLAGS) $(SERVER_EXEC_OBJS) $(STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_EXEC_NAME) 2>&1 >&3 | (grep -v "warning: relocation refers to discarded section" || true) >&2) 3>&1
endif
ifeq ($(NO_TCMALLOC),0)
	@objdump -T $(BUILD_DIR)/$(SERVER_EXEC_NAME) | c++filt | grep -q 'tcmalloc::\|google_malloc' || \
		(echo "    Failed to link in TCMalloc. Either install it, or make with NO_TCMALLOC=1." && \
		false)
endif



# The unittests use gtest, which uses macros that expand into switch statements which don't contain
# default cases. So we have to remove the -Wswitch-default argument for them.
$(OBJ_DIR)/unittest/%.o: RT_CXXFLAGS:=$(filter-out -Wswitch-default,$(RT_CXXFLAGS))

$(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME): $(OBJS) $(BUILD_DIR) $(OBJ_SUPPS)
ifeq ($(VERBOSE),0)
	@echo "    LD $@"
endif
ifeq ($(IGNORE_SOME_ICC_LD_WARNINGS),0)
	$(QUIET) $(RT_CXX) $(RT_LDFLAGS) $(SERVER_UNIT_TEST_OBJS) $(STATIC_LIBRARY_PATHS) $(UNIT_STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME)
else
# Note: that is the copy of the problem description that we have above in server linking section.
# The following line runs CXX in linking mode (supplying only the object files and libraries) and then
# pass the output through a filter which filters out bogus (?) icc-related problems.
# FIXME: figure out why we get these problems
	$(QUIET) ($(RT_CXX) $(RT_LDFLAGS) $(SERVER_UNIT_TEST_OBJS) $(STATIC_LIBRARY_PATHS) $(UNIT_STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME) 2>&1 >&3 | (grep -v "warning: relocation refers to discarded section" || true) >&2) 3>&1
endif

$(BUILD_DIR)/$(START_DB_NAME):
	$(QUIET) cp $(SCRIPTS_DIR)/$(START_DB_NAME) $(BUILD_DIR)/$(START_DB_NAME)

$(BUILD_DIR)/$(GDB_FUNCTIONS_NAME):
	$(QUIET) cp $(SCRIPTS_DIR)/$(GDB_FUNCTIONS_NAME) $(BUILD_DIR)/$(GDB_FUNCTIONS_NAME)

# The 'run' and 'gdb' targets build the server, automatically remove old data files, and then start
# it up. The difference is that the 'gdb' target starts it under a debugger. The 're*' targets are
# like the similarly-named targets except that they don't remove old data files.

run: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(QUIET) rm -f $(RUN_FILE)
	$(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) -f $(RUN_FILE) $(RUN_FLAGS)

rerun: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) -f $(RUN_FILE) $(RUN_FLAGS)

gdb: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(QUIET) rm -f $(RUN_FILE)
	cgdb --args $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) -f $(RUN_FILE) $(RUN_FLAGS)

regdb: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	cgdb --args $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) -f $(RUN_FILE) $(RUN_FLAGS)

sembuild: clean
	make SEMANTIC_SERIALIZER_CHECK=1 all

valgrind: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
ifeq ($(VALGRIND),0)
	$(error Using the 'valgrind' target doesn't automatically set VALGRIND to 1. Try again with \
'make valgrind VALGRIND=1')
endif
	valgrind $(VALGRIND_FLAGS) $(BUILD_DIR)/$(SERVER_EXEC_NAME) serve --cluster-port $(RUN_PORT)

revalgrind: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
ifeq ($(VALGRIND),0)
	$(error Using the 'revalgrind' target doesn't automatically set VALGRIND to 1. Try again with \
'make valgrind VALGRIND=1')
endif
	valgrind $(VALGRIND_FLAGS) $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) -f $(RUN_FILE) $(RUN_FLAGS)

ifeq ($(CALLGRIND),0)
callgrind:
	$(error "Using the 'callgrind' target doesn't automatically set CALLGRIND to 1. Try again with \
'make calgrind CALLGRIND=1'")
else
callgrind: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(QUIET) rm -f $(RUN_FILE)
	valgrind --tool=callgrind $(CALLGRIND_FLAGS) $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) -f $(RUN_FILE) $(RUN_FLAGS)
endif

TAGFLAGS:=-R --c++-kinds=+p --fields=+iaS --extra=+q --langmap="c++:.cc.tcc.hpp"
tags:
	$(QUIET) $(CTAGSPROG) $(TAGFLAGS) -f $(TAGSFILE)

etags:
	$(QUIET) rm -f $(ETAGSFILE)
	$(QUIET) $(ETAGSPROG) $(TAGFLAGS) -f $(ETAGSFILE)

cscope:
	$(QUIET) cscope -bR -f $(CSCOPE_XREF)

analyze: $(SOURCES)
	$(QUIET) clang --analyze $(RT_CXXFLAGS) $(SOURCES)

coffeelint:
	-coffeelint -f ../scripts/coffeelint.json -r ../admin/

style: coffeelint
	$(QUIET) ../scripts/check_style.sh

showdefines:
	$(QUIET) $(RT_CXX) $(RT_CXXFLAGS) -m32 -E -dM - < /dev/null

depclean:
ifeq ($(VERBOSE),0)
	@echo "    RM -f $(BUILD_ROOT_DIR)/*.d"
endif
	$(QUIET) if test -d $(BUILD_ROOT_DIR); then find $(BUILD_ROOT_DIR) -name '*.d' -exec rm {} \; ; fi

clean: clean-drivers
ifeq ($(VERBOSE),0)
	@echo "    RM *~"
	@echo "    RM -r $(BUILD_ROOT_DIR)"
endif
	$(QUIET) find . -name '*~' -exec rm {} \;
	$(QUIET) rm -rf $(BUILD_ROOT_DIR)

clean-drivers:
	$(QUIET) cd ../drivers ; $(MAKE) clean ;

# Directories
$(BUILD_DIR):
	$(QUIET) mkdir -p $(BUILD_DIR)

# Object files

# Proto buffers object files
# This is hacky, as the following rule is just a duplicate of the rule for our own sources
$(OBJ_DIR)/%.pb.o:  $(PROTO_DIR)/%.pb.cc $(MAKEFILE_DEPENDENCY) $(PROTO_HEADERS)
	$(QUIET) mkdir -p $(dir $@)
ifeq ($(VERBOSE),0)
	@echo "    CC $< -o $@"
endif
	$(QUIET) $(RT_CXX) $(RT_CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cc $(MAKEFILE_DEPENDENCY) $(V8_DEP)
	$(QUIET) mkdir -p $(dir $@)
ifeq ($(VERBOSE),0)
	@echo "    CC $< -o $@"
endif
	$(QUIET) $(RT_CXX) $(RT_CXXFLAGS) -c -o $@ $<

# Dependencies
$(DEP_DIR)/%.d: $(SOURCE_DIR)/%.cc $(PROTO_HEADERS) $(V8_DEP)
ifeq ($(VERBOSE),0)
	@echo "    DEP $@"
endif
	$(QUIET) mkdir -p $(dir $@)
	$(QUIET) $(RT_CXX) $(RT_CXXFLAGS) -M -MQ $(OBJ_DIR)/$*.o -MQ $@ $< > $@ || ( echo "      Build failure." ; rm $@ ; false ; )

# Without this, the proto headers get generated to build the .d files, then are removed as
# intermediate files; then the .d files are read in, and some depend on the proto headers, so they
# get remade. This is redundant work. Also, it appears to cause nondeterministic races - possibly a
# bug in GNU Make, possibly a bug in our Makefile.
.SECONDARY: $(PROTO_HEADERS)

NO_DEPS_TARGETS:=clean depclean tags etags analyze cscope style web-assets build-deb-src-control build-deb-support $(WEB_ASSETS_BUILD_DIR)/cluster-min.js $(WEB_ASSETS_BUILD_DIR)/cluster.css $(WEB_ASSETS_BUILD_DIR)/index.html $(WEB_ASSETS_BUILD_DIR)/js $(WEB_ASSETS_BUILD_DIR)/fonts $(WEB_ASSETS_BUILD_DIR)/images $(WEB_ASSETS_BUILD_DIR)/favicon.ico $(WEB_ASSETS_BUILD_DIR)/js/rethinkdb.js $(WEB_ASSETS_OBJ_DIR)/cluster-min.concat.coffee $(COFFEE_SOURCES) $(PROTO_DIR)/dummy $(PROTO_SOURCES) $(PROTO_HEADERS) $(PROTO_CODE) $(PROTO_DIR)/dummy $(OPROF_TARGETS) $(TC_LESSC_INT_EXE) $(TC_COFFEE_INT_EXE) $(TC_HANDLEBARS_INT_EXE) $(SUPPORT_DIR) $(V8_SRC_DIR) $(V8_DIR) $(V8_LIB) $(NODE_SRC_DIR) $(NODE_DIR) $(TC_NODE_INT_EXE) $(TC_NPM_INT_EXE) $(PROTOC_SRC_DIR) $(PROTOC_DIR) $(TC_PROTOC_INT_EXE) $(GPERFTOOLS_SRC_DIR) $(GPERFTOOLS_DIR) $(LIBUNWIND_SRC_DIR) $(LIBUNWIND_DIR) $(TCMALLOC_MINIMAL_INT_LIB)
# Include the dependencies into the makefile so that they take effect
ifneq (,$(if $(MAKECMDGOALS),$(filter-out $(NO_DEPS_TARGETS),$(MAKECMDGOALS)),non-empty-placeholder))
-include $(DEPS)
endif

# JavaScript tools from node.js

# Note that this assumes that its locations match those earlier in the Makefile. If you change TC_LESSC_EXE, you probably ought to change these as well .

$(TC_LESSC_INT_EXE): $(TC_NPM_EXE)
ifeq ($(FETCH_INTERNAL_TOOLS),1)
# ifeq ($(shell npm list --global --parseable | grep '\/less$$'),)
# We must do this either way now.
	$(QUIET) if [ ! -e $(SUPPORT_DIR) ] ; then mkdir -p $(SUPPORT_DIR) ; fi ;
	$(QUIET) if [ -d $(SUPPORT_DIR) ] && [ ! -e $(SUPPORT_DIR)/toolchain ] ; then mkdir -p $(SUPPORT_DIR)/toolchain ; fi ;
	@echo "    NPM-I coffee-script"
	$(QUIET) cd $(SUPPORT_DIR)/toolchain ; $(TC_NPM_EXE) install less ;
# endif
else
	@ if [ ! -e $(TC_LESSC_INT_EXE) ] ; then echo "    ERROR: Internal lessc is necessary but not present, and FETCH_INTERNAL_TOOLS != 1." ; false ; fi ;
endif

$(TC_COFFEE_INT_EXE): $(TC_NPM_EXE)
ifeq ($(FETCH_INTERNAL_TOOLS),1)
# ifeq ($(shell npm list --global --parseable | grep '\/coffee-script$$'),)
	$(QUIET) if [ ! -e $(SUPPORT_DIR) ] ; then mkdir -p $(SUPPORT_DIR) ; fi ;
	$(QUIET) if [ -d $(SUPPORT_DIR) ] && [ ! -e $(SUPPORT_DIR)/toolchain ] ; then mkdir -p $(SUPPORT_DIR)/toolchain ; fi ;
	@echo "    NPM-I coffee-script"
	$(QUIET) cd $(SUPPORT_DIR)/toolchain ; $(TC_NPM_EXE) install coffee-script ;
# endif
else
	@ if [ ! -e $(TC_COFFEE_INT_EXE) ] ; then echo "    ERROR: Internal coffee-script is necessary but not present, and FETCH_INTERNAL_TOOLS != 1." ; false ; fi ;
endif

$(TC_HANDLEBARS_INT_EXE): $(TC_NPM_EXE)
ifeq ($(FETCH_INTERNAL_TOOLS),1)
# ifeq ($(shell npm list --global --parseable | grep '\/handlebars$$'),)
	$(QUIET) if [ ! -e $(SUPPORT_DIR) ] ; then mkdir -p $(SUPPORT_DIR) ; fi ;
	$(QUIET) if [ -d $(SUPPORT_DIR) ] && [ ! -e $(SUPPORT_DIR)/toolchain ] ; then mkdir -p $(SUPPORT_DIR)/toolchain ; fi ;
	@echo "    NPM-I handlebars"
	$(QUIET) cd $(SUPPORT_DIR)/toolchain ; $(TC_NPM_EXE) install handlebars ;
# endif
else
	@ if [ ! -e $(TC_HANDLEBARS_INT_EXE) ] ; then echo "    ERROR: Internal handlebars is necessary but not present, and FETCH_INTERNAL_TOOLS != 1." ; false ; fi ;
endif

$(SUPPORT_DIR):
# We need to stop using this. The time-stamp causes other things to rebuild.
	$(QUIET) mkdir -p $(SUPPORT_DIR) ;

$(V8_SRC_DIR):
ifeq ($(FETCH_INTERNAL_TOOLS),1)
	$(QUIET) if [ ! -e $(SUPPORT_DIR)/src ] ; then mkdir -p $(SUPPORT_DIR)/src ; fi ;
	@echo "    SVN-CO V8"
	$(QUIET) cd ../support/src && svn checkout http://v8.googlecode.com/svn/trunk/ v8 && cd v8 && make dependencies ;
else
	@ if [ ! -e $(V8_SRC_DIR) ] ; then echo "    ERROR: Internal V8 is necessary but not not present, and FETCH_INTERNAL_TOOLS != 1." ; false ; fi ;
endif

$(V8_DIR): $(V8_SRC_DIR)
	$(QUIET) if [ ! -e $(SUPPORT_DIR)/build ] ; then mkdir -p $(SUPPORT_DIR)/build ; fi ;
	$(QUIET) cd ../support && rm -rf build/v8 && cp -pRP src/v8 build/ ;

$(V8_LIB): $(V8_DIR)
	$(QUIET) cd $(V8_DIR) && make prefix=$(SUPPORT_DIR_ABS)/usr DESTDIR=/ native && find ./ -iname "*.o" | grep -v '\/preparser_lib\/' | xargs ar cqs libv8.a ;

$(NODE_SRC_DIR):
	$(QUIET) if [ ! -e $(SUPPORT_DIR)/src ] ; then mkdir -p $(SUPPORT_DIR)/src ; fi ;
ifeq ($(FETCH_INTERNAL_TOOLS),1)
	@echo "    WGET NODE"
	$(QUIET) cd ../support/src && wget http://nodejs.org/dist/v0.8.11/node-v0.8.11.tar.gz && tar -xzf node-v0.8.11.tar.gz && rm -rf node && mv node-v0.8.11 node && rm node-v0.8.11.tar.gz ;
else
	@ if [ ! -e $(NODE_SRC_DIR) ] ; then echo "    ERROR: Internal node.js is necessary but not not present, and FETCH_INTERNAL_TOOLS != 1." ; false ; fi ;
endif

$(NODE_DIR): $(NODE_SRC_DIR)
	$(QUIET) if [ ! -e $(SUPPORT_DIR)/build ] ; then mkdir -p $(SUPPORT_DIR)/build ; fi ;
	$(QUIET) cd ../support && rm -rf build/node && cp -pRP src/node build/ ;

$(TC_NPM_INT_EXE): $(TC_NODE_INT_EXE)

$(TC_NODE_INT_EXE): $(NODE_DIR)
# NOTE: MAKEFLAGS pass through to chained makes, so it is necessary to over-ride those flags that one wishes not to pass through. Scrubbing all flags and environment variables is not a solution since we want to pass through things like LD_LIBRARY_PATH and PATH and such.
	( unset prefix && unset PREFIX && unset DESTDIR && unset MAKEFLAGS && unset MFLAGS && cd $(NODE_DIR) && ./configure --prefix=$(SUPPORT_DIR_ABS)/usr && $(MAKE) prefix=$(SUPPORT_DIR_ABS)/usr DESTDIR=/ && $(MAKE) install prefix=$(SUPPORT_DIR_ABS)/usr DESTDIR=/ ; )

$(PROTOC_SRC_DIR):
	$(QUIET) if [ ! -e $(SUPPORT_DIR)/src ] ; then mkdir -p $(SUPPORT_DIR)/src ; fi ;
ifeq ($(FETCH_INTERNAL_TOOLS),1)
	@echo "    WGET PROTOC"
	$(QUIET) cd ../support/src && wget http://protobuf.googlecode.com/files/protobuf-2.4.1.tar.bz2 && tar -xjf protobuf-2.4.1.tar.bz2 && rm -rf protobuf && mv protobuf-2.4.1 protobuf && rm protobuf-2.4.1.tar.bz2 ;
else
	@ if [ ! -e $(PROTOC_SRC_DIR) ] ; then echo "    ERROR: Internal protoc is necessary but not not present, and FETCH_INTERNAL_TOOLS != 1." ; false ; fi ;
endif

$(PROTOC_DIR): $(PROTOC_SRC_DIR)
	$(QUIET) if [ ! -e $(SUPPORT_DIR)/build ] ; then mkdir -p $(SUPPORT_DIR)/build ; fi ;
	$(QUIET) cd ../support && rm -rf build/protobuf && cp -pRP src/protobuf build/ ;

$(TC_PROTOC_INT_EXE): $(PROTOC_DIR)
# NOTE: MAKEFLAGS pass through to chained makes, so it is necessary to over-ride those flags that one wishes not to pass through. Scrubbing all flags and environment variables is not a solution since we want to pass through things like LD_LIBRARY_PATH and PATH and such.
	cd $(PROTOC_DIR) && ./configure --prefix=$(SUPPORT_DIR_ABS)/usr && $(MAKE) PREFIX=$(SUPPORT_DIR_ABS)/usr prefix=$(SUPPORT_DIR_ABS)/usr DESTDIR=/ && $(MAKE) install PREFIX=$(SUPPORT_DIR_ABS)/usr prefix=$(SUPPORT_DIR_ABS)/usr DESTDIR=/ ;

$(GPERFTOOLS_SRC_DIR):
	$(QUIET) if [ ! -e $(SUPPORT_DIR)/src ] ; then mkdir -p $(SUPPORT_DIR)/src ; fi ;
ifeq ($(FETCH_INTERNAL_TOOLS),1)
	@echo "    WGET GPERFTOOLS"
	$(QUIET) cd ../support/src && wget http://gperftools.googlecode.com/files/gperftools-2.0.tar.gz && tar -xzf gperftools-2.0.tar.gz && rm -rf gperftools && mv gperftools-2.0 gperftools && rm gperftools-2.0.tar.gz ;
else
	@ if [ ! -e $(GPERFTOOLS_SRC_DIR) ] ; then echo "    ERROR: Internal gperftools is necessary but not not present, and FETCH_INTERNAL_TOOLS != 1." ; false ; fi ;
endif

$(GPERFTOOLS_DIR): $(GPERFTOOLS_SRC_DIR)
	$(QUIET) if [ ! -e $(SUPPORT_DIR)/build ] ; then mkdir -p $(SUPPORT_DIR)/build ; fi ;
	$(QUIET) cd ../support && rm -rf build/gperftools && cp -pRP src/gperftools build/ ;

$(LIBUNWIND_SRC_DIR):
	$(QUIET) if [ ! -e $(SUPPORT_DIR)/src ] ; then mkdir -p $(SUPPORT_DIR)/src ; fi ;
ifeq ($(FETCH_INTERNAL_TOOLS),1)
	@echo "    WGET LIBUNWIND"
	$(QUIET) cd ../support/src && wget http://download.savannah.gnu.org/releases/libunwind/libunwind-1.1.tar.gz && tar -xzf libunwind-1.1.tar.gz && rm -rf libunwind && mv libunwind-1.1 libunwind && rm libunwind-1.1.tar.gz ;
else
	@ if [ ! -e $(LIBUNWIND_SRC_DIR) ] ; then echo "    ERROR: Internal libunwind is necessary but not not present, and FETCH_INTERNAL_TOOLS != 1." ; false ; fi ;
endif

$(LIBUNWIND_DIR): $(LIBUNWIND_SRC_DIR)
	$(QUIET) if [ ! -e $(SUPPORT_DIR)/build ] ; then mkdir -p $(SUPPORT_DIR)/build ; fi ;
	$(QUIET) cd ../support && rm -rf build/libunwind && cp -pRP src/libunwind build/ ;

$(TCMALLOC_MINIMAL_INT_LIB): $(LIBUNWIND_DIR) $(GPERFTOOLS_DIR)
	$(QUIET) cd ../support/build && rm -f native_list.txt semistaged_list.txt staged_list.txt boost_list.txt post_boost_list.txt && touch native_list.txt semistaged_list.txt staged_list.txt boost_list.txt post_boost_list.txt && echo libunwind >> semistaged_list.txt && echo gperftools >> semistaged_list.txt && cp -pRP $(COLONIZE_SCRIPT_ABS) ./ && ( unset PREFIX && unset prefix && unset MAKEFLAGS && unset MFLAGS && unset DESTDIR && bash ./colonize.sh ; )

