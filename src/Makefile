space=$(eval) $(eval)
comma=,


##### Pretty-printing

ANSI_BOLD_ON:=[1m
ANSI_BOLD_OFF:=[0m
ANSI_UL_ON:=[4m
ANSI_UL_OFF:=[0m

##### Configuring stuff

IGNORE_MAKEFILE_CHANGES?=0

ifeq ($(IGNORE_MAKEFILE_CHANGES),1)
MAKEFILE_DEPENDENCY:=
else
MAKEFILE_DEPENDENCY:=Makefile
endif

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

TRIAL_PACKAGE_NAME:=$(VANILLA_PACKAGE_NAME)-trial
PACKAGE_NAME:=$(VANILLA_PACKAGE_NAME)
SERVER_UNIT_TEST_NAME:=$(SERVER_EXEC_NAME)-unittest

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

JSON_SHORTCUTS?=0
DEBUG?=1
VALGRIND?=0
MCHECK?=0
NO_TCMALLOC?=0
SEMANTIC_SERIALIZER_CHECK?=0
MOCK_CACHE_CHECK?=0
VERBOSE?=0
# UNIT_TESTS?=$(DEBUG)
UNIT_TESTS?=0
AIOSUPPORT?=0
BUILD_DRIVERS?=1

# Set TRIAL to 1 to build a 21-day trial version. If you want a different number of days, set TIMEBOMB to the number of days instead.
TRIAL?=0
TIMEBOMB?=0

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

PACKAGES_DIR:=$(BUILD_ROOT_DIR)/packages
RPM_PACKAGE_DIR:=$(PACKAGES_DIR)/rpm
DEB_PACKAGE_DIR:=$(PACKAGES_DIR)/deb


ifeq ($(COMPILER),CLANG)
CXX:=clang
else
ifeq ($(COMPILER),INTEL)
CXX:=icc
endif
endif

ifeq ($(COMPILER),CLANG)
LDFLAGS:=-Wl,--no-as-needed -pthread -lrt -lstdc++ -lm -lv8 -lcrypto
else
ifeq ($(COMPILER),INTEL)
LDFLAGS:= -B/opt/intel/bin -pthread -lrt -lstdc++ -lv8 -lcrypto
else
# GCC
LDFLAGS:=-Wl,--no-as-needed -pthread -lrt -lv8 -lcrypto
endif
endif

ifeq ($(AIOSUPPORT),1)
CXXFLAGS+=-DAIOSUPPORT
LDFLAGS+= -laio
endif

STATIC_LIBRARIES:=boost_serialization protobuf boost_program_options

CXXFLAGS?=

CXXFLAGS+=-I$(SOURCE_DIR) -pthread
CXXFLAGS+="-DPRODUCT_NAME=\"$(PRODUCT_NAME)\""

ifeq ($(SOLO),1)
CXXFLAGS+=-DRETHINKDB_SOLO_PRODUCT
else
CXXFLAGS+=-DRETHINKDB_CACHE_PRODUCT
endif

CXXFLAGS+=-Wall -Wextra -Werror -Wnon-virtual-dtor

ifeq ($(COMPILER), INTEL)
CXXFLAGS+=-std=gnu++98 -w1 -ftls-model=local-dynamic
else
ifeq ($(COMPILER), CLANG)
CXXFLAGS+=-Wformat=2 -Wswitch-enum -Wswitch-default -Wno-unneeded-internal-declaration -Wused-but-marked-unused -Wunused-macros -Wundef -Wvla -Wshadow -Wconditional-uninitialized -Wmissing-noreturn
else
ifeq ($(LEGACY_GCC), 1)
CXXFLAGS+=-Wformat=2 -Wswitch-enum -Wswitch-default
else
CXXFLAGS+=-Wformat=2 -Wswitch-enum -Wswitch-default -Wno-array-bounds
endif
endif
endif

ifeq ($(AGRESSIVE_BUF_UNLOADING),1)
CXXFLAGS+=-DAGRESSIVE_BUF_UNLOADING=1
endif

ifeq ($(COMPILER),INTEL)
IGNORE_SOME_ICC_LD_WARNINGS?=1
else
IGNORE_SOME_ICC_LD_WARNINGS?=0
endif

STRIP_ON_INSTALL?=1

NO_OMIT_FRAME_POINTER?=0

# Configure debug vs. release
ifeq ($(DEBUG),1)
SYMBOLS:=1
CXXFLAGS+=-O0

ifeq ($(KEEP_INLINE),1)
CXXFLAGS+=-fkeep-inline-functions
endif

else
# use -fno-strict-aliasing to not break things
# march=native used to break the serializer
CXXFLAGS+=-O3 -DNDEBUG -fno-strict-aliasing # -match=native
# TODO: remove this once memcached is added back in the release (disables memcached from showing up in the admin CLI help or tab-completion)
CXXFLAGS+=-DNO_MEMCACHE
ifeq ($(NO_OMIT_FRAME_POINTER),1)
CXXFLAGS+=-fno-omit-frame-pointer
else
CXXFLAGS+=-fomit-frame-pointer
endif
endif

ifeq ($(DISABLE_BREAKPOINTS),1)
CXXFLAGS+=-DDISABLE_BREAKPOINTS
endif

ifneq ($(TRIAL),0)
ifeq ($(TIMEBOMB),0)
TIMEBOMB:=21
endif
endif

ifneq ($(TIMEBOMB),0)
TRIAL:=1
CXXFLAGS+=-DTIMEBOMB_DAYS=$(TIMEBOMB)
endif

ifneq ($(TRIAL),0)
BUILD_DIR:=$(BUILD_DIR)-trial
PACKAGE_NAME:=$(TRIAL_PACKAGE_NAME)
SERVER_EXEC_NAME:=$(SERVER_EXEC_NAME)-trial
endif

ifeq (${STATIC_LIBGCC},1)
LDFLAGS+=-static-libgcc -static-libstdc++
endif

ifeq ($(OPROFILE),1)
SYMBOLS=1
endif

ifeq ($(CALLGRIND),1)
SYMBOLS=1
endif

ifeq ($(SYMBOLS),1)
# -rdynamic is necessary so that backtrace_symbols() works properly
LDFLAGS+=-rdynamic
CXXFLAGS+=-g
endif

ifeq ($(SEMANTIC_SERIALIZER_CHECK),1)
CXXFLAGS+=-DSEMANTIC_SERIALIZER_CHECK
BUILD_DIR:=$(BUILD_DIR)-scs
endif

ifeq ($(MOCK_CACHE_CHECK),1)
CXXFLAGS+=-DMOCK_CACHE_CHECK
BUILD_DIR:=$(BUILD_DIR)-mockcache
endif

ifeq ($(BTREE_DEBUG),1)
CXXFLAGS+=-DBTREE_DEBUG
endif

ifeq ($(JSON_SHORTCUTS),1)
CXXFLAGS+=-DJSON_SHORTCUTS
endif

ifeq ($(MALLOC_PROF),1)
CXXFLAGS+=-DMALLOC_PROF
endif

ifeq ($(SERIALIZER_DEBUG),1)
CXXFLAGS:=$(CXXFLAGS) -DSERIALIZER_MARKERS
endif

ifneq ($(MEMCACHED_STRICT), 0)
CXXFLAGS+=-DMEMCACHED_STRICT
endif

ifeq ($(LEGACY_LINUX),1)
#CXX=g++44
CXXFLAGS+=-DLEGACY_LINUX -DNO_EPOLL -Wno-format
BUILD_DIR:=$(BUILD_DIR)-legacy
endif

ifeq ($(LEGACY_GCC),1)
CXXFLAGS+=-Wno-switch-default -Wno-switch-enum
BUILD_DIR:=$(BUILD_DIR)-legacy-gcc
endif

ifeq ($(NO_EVENTFD),1)
CXXFLAGS+=-DNO_EVENTFD
BUILD_DIR:=$(BUILD_DIR)-no_eventfd
endif

ifeq ($(NO_EPOLL),1)
CXXFLAGS+=-DNO_EPOLL
BUILD_DIR:=$(BUILD_DIR)-noepoll
endif

ifeq ($(MCHECK_PEDANTIC),1)
CXXFLAGS+=-DMCHECK_PEDANTIC
MCHECK:=1
endif

ifeq ($(MCHECK),1)
NO_TCMALLOC:=1
CXXFLAGS+=-DMCHECK
LDFLAGS+=-lmcheck
endif

ifeq ($(VALGRIND),1)
CXXFLAGS+=-DVALGRIND
BUILD_DIR:=$(BUILD_DIR)-valgrind
NO_TCMALLOC:=1
endif

ifeq ($(LEGACY_PROC_STAT),1)
CXXFLAGS+=-DLEGACY_PROC_STAT
BUILD_DIR:=$(BUILD_DIR)-legacy-proc-stat
endif

PROTO_DIR:=$(BUILD_DIR)/proto
CXXFLAGS += -I$(PROTO_DIR)

ifeq ($(NO_TCMALLOC),0)
STATIC_LIBRARIES+=tcmalloc_minimal
endif

# look for the static library in the same directory as the .so file
STATIC_LIBRARY_PATHS:=$(foreach lib,$(STATIC_LIBRARIES),$(shell /sbin/ldconfig -p | awk '/lib$(lib).so / { gsub("\\.so$$", ".a", $$NF); print $$NF; exit 0; }'))

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

# googletest flags
ifeq ($(UNIT_TESTS),1)
LDFLAGS+=-L/usr/local/lib -lgtest
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
#This atrociousness comes from the fact... fuck it blah blah blah coffee script.
#Basically coffee script can't handle dependencies.
COFFEE_SOURCES:=$(patsubst %, $(WEB_SOURCE_DIR)/static/coffee/%,\
			util.coffee \
			loading.coffee \
			body.coffee \
			ui_components/modals.coffee ui_components/list.coffee \
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
IMAGES_EXTERNAL_DIR:=$(WEB_SOURCE_DIR)/static/images
FAVICON:=$(WEB_SOURCE_DIR)/favicon.ico

#### Version number handling
RETHINKDB_VERSION:=$(shell ../scripts/gen-version.sh)
RETHINKDB_SHORT_VERSION:=$(shell ../scripts/gen-version.sh -s)
PACKAGING_ALTERNATIVES_PRIORITY:=$(shell expr $$(../scripts/gen-version.sh -r) / 100)

# ifneq ($(TIMEBOMB),0)
# RETHINKDB_VERSION:=$(RETHINKDB_VERSION)-trial
# RETHINKDB_SHORT_VERSION:=$(RETHINKDB_SHORT_VERSION)-trial
# endif
RETHINKDB_PACKAGING_VERSION?=$(RETHINKDB_VERSION)
CXXFLAGS+=-DRETHINKDB_VERSION=\"$(RETHINKDB_VERSION)\"

SERVER_EXEC_NAME_VERSIONED:=$(SERVER_EXEC_NAME)-$(RETHINKDB_SHORT_VERSION)

##### Build targets
.PHONY: all build-deb build-rpm callgrind callgrind clean cscope deb deb depclean drivers etags gdb install install-binaries install-deb install-docs install-manpages install-rpm install-tools prepare_deb_package_dirs prepare_rpm_package_dirs regdb rerun revalgrind rpm rpm rpm-suse10 run sembuild showdefines style tags unit valgrind web-assets coffeelint

# High level build targets

all: $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(BUILD_DIR)/$(START_DB_NAME) $(BUILD_DIR)/$(GDB_FUNCTIONS_NAME) web-assets drivers
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
DEB_PACKAGE_REVISION?=1
RETHINKDB_VERSION_DEB=$(RETHINKDB_PACKAGING_VERSION)-$(DEB_PACKAGE_REVISION)

VERSIONED_PACKAGE_NAME:=$(PACKAGE_NAME)-$(RETHINKDB_SHORT_VERSION)
VERSIONED_TRIAL_PACKAGE_NAME:=$(TRIAL_PACKAGE_NAME)-$(RETHINKDB_SHORT_VERSION)

VERSIONED_PRODUCT_SHARE_DIR:=/usr/share/$(VERSIONED_PACKAGE_NAME)

prefix?=/usr
bin_dir:=$(prefix)/bin
doc_dir:=$(prefix)/share/doc/$(VERSIONED_PACKAGE_NAME)
man_dir:=$(prefix)/share/man
man1_dir:=$(man_dir)/man1
share_dir:=$(prefix)/share/$(VERSIONED_PACKAGE_NAME)
bash_completion_dir:=/etc/bash_completion.d
internal_bash_completion_dir:=$(share_dir)$(bash_completion_dir)
scripts_dir:=$(share_dir)/scripts

ASSET_SCRIPTS:=$(ASSETS_DIR)/scripts/rdb_migrate

CXXFLAGS+=-DMIGRATION_SCRIPT_LOCATION=\"$(scripts_dir)/rdb_migrate\"

FULL_SERVER_EXEC_NAME=$(bin_dir)/$(SERVER_EXEC_NAME)
FULL_SERVER_EXEC_NAME_VERSIONED=$(bin_dir)/$(SERVER_EXEC_NAME_VERSIONED)

prepare_deb_package_dirs:
	$(QUIET) mkdir -p $(DEB_PACKAGE_DIR)
	$(QUIET) mkdir -p $(DEB_CONTROL_ROOT)

prepare_rpm_package_dirs:
	$(QUIET) mkdir -p $(RPM_PACKAGE_DIR)
	$(QUIET) for d in BUILD RPMS/x86_64 SOURCES SPECS SRPMS; do mkdir -p $(RPM_PACKAGE_DIR)/$$d; done

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

install-docs:
	$(QUIET) install -m755 -d $(DESTDIR)$(doc_dir)
	$(QUIET) install -m644 -T $(ASSETS_DIR)/docs/LICENSE $(DESTDIR)$(doc_dir)/copyright

install: install-binaries install-manpages install-docs install-tools
install-rpm: install
install-deb: install
	$(QUIET) install -m755 -d $(DESTDIR)$(doc_dir)
	$(QUIET) sed	-e 's/PACKAGING_VERSION/$(RETHINKDB_VERSION_DEB)/' $(ASSETS_DIR)/docs/changelog.Debian | \
		gzip -c9 | \
		install -m644 -T /dev/stdin $(DESTDIR)$(doc_dir)/changelog.Debian.gz


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
	$(QUIET) find $(DEB_PACKAGE_DIR) -type f -printf "/%P\n" | (grep '^/etc/' | true) > $(DEB_CONTROL_ROOT)/conffiles

# Copy {pre,post}{inst,rm} scripts
	$(QUIET) for script in preinst postinst prerm postrm; do		                  		\
			m4 																																						\
					-D "BIN_DIR=$(bin_dir)" 																									\
					-D "MAN1_DIR=$(man1_dir)" 																								\
					-D "BASH_COMPLETION_DIR=$(bash_completion_dir)" 													\
					-D "INTERNAL_BASH_COMPLETION_DIR=$(internal_bash_completion_dir)" 				\
					-D "SERVER_EXEC_NAME=$(SERVER_EXEC_NAME)" 																\
					-D "SERVER_EXEC_NAME_VERSIONED=$(SERVER_EXEC_NAME_VERSIONED)" 						\
					-D "PRIORITY=$(PACKAGING_ALTERNATIVES_PRIORITY)" 													\
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
				-D "LEGACY_LINUX=$(LEGACY_LINUX)"												\
				-D "TRIAL=$(TRIAL)"																			\
				-D "DISK_SIZE=$${disk_size}"														\
			../packaging/$(DEBIAN_PKG_DIR)/control >$(DEB_CONTROL_ROOT)/control

# Finally, produce a debian package
	$(QUIET) fakeroot dpkg-deb -b $(DEB_PACKAGE_DIR) $(PACKAGES_DIR)

deb: all prepare_deb_package_dirs
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=0 RETHINKDB_PACKAGING_VERSION=$(RETHINKDB_VERSION)-unstripped build-deb
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=1 RETHINKDB_PACKAGING_VERSION=$(RETHINKDB_VERSION) build-deb

# Print a nice message with the location of the created package
	@echo
	@echo "$(ANSI_BOLD_ON)Your DEB packages are here (with lintian output following each package):$(ANSI_BOLD_OFF)"
	$(QUIET) for f in $(PACKAGES_DIR)/*.deb; do \
			deb_name=`readlink -f $(PACKAGES_DIR)`/$$(basename $$f); \
			echo "  $(ANSI_UL_ON)$${deb_name}$(ANSI_UL_OFF)"; \
			lintian --color auto --suppress-tags "no-copyright-file,$(subst $(space),$(comma),$(SUPPRESSED_LINTIAN_TAGS))" $${deb_name} || true; \
		done
	$(QUIET) echo

build-rpm: all prepare_rpm_package_dirs
	@echo
	@echo "$(ANSI_BOLD_ON)Building RPM package for RethinkDB version $(RETHINKDB_PACKAGING_VERSION)$(ANSI_BOLD_OFF)"
	@echo

# Replace version/topdir fields in the spec file
	$(QUIET) m4																															\
				-D "SOLO=$(SOLO)"																									\
				-D "RPM_PACKAGE_DIR=`readlink -f $(RPM_PACKAGE_DIR)`"							\
				-D "SERVER_EXEC_NAME=$(SERVER_EXEC_NAME)"													\
				-D "SERVER_EXEC_NAME_VERSIONED=$(SERVER_EXEC_NAME_VERSIONED)"			\
				-D "PACKAGE_NAME=$(PACKAGE_NAME)"																	\
				-D "VERSIONED_PACKAGE_NAME=$(VERSIONED_PACKAGE_NAME)"							\
				-D "VANILLA_PACKAGE_NAME=$(VANILLA_PACKAGE_NAME)"									\
				-D "TRIAL_PACKAGE_NAME=$(TRIAL_PACKAGE_NAME)"											\
				-D "VERSIONED_TRIAL_PACKAGE_NAME=$(VERSIONED_TRIAL_PACKAGE_NAME)"	\
				-D "PACKAGE_VERSION=$(RETHINKDB_VERSION_DEB)"											\
				-D "LEGACY_LINUX=$(LEGACY_LINUX)"																	\
				-D "PACKAGE_FOR_SUSE_10=$(PACKAGE_FOR_SUSE_10)"								    \
				-D "TRIAL=$(TRIAL)"																						    \
				-D "BIN_DIR=$(bin_dir)"																						\
				-D "DOC_DIR=$(doc_dir)"																						\
				-D "MAN1_DIR=$(man1_dir)"																					\
				-D "BASH_COMPLETION_DIR=$(bash_completion_dir)" 									\
				-D "INTERNAL_BASH_COMPLETION_DIR=$(internal_bash_completion_dir)"	\
				-D "SCRIPTS_DIR=$(scripts_dir)"																		\
				-D "PRIORITY=$(PACKAGING_ALTERNATIVES_PRIORITY)" 									\
			$(RPM_SPEC_INPUT) >$(RPM_SPEC_FILE)

# Copy files to their correct places
	$(QUIET) $(MAKE) DESTDIR=$(RPM_BUILD_ROOT) install-rpm

# Produce RPM package
# FIXME: write rpmbuild output out to stderr if there's an error
	$(QUIET) rpmbuild -bb --target=x86_64 --buildroot `readlink -f $(RPM_BUILD_ROOT)` $(RPM_SPEC_FILE) > $(RPM_PACKAGE_DIR)/rpmbuild.stdout

# RPM for redhat and centos
rpm: all prepare_rpm_package_dirs
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=0 RETHINKDB_VERSION=$(RETHINKDB_VERSION)-unstripped build-rpm
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=1 RETHINKDB_VERSION=$(RETHINKDB_VERSION) build-rpm

# Print a nice message with the location of the created package
	@echo
	@echo "$(ANSI_BOLD_ON)Your RPM packages are here:$(ANSI_BOLD_OFF)"
	$(QUIET) for f in $(RPM_PACKAGE_DIR)/RPMS/x86_64/*.rpm; do	                \
			rpm_name=`readlink -f $(PACKAGES_DIR)`/$$(basename $$f);	\
			mv $$f $$rpm_name;						\
			echo "  $(ANSI_UL_ON)$${rpm_name}$(ANSI_UL_OFF)";		\
		done
	$(QUIET) echo

else
# Ubuntu/Debian
deb: clean
	$(QUIET) $(MAKE) PACKAGING=1 DEBUG=0 SYMBOLS=1 deb

# RPM for CentOS/RedHat
rpm: clean
	$(QUIET) $(MAKE) PACKAGING=1 DEBUG=0 SYMBOLS=1 rpm

# RPM for Suse 10
rpm-suse10:
	$(QUIET) $(MAKE) PACKAGE_FOR_SUSE_10=1 rpm
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
	$(QUIET) cd ../drivers ; $(MAKE) $(MFLAGS) ;
else
	$(QUIET) cd ../drivers/javascript ; $(MAKE) $(MFLAGS) ;
endif #BUILD_DRIVERS


$(WEB_ASSETS_BUILD_DIR)/js/rethinkdb.js: drivers
	$(QUIET) cp -pRP ../drivers/javascript/rethinkdb.js $(WEB_ASSETS_BUILD_DIR)/js/rethinkdb.js ;

rpc/semilattice/joins/macros.hpp: ../scripts/generate_join_macros.py
rpc/serialize_macros.hpp: ../scripts/generate_serialize_macros.py
rpc/mailbox/typed.hpp: ../scripts/generate_rpc_templates.py

rpc/semilattice/joins/macros.hpp rpc/serialize_macros.hpp rpc/mailbox/typed.hpp:
	$< > $@

web-assets: $(WEB_ASSETS_BUILD_DIR)/cluster-min.js $(WEB_ASSETS_BUILD_DIR)/cluster.css $(WEB_ASSETS_BUILD_DIR)/index.html $(WEB_ASSETS_BUILD_DIR)/js $(WEB_ASSETS_BUILD_DIR)/images $(WEB_ASSETS_BUILD_DIR)/favicon.ico $(WEB_ASSETS_BUILD_DIR)/js/rethinkdb.js

$(WEB_ASSETS_OBJ_DIR)/cluster-min.concat.coffee: $(COFFEE_SOURCES)
	$(QUIET) mkdir -p $(WEB_ASSETS_OBJ_DIR)
ifeq ($(VERBOSE),0)
	@echo "    CONCAT $@"
endif
	$(QUIET) cat $+ > $@

$(WEB_ASSETS_BUILD_DIR)/cluster-min.js: $(WEB_ASSETS_OBJ_DIR)/cluster-min.concat.coffee
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    COFFEE $@"
endif
	$(QUIET) coffee -bp --stdio < $(WEB_ASSETS_OBJ_DIR)/cluster-min.concat.coffee > $@

$(WEB_ASSETS_BUILD_DIR)/cluster.css: $(LESS_MAIN)
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    LESSC $@"
endif
	$(QUIET) lessc $(LESS_MAIN) > $@

$(WEB_ASSETS_BUILD_DIR)/index.html: $(CLUSTER_HTML) $(HANDLEBARS_SOURCES)
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    STRIP GEN $@"
	@echo "    INJECT HANDLEBARS $@"
endif
	$(QUIET) sed 's:="gen/:=":g' $(CLUSTER_HTML) | python -c 'import sys; sys.stdout.write(sys.stdin.read().replace("<!--{{INJECT_HANDLEBARS_TEMPLATES}}-->", reduce(lambda acc, fname: acc + open(fname).read(), sys.argv[1:], "")))' $(HANDLEBARS_SOURCES) > $@

$(WEB_ASSETS_BUILD_DIR)/js:
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    CP $(JS_EXTERNAL_DIR) -> $(WEB_ASSETS_BUILD_DIR)"
endif
	$(QUIET) cp -rP $(JS_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)

$(WEB_ASSETS_BUILD_DIR)/images:
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    CP $(IMAGES_EXTERNAL_DIR) -> $(WEB_ASSETS_BUILD_DIR)"
endif
	$(QUIET) cp -rP $(IMAGES_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)

$(WEB_ASSETS_BUILD_DIR)/favicon.ico: $(FAVICON)
	$(QUIET) mkdir -p $(WEB_ASSETS_BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    CP $(FAVICON) -> $(WEB_ASSETS_BUILD_DIR)"
endif
	$(QUIET) cp -P $(FAVICON) $(WEB_ASSETS_BUILD_DIR)

# Special recipe for the proto buffer generated code
# Need to use a dummy file to avoid calling protoc once for each input file.
$(PROTO_DIR)/dummy: $(PROTO_SOURCES)
	$(QUIET) mkdir -p $(PROTO_DIR)
ifeq ($(VERBOSE),0)
	@echo "    PROTOC[CPP] $^"
endif
	$(QUIET) protoc $(PROTOCFLAGS) --cpp_out $(PROTO_DIR) $^
	$(QUIET) touch $@

$(PROTO_HEADERS) $(PROTO_CODE): $(PROTO_DIR)/dummy

$(BUILD_DIR)/$(SERVER_EXEC_NAME): $(OBJS) $(BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    LD $@"
endif
ifeq ($(IGNORE_SOME_ICC_LD_WARNINGS),0)
	$(QUIET) $(CXX) $(LDFLAGS) $(SERVER_EXEC_OBJS) $(STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_EXEC_NAME)
else
# The following line runs CXX in linking mode (supplying only the object files and libraries) and then
# pass the output through a filter which filters out bogus (?) icc-related problems.
# FIXME: figure out why we get these problems
	$(QUIET) ($(CXX) $(LDFLAGS) $(SERVER_EXEC_OBJS) $(STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_EXEC_NAME) 2>&1 >&3 | (grep -v "warning: relocation refers to discarded section" || true) >&2) 3>&1
endif
ifeq ($(NO_TCMALLOC),0)
	@objdump -T $(BUILD_DIR)/$(SERVER_EXEC_NAME) | c++filt | grep -q 'tcmalloc::\|google_malloc' || \
		(echo "    Failed to link in TCMalloc. Either install it, or make with NO_TCMALLOC=1." && \
		false)
endif



# The unittests use gtest, which uses macros that expand into switch statements which don't contain
# default cases. So we have to remove the -Wswitch-default argument for them.
$(OBJ_DIR)/unittest/%.o: CXXFLAGS:=$(filter-out -Wswitch-default,$(CXXFLAGS))

$(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME): $(OBJS) $(BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    LD $@"
endif
ifeq ($(IGNORE_SOME_ICC_LD_WARNINGS),0)
	$(QUIET) $(CXX) $(LDFLAGS) $(SERVER_UNIT_TEST_OBJS) $(STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME)
else
# Note: that is the copy of the problem description that we have above in server linking section.
# The following line runs CXX in linking mode (supplying only the object files and libraries) and then
# pass the output through a filter which filters out bogus (?) icc-related problems.
# FIXME: figure out why we get these problems
	$(QUIET) ($(CXX) $(LDFLAGS) $(SERVER_UNIT_TEST_OBJS) $(STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME) 2>&1 >&3 | (grep -v "warning: relocation refers to discarded section" || true) >&2) 3>&1
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
	$(QUIET) clang --analyze $(CXXFLAGS) $(SOURCES)

coffeelint:
	-coffeelint -f ../scripts/coffeelint.json -r ../admin/

style: coffeelint
	$(QUIET) ../scripts/check_style.sh

showdefines:
	$(QUIET) $(CXX) $(CXXFLAGS) -m32 -E -dM - < /dev/null

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
	$(QUIET) find -name '*~' -exec rm {} \;
	$(QUIET) rm -rf $(BUILD_ROOT_DIR)

clean-drivers:
	$(QUIET) cd ../drivers ; $(MAKE) $(MFLAGS) clean ;

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
	$(QUIET) $(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cc $(MAKEFILE_DEPENDENCY)
	$(QUIET) mkdir -p $(dir $@)
ifeq ($(VERBOSE),0)
	@echo "    CC $< -o $@"
endif
	$(QUIET) $(CXX) $(CXXFLAGS) -c -o $@ $<

# Dependencies
$(DEP_DIR)/%.d: $(SOURCE_DIR)/%.cc $(PROTO_HEADERS)
ifeq ($(VERBOSE),0)
	@echo "    DEP $@"
endif
	$(QUIET) mkdir -p $(dir $@)
	$(QUIET) $(CXX) $(CXXFLAGS) -M -MQ $(OBJ_DIR)/$*.o -MQ $@ $< > $@

# Without this, the proto headers get generated to build the .d files, then are removed as
# intermediate files; then the .d files are read in, and some depend on the proto headers, so they
# get remade. This is redundant work. Also, it appears to cause nondeterministic races - possibly a
# bug in GNU Make, possibly a bug in our Makefile.
.SECONDARY: $(PROTO_HEADERS)

NO_DEPS_TARGETS:=clean depclean tags etags analyze cscope style $(OPROF_TARGETS)
# Include the dependencies into the makefile so that they take effect
ifneq (,$(if $(MAKECMDGOALS),$(filter-out $(NO_DEPS_TARGETS),$(MAKECMDGOALS)),non-empty-placeholder))
-include $(DEPS)
endif
