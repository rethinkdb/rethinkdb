# Copyright 2010-2013 RethinkDB, all rights reserved.

##### Build parameters

# We assemble path directives.
LDPATHDS ?=
CXXPATHDS ?=
RT_CXX := $(CXX)
LDFLAGS ?= 
CXXFLAGS ?=
RT_LDFLAGS := $(LDFLAGS)
RT_CXXFLAGS := $(CXXFLAGS)

STATICFORCE := $(STATIC)

ifeq ($(OS),Linux)
  LDPTHREADFLAG := -pthread
else
  LDPTHREADFLAG :=
endif

ifeq ($(COMPILER),CLANG)

  ifeq ($(OS),Darwin)
    # TODO: ld: unknown option: --no-as-needed
    # RT_LDFLAGS += -Wl,--no-as-needed
  endif

  ifeq ($(STATICFORCE),1)
    # TODO(OSX)
    ifeq ($(OS),Linux)
      RT_LDFLAGS += -static
      STATIC_LIBGCC := 1
    endif
  endif

  RT_LDFLAGS += $(LDPATHDS) $(LDPTHREADFLAG) -lstdc++ -lm

else ifeq ($(COMPILER),INTEL)
  RT_LDFLAGS += -B/opt/intel/bin

  ifeq ($(STATICFORCE),1)
    # TODO(OSX)
    ifeq ($(OS),Linux)
      RT_LDFLAGS += -static
      STATIC_LIBGCC = 1
    endif
  endif

  RT_LDFLAGS += $(LDPATHDS) $(LDPTHREADFLAG) -lstdc++
else ifeq ($(COMPILER),GCC)

  ifeq ($(OS),Linux)
    RT_LDFLAGS += -Wl,--no-as-needed
  endif

  ifeq ($(STATICFORCE),1)
    # TODO(OSX)
    ifeq ($(OS),Linux)
      RT_LDFLAGS += -static
      STATIC_LIBGCC := 1
    endif
  endif

  RT_LDFLAGS += $(LDPATHDS) $(LDPTHREADFLAG)
endif

ifeq ($(OS),Linux)
  LIBRARY_PATHS += -lrt
endif

ifeq ($(STATICFORCE),1)
  # TODO(OSX)
  ifeq ($(OS),Linux)
    RT_LDFLAGS += -Wl,-Bdynamic
  endif
endif

ifeq ($(BUILD_PORTABLE),1)
  ifeq ($(OS),Linux)
    RT_LDFLAGS += -lgcc_s
    RT_LDFLAGS += -lgcc
  endif
endif

ifeq ($(BUILD_PORTABLE),1)
  LEGACY_PACKAGE := 1
else ifeq ($(LEGACY_LINUX),1)
  LEGACY_PACKAGE := 1
else
  LEGACY_PACKAGE := 0
endif

RT_LDFLAGS += $(foreach TLIB, $(LIB_SEARCH_PATHS), -L $(TLIB))

RT_CXXFLAGS?=
RT_CXXFLAGS += -I$(SOURCE_DIR)
RT_CXXFLAGS += -pthread
RT_CXXFLAGS += -DBOOST_NO_CXX11_RVALUE_REFERENCES
RT_CXXFLAGS += "-DPRODUCT_NAME=\"$(PRODUCT_NAME)\""
RT_CXXFLAGS += -DWEB_ASSETS_DIR_NAME='"$(WEB_ASSETS_DIR_NAME)"'
RT_CXXFLAGS += $(CXXPATHDS)
RT_CXXFLAGS += -Wall -Wextra

ifneq (1,$(ALLOW_WARNINGS))
  RT_CXXFLAGS += -Werror
endif

RT_CXXFLAGS += -Wnon-virtual-dtor -std=gnu++0x

ifeq ($(COMPILER), INTEL)
  RT_CXXFLAGS += -w1 -ftls-model=local-dynamic

else ifeq ($(COMPILER), CLANG)
  RT_CXXFLAGS += -Wformat=2 -Wswitch-enum -Wswitch-default # -Wno-unneeded-internal-declaration
  RT_CXXFLAGS += -Wused-but-marked-unused -Wunused-macros -Wundef -Wvla -Wshadow
  RT_CXXFLAGS += -Wconditional-uninitialized -Wmissing-noreturn

