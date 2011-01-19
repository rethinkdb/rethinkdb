##### Pretty-printing

ANSI_BOLD_ON:=[1m
ANSI_BOLD_OFF:=[0m
ANSI_UL_ON:=[4m
ANSI_UL_OFF:=[0m

##### Configuring stuff

prefix?=/usr
bindir=$(prefix)/bin
docdir=$(prefix)/share/doc/rethinkdb
mandir=$(prefix)/share/man
man1dir=$(mandir)/man1
bashcompletiondir=/etc/bash_completion.d

SERVER_EXEC_NAME:=rethinkdb
SERVER_UNIT_TEST_NAME:=rethinkdb-unittest

SCRIPTS_DIR:=../scripts
START_DB_NAME:=start_rethinkdb
GDB_FUNCTIONS_NAME:=rethinkdb-gdb.py
TAGS:=.tags
CSCOPE_XREF:=.cscope
RUN_PORT?=11211
RUN_FLAGS?=-s 3 -c 2 -m 10
VALGRIND_FLAGS?=--leak-check=full --db-attach=yes --show-reachable=yes
CALLGRIND_FLAGS?=--simulate-cache=yes --simulate-wb=yes --simulate-hwpref=yes --cacheuse=yes --collect-jumps=yes

# Set SYMBOLS to 1 to enable symbols, even in release mode
SYMBOLS?=0

DEBUG?=1
VALGRIND?=0
SEMANTIC_SERIALIZER_CHECK?=0
MOCK_CACHE_CHECK?=0
VERBOSE?=0
UNIT_TESTS?=0

# Choose our directories
# This makefile is meant to be run from the 'src' directory.
SOURCE_DIR:=.
BUILD_ROOT_DIR:=../build
ifeq (${DEBUG},1)
BUILD_DIR:=$(BUILD_ROOT_DIR)/debug
else
BUILD_DIR:=$(BUILD_ROOT_DIR)/release
endif

MAN_PAGES:=$(wildcard ../docs/man/*)

PACKAGES_DIR:=$(BUILD_ROOT_DIR)/packages
RPM_PACKAGE_DIR:=$(PACKAGES_DIR)/rpm
DEB_PACKAGE_DIR:=$(PACKAGES_DIR)/deb

# Define configuration variables
CXX:=g++
LDFLAGS:=-Wl,--no-as-needed -lpthread -lrt -laio
STATIC_LIBRARIES:=

CXXFLAGS:=-I$(SOURCE_DIR) -Wall -Wextra -Wformat=2 -Wno-unused-parameter -Wswitch-enum -Werror
ifeq ($(UNIT_TESTS),0)
CXXFLAGS+=-Wswitch-default
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

ifneq ($(FAST_PERFMON), 0)
CXXFLAGS+=-DFAST_PERFMON
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
CXX=g++44
CXXFLAGS+=-DLEGACY_LINUX -DNO_EPOLL
BUILD_DIR:=$(BUILD_DIR)-legacy
endif

ifeq ($(NO_EPOLL),1)
CXXFLAGS+=-DNO_EPOLL
BUILD_DIR:=$(BUILD_DIR)-noepoll
endif

ifeq ($(VALGRIND),1)
CXXFLAGS+=-DVALGRIND
BUILD_DIR:=$(BUILD_DIR)-valgrind
else
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
RETHINKDB_PACKAGING_VERSION?=$(RETHINKDB_VERSION)
CXXFLAGS+=-DRETHINKDB_VERSION=\"$(RETHINKDB_VERSION)\"

##### Build targets

# High level build targets

ifeq ($(UNIT_TESTS),1)
all: $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(BUILD_DIR)/$(SERVER_UNIT_TEST_NAME) $(BUILD_DIR)/$(START_DB_NAME) $(BUILD_DIR)/$(GDB_FUNCTIONS_NAME)
else
all: $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(BUILD_DIR)/$(START_DB_NAME) $(BUILD_DIR)/$(GDB_FUNCTIONS_NAME)
endif

ifeq ($(PACKAGING),1)
RPM_BUILD_ROOT=$(RPM_PACKAGE_DIR)/BUILD
DEB_CONTROL_ROOT=$(DEB_PACKAGE_DIR)/DEBIAN
RPM_SPEC_FILE=$(RPM_PACKAGE_DIR)/SPECS/rethinkdb.spec

RETHINKDB_VERSION_RPM=$(subst -,_,$(RETHINKDB_PACKAGING_VERSION))

prepare_deb_package_dirs:
	$(QUIET) mkdir -p $(DEB_PACKAGE_DIR)
	$(QUIET) mkdir -p $(DEB_CONTROL_ROOT)

prepare_rpm_package_dirs:
	$(QUIET) mkdir -p $(RPM_PACKAGE_DIR)
	$(QUIET) for d in BUILD RPMS/x86_64 SOURCES SPECS SRPMS; do mkdir -p $(RPM_PACKAGE_DIR)/$$d; done

install-binaries: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(QUIET) install -m755 -d $(DESTDIR)$(bindir)
	$(QUIET) install -m755 -T $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(DESTDIR)$(bindir)/$(SERVER_EXEC_NAME)
ifeq ($(STRIP_ON_INSTALL),1)
	$(QUIET) strip --strip-unneeded $(DESTDIR)$(bindir)/$(SERVER_EXEC_NAME)
endif

install-manpages: $(MAN_PAGES)
	$(QUIET) install -m755 -d $(DESTDIR)$(man1dir)
# TODO: support other man sections
	$(QUIET) for manpage in $(MAN_PAGES); do \
			gzip -9 < $$manpage | install -m644 -T /dev/stdin $(DESTDIR)$(man1dir)/$$(basename $$manpage).gz; \
		done

install-tools: ../scripts/rethinkdb.bash
	$(QUIET) install -m755 -d $(DESTDIR)$(bashcompletiondir)
	$(QUIET) install -m644 ../scripts/rethinkdb.bash $(DESTDIR)$(bashcompletiondir)/rethinkdb

install-docs:
	$(QUIET) install -m755 -d $(DESTDIR)$(docdir)
	$(QUIET) install -m644 -T ../docs/LICENSE $(DESTDIR)$(docdir)/copyright

install: install-binaries install-manpages install-docs install-tools
install-rpm: install
install-deb: install
	$(QUIET) install -m755 -d $(DESTDIR)$(docdir)
	$(QUIET) sed	-e 's/PACKAGING_VERSION/$(RETHINKDB_PACKAGING_VERSION)/' ../docs/changelog.Debian | \
		gzip -c9 | \
		install -m644 -T /dev/stdin $(DESTDIR)$(docdir)/changelog.Debian.gz


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
	$(QUIET) find $(DEB_PACKAGE_DIR) -type f -printf "/%P\n" | grep '^/etc/' > $(DEB_CONTROL_ROOT)/conffiles

# Copy {pre,post}{inst,rm} scripts
	$(QUIET) for script in preinst postinst prerm postrm; do		\
			cp ../packaging/debian/$${script} $(DEB_CONTROL_ROOT);	\
			chmod 0755 $(DEB_CONTROL_ROOT)/$${script};							\
		done

# Replace version/size fields in the deb control file
	$(QUIET) disk_size=$$(du -s -k $(DEB_PACKAGE_DIR) | cut -f1);		\
		sed																														\
				-e "s/VERSION/$(RETHINKDB_PACKAGING_VERSION)/"						\
				-e "s/DISK_SIZE/$${disk_size}/"														\
			../packaging/debian/control >$(DEB_CONTROL_ROOT)/control

# Finally, produce a debian package
	$(QUIET) fakeroot dpkg-deb -b $(DEB_PACKAGE_DIR) $(PACKAGES_DIR)

deb: all prepare_deb_package_dirs
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=0 RETHINKDB_PACKAGING_VERSION=$(RETHINKDB_VERSION)_unstripped build-deb
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=1 RETHINKDB_PACKAGING_VERSION=$(RETHINKDB_VERSION) build-deb

# Print a nice message with the location of the created package
	@echo
	@echo "$(ANSI_BOLD_ON)Your DEB packages are here (with lintian output following each package):$(ANSI_BOLD_OFF)"
	$(QUIET) for f in $(PACKAGES_DIR)/*.deb; do \
			deb_name=$(realpath $(PACKAGES_DIR))/$$(basename $$f); \
			echo "  $(ANSI_UL_ON)$${deb_name}$(ANSI_UL_OFF)"; \
			lintian --color auto $${deb_name}; \
		done
	$(QUIET) echo

build-rpm: all prepare_rpm_package_dirs
	@echo
	@echo "$(ANSI_BOLD_ON)Building RPM package for RethinkDB version $(RETHINKDB_PACKAGING_VERSION)$(ANSI_BOLD_OFF)"
	@echo

# Replace version/topdir fields in the spec file
	$(QUIET) sed 																				\
			-e "s/VERSION/$(RETHINKDB_VERSION_RPM)/"				\
			-e "s:TOPDIR:$(realpath $(RPM_PACKAGE_DIR)):"		\
		../packaging/rpm.spec >$(RPM_SPEC_FILE)

# Copy files to their correct places
	$(QUIET) $(MAKE) DESTDIR=$(RPM_BUILD_ROOT) install-rpm

# Produce RPM package
# FIXME: write rpmbuild output out to stderr if there's an error
	$(QUIET) rpmbuild -bb --target=x86_64 --buildroot $(realpath $(RPM_BUILD_ROOT)) $(RPM_SPEC_FILE) > /tmp/rpmbuild.stdout

rpm: all prepare_rpm_package_dirs
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=0 RETHINKDB_VERSION=$(RETHINKDB_VERSION)-unstripped build-rpm
	$(QUIET) $(MAKE) STRIP_ON_INSTALL=1 RETHINKDB_VERSION=$(RETHINKDB_VERSION) build-rpm

# Print a nice message with the location of the created package
	@echo
	@echo "$(ANSI_BOLD_ON)Your RPM packages are here:$(ANSI_BOLD_OFF)"
	$(QUIET) for f in $(RPM_PACKAGE_DIR)/RPMS/x86_64/*.rpm; do	\
			rpm_name=$(realpath $(PACKAGES_DIR))/$$(basename $$f);	\
			mv $$f $$rpm_name;																			\
			echo "  $(ANSI_UL_ON)$${rpm_name}$(ANSI_UL_OFF)";				\
		done
	$(QUIET) echo
else
deb: clean
	@$(MAKE) PACKAGING=1 DEBUG=0 SYMBOLS=1 deb
rpm: clean
	@$(MAKE) PACKAGING=1 DEBUG=0 SYMBOLS=1 rpm
endif


$(BUILD_DIR)/$(SERVER_EXEC_NAME): $(OBJS) $(BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    LD $@"
endif
	$(QUIET) $(CXX) $(LDFLAGS) $(SERVER_EXEC_OBJS) $(STATIC_LIBRARY_PATHS) -o $(BUILD_DIR)/$(SERVER_EXEC_NAME)
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
	$(QUIET) rm -f rethinkdb_data*
	$(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) $(RUN_FLAGS)

rerun: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) $(RUN_FLAGS)

gdb: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(QUIET) rm -f rethinkdb_data*
	gdb --args $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) $(RUN_FLAGS)

regdb: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	gdb --args $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) $(RUN_FLAGS)

sembuild: clean
	make SEMANTIC_SERIALIZER_CHECK=1 all

valgrind: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
ifeq ($(VALGRIND),0)
	$(error Using the 'valgrind' target doesn't automatically set VALGRIND to 1. Try again with \
'make valgrind VALGRIND=1')
endif
	$(QUIET) rm -f rethinkdb_data*
	valgrind $(VALGRIND_FLAGS) $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) $(RUN_FLAGS)

revalgrind: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
ifeq ($(VALGRIND),0)
	$(error Using the 'revalgrind' target doesn't automatically set VALGRIND to 1. Try again with \
'make valgrind VALGRIND=1')
endif
	valgrind $(VALGRIND_FLAGS) $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) $(RUN_FLAGS)

callgrind: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
ifeq ($(CALLGRIND),0)
	$(error Using the 'callgrind' target doesn't automatically set CALLGRIND to 1. Try again with \
'make calgrind CALLGRIND=1')
endif
	$(QUIET) rm -f rethinkdb_data*
	valgrind --tool=callgrind $(CALLGRIND_FLAGS) $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) $(RUN_FLAGS)

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
	@echo "    RM $(TAGS)"
	@echo "    RM $(CSCOPE_XREF)"
endif
	$(QUIET) find -name '*~' -exec rm {} \;
	$(QUIET) rm -rf $(BUILD_ROOT_DIR)
	$(QUIET) rm -f $(TAGS) $(CSCOPE_XREF)

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
-include $(DEPS)
