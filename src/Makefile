space=$(eval) $(eval)
comma=,

##### Pretty-printing

ANSI_BOLD_ON:=[1m
ANSI_BOLD_OFF:=[0m
ANSI_UL_ON:=[4m
ANSI_UL_OFF:=[0m

##### Configuring stuff

# Set SOLO to 0 to build RethinkDB Cache (otherwise, RethinkDB Solo is built)
SOLO?=1
ifeq ($(SOLO),1)
RETHINKDB_CACHE_PRODUCT:=0
RETHINKDB_SOLO_PRODUCT:=1
PRODUCT_NAME:=RethinkDB Solo
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
TAGS:=.tags
CSCOPE_XREF:=.cscope
RUN_PORT?=11211
RUN_FLAGS?=-s 3 -c 2 -m 10
RUN_FILE?=rethinkdb_data
VALGRIND_FLAGS?=--leak-check=full --db-attach=yes --show-reachable=yes --suppressions=../scripts/rethinkdb-valgrind-suppressions.supp
CALLGRIND_FLAGS?=--simulate-cache=yes --simulate-wb=yes --simulate-hwpref=yes --cacheuse=yes --collect-jumps=yes

# Set SYMBOLS to 1 to enable symbols, even in release mode
SYMBOLS?=0

DEBUG?=1
VALGRIND?=0
NO_TCMALLOC?=0
SEMANTIC_SERIALIZER_CHECK?=0
MOCK_CACHE_CHECK?=0
VERBOSE?=0
UNIT_TESTS?=$(DEBUG)

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

PACKAGES_DIR:=$(BUILD_ROOT_DIR)/packages
RPM_PACKAGE_DIR:=$(PACKAGES_DIR)/rpm
DEB_PACKAGE_DIR:=$(PACKAGES_DIR)/deb

# Define configuration variables
CXX:=g++
LDFLAGS:=-Wl,--no-as-needed -lpthread -lrt -laio
STATIC_LIBRARIES:=

CXXFLAGS?=

CXXFLAGS+=-I$(SOURCE_DIR) -Wall -Wextra -Wformat=2 -Wswitch-enum -Werror
CXXFLAGS+="-DPRODUCT_NAME=\"$(PRODUCT_NAME)\""

ifeq ($(SOLO),1)
CXXFLAGS+=-DRETHINKDB_SOLO_PRODUCT
else
CXXFLAGS+=-DRETHINKDB_CACHE_PRODUCT
endif

ifeq ($(UNIT_TESTS),0)
CXXFLAGS+=-Wswitch-default
endif

ifeq ($(AGRESSIVE_BUF_UNLOADING),1)
CXXFLAGS+=-DAGRESSIVE_BUF_UNLOADING=1
endif

# Configure libcoroutine
CXXFLAGS+=-DUSE_UCONTEXT

STRIP_ON_INSTALL?=1

# Configure debug vs. release
ifeq ($(DEBUG),1)
SYMBOLS:=1
CXXFLAGS+=-O0
# -rdynamic is necessary so that backtrace_symbols() works properly
LDFLAGS+=-rdynamic
else
# use -fno-strict-aliasing to not break things
# march=native used to break the serializer
CXXFLAGS+=-O3 -DNDEBUG -fno-strict-aliasing # -match=native
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
CXXFLAGS+=-g
endif

ifeq ($(SEMANTIC_SERIALIZER_CHECK),1)
CXXFLAGS+=-DSEMANTIC_SERIALIZER_CHECK
endif

ifeq ($(MOCK_CACHE_CHECK),1)
CXXFLAGS+=-DMOCK_CACHE_CHECK
BUILD_DIR:=$(BUILD_DIR)-mockcache
endif

ifeq ($(MOCK_IO_LAYER),1)
CXXFLAGS+=-DMOCK_IO_LAYER
BUILD_DIR:=$(BUILD_DIR)-mockio
endif

ifeq ($(BTREE_DEBUG),1)
CXXFLAGS+=-DBTREE_DEBUG
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

ifeq ($(VALGRIND),1)
CXXFLAGS+=-DVALGRIND
BUILD_DIR:=$(BUILD_DIR)-valgrind
NO_TCMALLOC:=1
endif

ifeq ($(NO_TCMALLOC),0)
STATIC_LIBRARIES+=tcmalloc_minimal
endif

# look for the static library in the same directory as the .so file
STATIC_LIBRARY_PATHS:=$(foreach lib,$(STATIC_LIBRARIES),$(shell /sbin/ldconfig -p | awk '/lib$(lib).so / { gsub("\\.so$$", ".a", $$NF); print $$NF; exit 0; }'))

# Should makefile be noisy?
ifeq ($(VERBOSE),1)
QUIET:=
else
QUIET:=@
endif

# googletest flags
ifeq ($(UNIT_TESTS),1)
LDFLAGS+=-lgtest
endif

##### Finding what to build

DEP_DIR:=$(BUILD_DIR)/dep
OBJ_DIR:=$(BUILD_DIR)/obj

ifeq ($(UNIT_TESTS),1)
SOURCES:=$(shell find $(SOURCE_DIR) -name '*.cc')
else
SOURCES:=$(shell find $(SOURCE_DIR) -name '*.cc' | grep -vF "`find $(SOURCE_DIR)/unittest`")
endif

NAMES:=$(patsubst $(SOURCE_DIR)/%.cc,%,$(SOURCES))
DEPS:=$(patsubst %,$(DEP_DIR)/%.d,$(NAMES))
OBJS:=$(patsubst %,$(OBJ_DIR)/%.o,$(NAMES))

SERVER_EXEC_OBJS:=$(patsubst %.cc,$(OBJ_DIR)/%.o,$(shell find $(SOURCE_DIR) -name '*.cc' | grep -vF "`find $(SOURCE_DIR)/unittest`"))

SERVER_NOMAIN_OBJS:=$(patsubst %.cc,$(OBJ_DIR)/%.o,$(shell find $(SOURCE_DIR) -name '*.cc' | grep -vF "`find $(SOURCE_DIR) -name 'main.cc'`"))

SERVER_UNIT_TEST_OBJS:=$(SERVER_NOMAIN_OBJS) $(OBJ_DIR)/unittest/main.o

#### Version number handling
RETHINKDB_VERSION:=$(shell ../scripts/gen-version.sh)
RETHINKDB_SHORT_VERSION:=$(shell ../scripts/gen-version.sh -s)
PACKAGING_ALTERNATIVES_PRIORITY:=$(shell echo $$(../scripts/gen-version.sh -r)/100 | bc)

# ifneq ($(TIMEBOMB),0)
# RETHINKDB_VERSION:=$(RETHINKDB_VERSION)-trial
# RETHINKDB_SHORT_VERSION:=$(RETHINKDB_SHORT_VERSION)-trial
# endif
RETHINKDB_PACKAGING_VERSION?=$(RETHINKDB_VERSION)
CXXFLAGS+=-DRETHINKDB_VERSION=\"$(RETHINKDB_VERSION)\"

SERVER_EXEC_NAME_VERSIONED:=$(SERVER_EXEC_NAME)-$(RETHINKDB_SHORT_VERSION)

##### Build targets
.PHONY: all install-binaries install-manpages install-tools install install-rpm install-deb build-deb deb build-rpm rpm rpm-suse10 run rerun gdb regdb sembuild valgrind revalgrind callgrind

# High level build targets

ifeq ($(UNIT_TESTS),1)
all: $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME) $(BUILD_DIR)/$(START_DB_NAME) $(BUILD_DIR)/$(GDB_FUNCTIONS_NAME)
else
all: $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(BUILD_DIR)/$(START_DB_NAME) $(BUILD_DIR)/$(GDB_FUNCTIONS_NAME)
endif

unit:
	$(MAKE) UNIT_TESTS=1
	$(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME)

# Packaging
ifeq ($(PACKAGING),1)
PACKAGE_FOR_SUSE_10?=0
RPM_SPEC_INPUT?=../packaging/rpm.spec 
DEBIAN_PKG_DIR:=debian
SUPPRESSED_LINTIAN_TAGS:=new-package-should-close-itp-bug
RPM_BUILD_ROOT=$(RPM_PACKAGE_DIR)/BUILD
DEB_CONTROL_ROOT=$(DEB_PACKAGE_DIR)/DEBIAN
RPM_SPEC_FILE=$(RPM_PACKAGE_DIR)/SPECS/rethinkdb.spec

RETHINKDB_VERSION_RPM=$(subst -,_,$(RETHINKDB_PACKAGING_VERSION))
DEB_PACKAGE_REVISION?=1
RETHINKDB_VERSION_DEB=$(RETHINKDB_PACKAGING_VERSION)-$(DEB_PACKAGE_REVISION)

VERSIONED_PACKAGE_NAME:=$(PACKAGE_NAME)-$(RETHINKDB_SHORT_VERSION)
VERSIONED_PRODUCT_SHARE_DIR:=/usr/share/$(VERSIONED_PACKAGE_NAME)

prefix?=/usr
bin_dir:=$(prefix)/bin
doc_dir:=$(prefix)/share/doc/$(VERSIONED_PACKAGE_NAME)
man_dir:=$(prefix)/share/man
man1_dir:=$(man_dir)/man1
share_dir:=$(prefix)/share/$(VERSIONED_PACKAGE_NAME)
bash_completion_dir:=/etc/bash_completion.d
internal_bash_completion_dir:=$(share_dir)$(bash_completion_dir)

prepare_deb_package_dirs:
	$(QUIET) mkdir -p $(DEB_PACKAGE_DIR)
	$(QUIET) mkdir -p $(DEB_CONTROL_ROOT)

prepare_rpm_package_dirs:
	$(QUIET) mkdir -p $(RPM_PACKAGE_DIR)
	$(QUIET) for d in BUILD RPMS/x86_64 SOURCES SPECS SRPMS; do mkdir -p $(RPM_PACKAGE_DIR)/$$d; done

install-binaries: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(QUIET) install -m755 -d $(DESTDIR)$(bin_dir)
	$(QUIET) install -m755 -T $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(DESTDIR)$(bin_dir)/$(SERVER_EXEC_NAME_VERSIONED)
ifeq ($(STRIP_ON_INSTALL),1)
	$(QUIET) strip --strip-unneeded $(DESTDIR)$(bin_dir)/$(SERVER_EXEC_NAME_VERSIONED)
endif

install-manpages: ../docs/man/rethinkdb.1
	$(QUIET) install -m755 -d $(DESTDIR)$(man1_dir)
# TODO: support other man pages
	$(QUIET) gzip -9 < ../docs/man/rethinkdb.1 | install -m644 -T /dev/stdin $(DESTDIR)$(man1_dir)/$(VERSIONED_PACKAGE_NAME).1.gz;
#	$(QUIET) for manpage in $(MAN_PAGES); do \
#			gzip -9 < $$manpage | install -m644 -T /dev/stdin $(DESTDIR)$(man1_dir)/$$(basename $$manpage).gz; \
#		done

install-tools: ../scripts/rethinkdb.bash
	$(QUIET) install -m755 -d $(DESTDIR)$(internal_bash_completion_dir)
	$(QUIET) install -m755 -d $(DESTDIR)$(bash_completion_dir)
	$(QUIET) install -m644 ../scripts/rethinkdb.bash $(DESTDIR)$(internal_bash_completion_dir)/$(SERVER_EXEC_NAME).bash

install-docs:
	$(QUIET) install -m755 -d $(DESTDIR)$(doc_dir)
	$(QUIET) install -m644 -T ../docs/LICENSE $(DESTDIR)$(doc_dir)/copyright

install: install-binaries install-manpages install-docs install-tools
install-rpm: install
install-deb: install
	$(QUIET) install -m755 -d $(DESTDIR)$(doc_dir)
	$(QUIET) sed	-e 's/PACKAGING_VERSION/$(RETHINKDB_VERSION_DEB)/' ../docs/changelog.Debian | \
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
				-D "PACKAGE_VERSION=$(RETHINKDB_VERSION_DEB)"											\
				-D "LEGACY_LINUX=$(LEGACY_LINUX)"																	\
				-D "PACKAGE_FOR_SUSE_10=$(PACKAGE_FOR_SUSE_10)"								    \
				-D "TRIAL=$(TRIAL)"																						    \
				-D "BIN_DIR=$(bin_dir)"																						\
				-D "DOC_DIR=$(doc_dir)"																						\
				-D "MAN1_DIR=$(man1_dir)"																					\
				-D "BASH_COMPLETION_DIR=$(bash_completion_dir)" 									\
				-D "INTERNAL_BASH_COMPLETION_DIR=$(internal_bash_completion_dir)"	\
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

# Build rules

$(BUILD_DIR)/$(SERVER_EXEC_NAME): $(OBJS) $(BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    LD $@"
endif
	$(QUIET) $(CXX) $(LDFLAGS) $(SERVER_EXEC_OBJS) $(STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_EXEC_NAME)
ifeq ($(NO_TCMALLOC),0)
	@objdump -T $(BUILD_DIR)/$(SERVER_EXEC_NAME) | c++filt | grep -q "tcmalloc::" || \
		(echo "    Failed to link in TCMalloc. Either install it, or make with NO_TCMALLOC=1." && \
		false)
endif
	@echo "    Finished building RethinkDB key-value store server version '${RETHINKDB_VERSION}'"

$(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME): $(OBJS) $(BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    LD $@"
endif
	$(QUIET) $(CXX) $(LDFLAGS) $(SERVER_UNIT_TEST_OBJS) $(STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME)




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
	$(QUIET) rm -f $(RUN_FILE)
	valgrind $(VALGRIND_FLAGS) $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) -f $(RUN_FILE) $(RUN_FLAGS)

revalgrind: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
ifeq ($(VALGRIND),0)
	$(error Using the 'revalgrind' target doesn't automatically set VALGRIND to 1. Try again with \
'make valgrind VALGRIND=1')
endif
	valgrind $(VALGRIND_FLAGS) $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) -f $(RUN_FILE) $(RUN_FLAGS)

callgrind: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
ifeq ($(CALLGRIND),0)
	$(error Using the 'callgrind' target doesn't automatically set CALLGRIND to 1. Try again with \
'make calgrind CALLGRIND=1')
endif
	$(QUIET) rm -f $(RUN_FILE)
	valgrind --tool=callgrind $(CALLGRIND_FLAGS) $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) -f $(RUN_FILE) $(RUN_FLAGS)

tags:
	$(QUIET) ctags -R -f $(TAGS) --langmap="c++:.cc.tcc.hpp"

cscope:
	$(QUIET) cscope -bR -f $(CSCOPE_XREF)

style:
	$(QUIET) find . -name \*.cc -o -name \*.hpp | xargs ../scripts/cpplint --verbose 2 --filter=-whitespace/end_of_line,-whitespace/parens,-whitespace/line_length,-readability/casting,-whitespace/braces,-readability/todo,-legal/copyright,-whitespace/comments,-build/include,-whitespace/labels,-runtime/int,-runtime/printf 2>&1 | grep -v Done\ processing

showdefines:
	$(QUIET) $(CXX) $(CXXFLAGS) -m32 -E -dM - < /dev/null

clean:
ifeq ($(VERBOSE),0)
	@echo "    RM *~"
	@echo "    RM -r $(BUILD_ROOT_DIR)"
#	@echo "    RM $(TAGS)"
#	@echo "    RM $(CSCOPE_XREF)"
endif
	$(QUIET) find -name '*~' -exec rm {} \;
	$(QUIET) rm -rf $(BUILD_ROOT_DIR)
#	$(QUIET) rm -f $(TAGS) $(CSCOPE_XREF)

# Directories
$(BUILD_DIR):
	$(QUIET) mkdir -p $(BUILD_DIR)

# Object files

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cc Makefile
	$(QUIET) mkdir -p $(dir $@)
ifeq ($(VERBOSE),0)
	@echo "    CC $< -o $@"
endif
	$(QUIET) $(CXX) $(CXXFLAGS) -c -o $@ $<

# Dependencies
$(DEP_DIR)/%.d: $(SOURCE_DIR)/%.cc
	$(QUIET) mkdir -p $(dir $@)
	$(QUIET) $(CXX) $(CXXFLAGS) -M $< > $@.$$$$;				   \
	sed 's,$(notdir $*)\.o[ :]*,$(OBJ_DIR)/$*.o $@ : ,g' < $@.$$$$ > $@;	  \
	rm -f $@.$$$$
# Include the dependencies into the makefile so that they take effect

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