else ifeq ($(COMPILER), GCC)
  ifeq ($(LEGACY_GCC), 1)
    RT_CXXFLAGS += -Wformat=2 -Wswitch-enum -Wswitch-default
  else
    RT_CXXFLAGS += -Wformat=2 -Wswitch-enum -Wswitch-default -Wno-array-bounds
  endif
endif

ifeq ($(COVERAGE), 1)
  ifeq ($(COMPILER), GCC)
    RT_CXXFLAGS += --coverage
    RT_LDFLAGS += --coverage
  else
    $(error COVERAGE=1 not yet supported for $(COMPILER))
  endif
endif

ifeq ($(AGRESSIVE_BUF_UNLOADING),1)
  RT_CXXFLAGS += -DAGRESSIVE_BUF_UNLOADING=1
endif

RT_CXXFLAGS += -DWEBRESDIR='"$(web_res_dir)"'

# TODO: >() only works on bash >= 4
LD_OUTPUT_FILTER ?=
ifeq ($(COMPILER),INTEL)
  # TODO: get rid of the cause of this warning, not just the warning itself
  LD_OUTPUT_FILTER += 2> >(grep -v "warning: relocation refers to discarded section")
endif


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
  SYMBOLS := 1
  RT_CXXFLAGS += -O0
  ifeq ($(KEEP_INLINE),1)
    RT_CXXFLAGS+=-fkeep-inline-functions
  endif

else # ifeq ($(DEBUG),1)
  # use -fno-strict-aliasing to not break things
  # march=native used to break the serializer
  RT_CXXFLAGS += -O3 -DNDEBUG -fno-strict-aliasing # -march=native
  # TODO: remove this once memcached is added back in the release
  # (disables memcached from showing up in the admin CLI help or tab-completion)
  RT_CXXFLAGS += -DNO_MEMCACHE
  ifeq ($(NO_OMIT_FRAME_POINTER),1)
    RT_CXXFLAGS += -fno-omit-frame-pointer
  endif
endif # ifeq ($(DEBUG),1)

ifeq ($(DISABLE_BREAKPOINTS),1)
  RT_CXXFLAGS += -DDISABLE_BREAKPOINTS
endif

ifeq (${STATIC_LIBGCC},1)
  RT_LDFLAGS += -static-libgcc -static-libstdc++
endif

ifeq ($(OPROFILE),1)
  SYMBOLS=1
endif

ifeq ($(SYMBOLS),1)
  # -rdynamic is necessary so that backtrace_symbols() works properly
  ifneq ($(OS),Darwin)
    RT_LDFLAGS += -rdynamic
  endif
  RT_CXXFLAGS += -g
endif  # ($(SYMBOLS),1)

ifeq ($(SEMANTIC_SERIALIZER_CHECK),1)
  RT_CXXFLAGS += -DSEMANTIC_SERIALIZER_CHECK
endif

ifeq ($(MOCK_CACHE_CHECK),1)
  RT_CXXFLAGS += -DMOCK_CACHE_CHECK
endif

ifeq ($(BTREE_DEBUG),1)
  RT_CXXFLAGS += -DBTREE_DEBUG
endif

ifeq ($(JSON_SHORTCUTS),1)
  RT_CXXFLAGS += -DJSON_SHORTCUTS
endif

ifeq ($(MALLOC_PROF),1)
  RT_CXXFLAGS += -DMALLOC_PROF
endif

ifeq ($(SERIALIZER_DEBUG),1)
  RT_CXXFLAGS += -DSERIALIZER_MARKERS
endif

ifeq ($(MEMCACHED_STRICT),1)
  RT_CXXFLAGS += -DMEMCACHED_STRICT
endif

ifeq ($(LEGACY_LINUX),1)
  RT_CXXFLAGS += -DLEGACY_LINUX -DNO_EPOLL -Wno-format
endif

ifeq ($(LEGACY_GCC),1)
  RT_CXXFLAGS += -Wno-switch-default -Wno-switch-enum
endif

ifeq ($(NO_EVENTFD),1)
  RT_CXXFLAGS += -DNO_EVENTFD
endif

ifeq ($(NO_EPOLL),1)
  RT_CXXFLAGS += -DNO_EPOLL
endif

ifeq ($(MCHECK_PEDANTIC),1)
  RT_CXXFLAGS += -DMCHECK_PEDANTIC
  MCHECK := 1
endif

ifeq ($(MCHECK),1)
  ifneq (1,$(NO_TCMALLOC))
    $(error cannot build with MCHECK=1 when NO_TCMALLOC=0)
  endif
  RT_CXXFLAGS += -DMCHECK
  RT_LDFLAGS += -lmcheck
endif

ifeq ($(VALGRIND),1)
  ifneq (1,$(NO_TCMALLOC))
    $(error cannot build with VALGRIND=1 when NO_TCMALLOC=0)
  endif
  RT_CXXFLAGS += -DVALGRIND
endif

ifeq ($(AIOSUPPORT),1)
  RT_CXXFLAGS += -DAIOSUPPORT
  RT_LDFLAGS += -laio
endif

ifeq ($(LEGACY_PROC_STAT),1)
  RT_CXXFLAGS += -DLEGACY_PROC_STAT
endif

RT_CXXFLAGS += -I$(PROTO_DIR)

UNIT_STATIC_LIBRARY_PATH := $(EXTERNAL_DIR)/gtest-1.6.0/make/gtest.a
UNIT_TEST_INCLUDE_FLAG := -I$(EXTERNAL_DIR)/gtest-1.6.0/include

RT_CXXFLAGS += -DMIGRATION_SCRIPT_LOCATION=\"$(scripts_dir)/rdb_migrate\"

#### Finding what to build

SOURCES := $(shell find $(SOURCE_DIR) -name '*.cc')

SERVER_EXEC_SOURCES := $(filter-out $(SOURCE_DIR)/unittest/%,$(SOURCES))

PROTO_SOURCES := $(shell find $(SOURCE_DIR) -name '*.proto')
PROTO_HEADERS := $(patsubst $(SOURCE_DIR)/%.proto,$(PROTO_DIR)/%.pb.h,$(PROTO_SOURCES))
PROTO_CODE := $(patsubst $(SOURCE_DIR)/%.proto,$(PROTO_DIR)/%.pb.cc,$(PROTO_SOURCES))
PROTO_NAMES += $(patsubst $(PROTO_DIR)/%.cc,%,$(PROTO_CODE))
PROTO_OBJS := $(patsubst %,$(OBJ_DIR)/%.o,$(PROTO_NAMES))
PROTOCFLAGS_CXX := --proto_path=$(SOURCE_DIR)

NAMES := $(patsubst $(SOURCE_DIR)/%.cc,%,$(SOURCES))
DEPS := $(patsubst %,$(DEP_DIR)/%.d,$(NAMES))
OBJS := $(PROTO_OBJS) $(patsubst %,$(OBJ_DIR)/%.o,$(NAMES))

SERVER_EXEC_OBJS := $(PROTO_OBJS) $(patsubst $(SOURCE_DIR)/%.cc,$(OBJ_DIR)/%.o,$(SERVER_EXEC_SOURCES))

SERVER_NOMAIN_OBJS := $(PROTO_OBJS) $(patsubst $(SOURCE_DIR)/%.cc,$(OBJ_DIR)/%.o,$(filter-out %/main.cc,$(SOURCES)))

SERVER_UNIT_TEST_OBJS := $(SERVER_NOMAIN_OBJS) $(OBJ_DIR)/unittest/main.o

##### Version number handling

RT_CXXFLAGS += -DRETHINKDB_VERSION=\"$(RETHINKDB_VERSION)\"
RT_CXXFLAGS += -DRETHINKDB_CODE_VERSION=\"$(RETHINKDB_CODE_VERSION)\"

##### Build targets

ALL += $(SOURCE_DIR)
.PHONY: $(SOURCE_DIR)/all
$(SOURCE_DIR)/all: $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(BUILD_DIR)/$(GDB_FUNCTIONS_NAME) | $(BUILD_DIR)/.

ifeq ($(UNIT_TESTS),1)
  $(SOURCE_DIR)/all: $(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME)
endif

$(UNIT_STATIC_LIBRARY_PATH):
	$P MAKE $@
	$(EXTERN_MAKE) -C $(EXTERNAL_DIR)/gtest-1.6.0/make gtest.a

.PHONY: unit
unit: $(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME)
	$P RUN $(SERVER_UNIT_TEST_NAME)
	$(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME) --gtest_filter=$(UNIT_TEST_FILTER)

.SECONDARY: $(PROTO_HEADERS) $(PROTO_CODE)
$(PROTO_HEADERS) $(PROTO_CODE): $(PROTO_DIR)/.protocppgen
$(PROTO_DIR)/.protocppgen: $(PROTO_SOURCES) | $(PROTOC_DEP) $(PROTO_DIR)/.
	$P PROTOC[CPP] $^
	$(PROTOC_RUN) $(PROTOCFLAGS_CXX) --cpp_out $(PROTO_DIR) $^
	touch $@

rpc/semilattice/joins/macros.hpp: $(TOP)/scripts/generate_join_macros.py
rpc/serialize_macros.hpp: $(TOP)/scripts/generate_serialize_macros.py
rpc/mailbox/typed.hpp: $(TOP)/scripts/generate_rpc_templates.py
rpc/semilattice/joins/macros.hpp rpc/serialize_macros.hpp rpc/mailbox/typed.hpp:
	$P GEN $@
	$< > $@

.PHONY: rethinkdb
rethinkdb: $(BUILD_DIR)/$(SERVER_EXEC_NAME)

$(BUILD_DIR)/$(SERVER_EXEC_NAME): $(SERVER_EXEC_OBJS) | $(BUILD_DIR)/. $(TCMALLOC_DEP)
	$P LD $@
	$(RT_CXX) $(RT_LDFLAGS) $(SERVER_EXEC_OBJS) $(LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(LD_OUTPUT_FILTER)
ifeq ($(NO_TCMALLOC),0)
# TODO: c++filt is not available everywhere
	@objdump -T $(BUILD_DIR)/$(SERVER_EXEC_NAME) | c++filt | grep -q 'tcmalloc::\|google_malloc' || \
		(echo "    Failed to link in TCMalloc. You may have to run ./configure with the --without-tcmalloc flag." && \
		false)
endif

# The unittests use gtest, which uses macros that expand into switch statements which don't contain
# default cases. So we have to remove the -Wswitch-default argument for them.
$(OBJ_DIR)/unittest/%.o: RT_CXXFLAGS := $(filter-out -Wswitch-default,$(RT_CXXFLAGS)) $(UNIT_TEST_INCLUDE_FLAG)

$(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME): $(SERVER_UNIT_TEST_OBJS) $(UNIT_STATIC_LIBRARY_PATH) | $(BUILD_DIR)/. $(TCMALLOC_DEP)
	$P LD $@
	$(RT_CXX) $(RT_LDFLAGS) $(SERVER_UNIT_TEST_OBJS) $(UNIT_STATIC_LIBRARY_PATH) $(LIBRARY_PATHS) -o $@ $(LD_OUTPUT_FILTER)

$(BUILD_DIR)/$(GDB_FUNCTIONS_NAME):
	$P CP $@
	cp $(SCRIPTS_DIR)/$(GDB_FUNCTIONS_NAME) $@

depclean:
	$P RM "$(BUILD_ROOT_DIR)/*.d"
	if test -d $(BUILD_ROOT_DIR); then find $(BUILD_ROOT_DIR) -name '*.d' -exec rm {} \; ; fi

.PHONY: $(TOP)/src/clean
$(TOP)/src/clean:
	$P RM $(BUILD_DIR)
	rm -rf $(BUILD_DIR)

$(OBJ_DIR)/%.pb.o: $(PROTO_DIR)/%.pb.cc $(MAKEFILE_DEPENDENCY) $(PROTO_HEADERS)
	mkdir -p $(dir $@)
	$P CC $< -o $@
	$(RT_CXX) $(RT_CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cc $(MAKEFILE_DEPENDENCY) $(V8_DEP) | $(PROTO_OBJS)
	mkdir -p $(dir $(DEP_DIR)/$*)
	$(RT_CXX) $(RT_CXXFLAGS) -MM -MP -MQ $@ -MQ $(DEP_DIR)/$*.d $< > $(DEP_DIR)/$*.d
	mkdir -p $(dir $@)
	$P CC $< -o $@
	$(RT_CXX) $(RT_CXXFLAGS) -c -o $@ $<

-include $(DEPS)