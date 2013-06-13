# Copyright 2010-2013 RethinkDB, all rights reserved.

# The rules in this file can download, build and setup some of the rethinkdb dependencies.
# It is used for portable builds on platforms where these dependencies are not available or are too old.

V8_DEP :=
PROTOBUF_DEP :=
NPM_DEP :=
TCMALLOC_DEP :=
PROTOC_DEP :=
COFFEE_DEP :=

ifdef WGET
  GETURL := $(WGET) --quiet --output-document=-
else ifdef CURL
  GETURL := $(CURL) --silent
endif

ifneq (1,$(FETCH_INTERNAL_TOOLS))
  GETURL = bash -c 'echo "Error: Refusing to download $$0 (needed to build $@)" >&2; echo Run ./configure with --allow-fetch to enable downloads. >&2; false'
else
  PATH := $(abspath $(TOP)/support/usr/bin):$(PATH)
endif

SUPPORT_DIR := $(TOP)/support
SUPPORT_DIR_ABS := $(abspath $(SUPPORT_DIR))
SUPPORT_INST_DIR := $(SUPPORT_DIR)/usr
SUPPORT_LOG_DIR := $(SUPPORT_DIR_ABS)/log
TC_BUILD_DIR := $(SUPPORT_DIR)/build
TC_SRC_DIR := $(SUPPORT_DIR)/src
TOOLCHAIN_DIR := $(SUPPORT_DIR)/toolchain
NODE_MODULES_DIR := $(TOOLCHAIN_DIR)/node_modules
NODE_DIR := $(TC_BUILD_DIR)/node
NODE_SRC_DIR := $(TC_SRC_DIR)/node
PROTOC_DIR := $(TC_BUILD_DIR)/protobuf
PROTOC_SRC_DIR := $(TC_SRC_DIR)/protobuf
GPERFTOOLS_DIR := $(TC_BUILD_DIR)/gperftools
GPERFTOOLS_SRC_DIR := $(TC_SRC_DIR)/gperftools
LIBUNWIND_DIR := $(TC_BUILD_DIR)/libunwind
LIBUNWIND_SRC_DIR := $(TC_SRC_DIR)/libunwind
UNWIND_INT_LIB := # TODO
TCMALLOC_MINIMAL_INT_LIB := $(SUPPORT_INST_DIR)/lib/libtcmalloc_minimal.a
TC_PROTOC_INT_EXE := $(SUPPORT_INST_DIR)/bin/protoc
TC_PROTOC_INT_BIN_DIR := $(SUPPORT_INST_DIR)/bin
TC_PROTOC_INT_LIB_DIR := $(SUPPORT_INST_DIR)/lib
TC_PROTOC_INT_INC_DIR := $(SUPPORT_INST_DIR)/include
PROTOBUF_INT_LIB := $(TC_PROTOC_INT_LIB_DIR)/libprotobuf.a
TC_NODE_INT_EXE := $(SUPPORT_DIR)/usr/bin/node
TC_NPM_INT_EXE := $(SUPPORT_DIR)/usr/bin/npm
TC_LESSC_INT_EXE := $(SUPPORT_DIR)/usr/bin/lessc
TC_COFFEE_INT_EXE := $(SUPPORT_DIR)/usr/bin/coffee
TC_HANDLEBARS_INT_EXE := $(SUPPORT_DIR)/usr/bin/handlebars
V8_SRC_DIR := $(TC_SRC_DIR)/v8
V8_INT_DIR := $(TC_BUILD_DIR)/v8
V8_INT_LIB := $(V8_INT_DIR)/libv8.a

$(shell mkdir -p $(SUPPORT_DIR) $(TOOLCHAIN_DIR) $(TC_BUILD_DIR) $(TC_SRC_DIR))

ifeq (0,$(VERBOSE))
  $(shell mkdir -p $(SUPPORT_LOG_DIR))
  SUPPORT_LOG_PATH = $(SUPPORT_LOG_DIR)/$(notdir $@).log
  SUPPORT_LOG_REDIRECT = > $(SUPPORT_LOG_PATH) 2>&1 || ( echo Error log: $(SUPPORT_LOG_PATH) ; tail -n 40 $(SUPPORT_LOG_PATH) ; false )
else
  SUPPORT_LOG_REDIRECT :=
endif

ifeq ($(PROTOC),$(TC_PROTOC_INT_EXE))
  LD_LIBRARY_PATH ?=
  PROTOC_DEP := $(TC_PROTOC_INT_EXE)
  PROTOC_RUN := env LD_LIBRARY_PATH=$(TC_PROTOC_INT_LIB_DIR):$(LD_LIBRARY_PATH) PATH=$(TC_PROTOC_INT_BIN_DIR):$(PATH) $(PROTOC)
  CXXFLAGS += -isystem $(TC_PROTOC_INT_INC_DIR)
else
  PROTOC_RUN := $(PROTOC)
endif

ifeq ($(V8_INT_LIB),$(V8_LIBS))
  V8_DEP := $(V8_INT_LIB)
  CXXPATHDS += -isystem $(V8_INT_DIR)/include
else
  V8_CXXFLAGS :=
endif

ifeq ($(PROTOBUF_INT_LIB),$(PROTOBUF_LIBS))
  PROTOBUF_DEP := $(PROTOBUF_INT_LIB)
endif

NPM ?= NO_NPM
ifeq ($(NPM),$(TC_NPM_INT_EXE))
  NPM_DEP := $(NPM)
endif

COFFEE ?= NO_COFFEE
ifeq ($(COFFEE),$(TC_COFFEE_INT_EXE))
  COFFEE_DEP := $(COFFEE)
endif

ifeq ($(TCMALLOC_MINIMAL_INT_LIB),$(TCMALLOC_MINIMAL_LIBS))
  TCMALLOC_DEP := $(TCMALLOC_MINIMAL_INT_LIB)
endif

.PHONY: support
support: $(COFFEE_DEP) $(V8_DEP) $(PROTOBUF_DEP) $(NPM_DEP) $(TCMALLOC_DEP) $(PROTOC_DEP)
support: $(LESSC) $(HANDLEBARS)

$(TC_BUILD_DIR)/%: $(TC_SRC_DIR)/%
	$P CP
	rm -rf $@
	cp -pRP $< $@

$(TC_LESSC_INT_EXE): $(NODE_MODULES_DIR)/less | $(dir $(TC_LESSC_INT_EXE)).
	$P LN
	rm -f $@
	ln -s $(abspath $</bin/lessc) $@
	touch $@

$(NODE_MODULES_DIR)/less: $(NPM_DEP) | $(NODE_MODULES_DIR)/.
	$P NPM-I less
	cd $(TOOLCHAIN_DIR) && $(abspath $(NPM)) install less@1.3.3 $(SUPPORT_LOG_REDIRECT)

$(TC_COFFEE_INT_EXE): $(NODE_MODULES_DIR)/coffee-script | $(dir $(TC_COFFEE_INT_EXE)).
	$P LN
	rm -f $@
	ln -s $(abspath $</bin/coffee) $@
	touch $@

$(NODE_MODULES_DIR)/coffee-script: $(NPM_DEP) | $(NODE_MODULES_DIR)/.
	$P NPM-I coffee-script
	cd $(TOOLCHAIN_DIR) && \
	  $(abspath $(NPM)) install coffee-script@1.4.0 $(SUPPORT_LOG_REDIRECT)

$(TC_HANDLEBARS_INT_EXE): $(NODE_MODULES_DIR)/handlebars | $(dir $(TC_HANDLEBARS_INT_EXE)).
	$P LN
	rm -f $@
	ln -s $(abspath $</bin/handlebars) $@
	touch $@

$(NODE_MODULES_DIR)/handlebars: $(NPM_DEP) | $(NODE_MODULES_DIR)/.
	$P NPM-I handlebars
	cd $(TOOLCHAIN_DIR) && \
	  $(abspath $(NPM)) install handlebars@1.0.7 $(SUPPORT_LOG_REDIRECT)

$(V8_SRC_DIR):
	$P SVN-CO v8
	( cd $(TC_SRC_DIR) && \
	  svn checkout http://v8.googlecode.com/svn/tags/3.17.4.1 v8 ) $(SUPPORT_LOG_REDIRECT)

	$P MAKE v8 dependencies
	$(EXTERN_MAKE) -C $(V8_SRC_DIR) dependencies $(SUPPORT_LOG_REDIRECT)

$(V8_INT_LIB): $(V8_INT_DIR)
	$P MAKE v8
	$(EXTERN_MAKE) -C $(V8_INT_DIR) native $(SUPPORT_LOG_REDIRECT)
	$P AR $@
	find $(V8_INT_DIR) -iname "*.o" | grep -v '\/preparser_lib\/' | xargs ar cqs $(V8_INT_LIB);

$(NODE_SRC_DIR):
	$P DOWNLOAD node
	$(GETURL) http://nodejs.org/dist/v0.8.11/node-v0.8.11.tar.gz | ( \
	  cd $(TC_SRC_DIR) && tar -xzf - && rm -rf node && mv node-v0.8.11 node )

$(TC_NPM_INT_EXE): $(TC_NODE_INT_EXE)

$(TC_NODE_INT_EXE): $(NODE_DIR)
	$P MAKE node
	( unset prefix PREFIX DESTDIR MAKEFLAGS MFLAGS && \
	  cd $(NODE_DIR) && \
	  ./configure --prefix=$(SUPPORT_DIR_ABS)/usr && \
	  $(EXTERN_MAKE) prefix=$(SUPPORT_DIR_ABS)/usr DESTDIR=/ && \
	  $(EXTERN_MAKE) install prefix=$(SUPPORT_DIR_ABS)/usr DESTDIR=/ ) $(SUPPORT_LOG_REDIRECT)
	touch $@

$(PROTOC_SRC_DIR):
	$P DOWNLOAD protoc
	$(GETURL) http://protobuf.googlecode.com/files/protobuf-2.5.0.tar.bz2 | ( \
	  cd $(TC_SRC_DIR) && \
	  tar -xjf - && \
	  rm -rf protobuf && \
	  mv protobuf-2.5.0 protobuf )

ifeq ($(COMPILER) $(OS),CLANG Darwin)
  BUILD_PROTOC_ENV := CXX=clang++ CXXFLAGS='-std=c++11 -stdlib=libc++' LDFLAGS=-lc++
else
  BUILD_PROTOC_ENV :=
endif

$(PROTOBUF_INT_LIB): $(TC_PROTOC_INT_EXE)
$(TC_PROTOC_INT_EXE): $(PROTOC_DIR)
	$P MAKE protoc
	( cd $(PROTOC_DIR) && \
	  $(BUILD_PROTOC_ENV) \
	  ./configure --prefix=$(SUPPORT_DIR_ABS)/usr && \
	  $(EXTERN_MAKE) PREFIX=$(SUPPORT_DIR_ABS)/usr prefix=$(SUPPORT_DIR_ABS)/usr DESTDIR=/ && \
	  $(EXTERN_MAKE) install PREFIX=$(SUPPORT_DIR_ABS)/usr prefix=$(SUPPORT_DIR_ABS)/usr DESTDIR=/ ) \
	    $(SUPPORT_LOG_REDIRECT)

$(GPERFTOOLS_SRC_DIR):
	$P DOWNLAOD gperftools
	$(GETURL) http://gperftools.googlecode.com/files/gperftools-2.0.tar.gz | ( \
	  cd $(TC_SRC_DIR) && \
	  tar -xzf - && \
	  rm -rf gperftools && \
	  mv gperftools-2.0 gperftools )

$(LIBUNWIND_SRC_DIR):
	$P DOWNLOAD libunwind
	$(GETURL) http://download.savannah.gnu.org/releases/libunwind/libunwind-1.1.tar.gz | ( \
	  cd $(TC_SRC_DIR) && \
	  tar -xzf - && \
	  rm -rf libunwind && \
	  mv libunwind-1.1 libunwind )

$(LIBUNWIND_DIR): $(LIBUNWIND_SRC_DIR)

# TODO: don't use colonize.sh
# TODO: seperate step and variable for building $(UNWIND_INT_LIB)
$(TCMALLOC_MINIMAL_INT_LIB): $(LIBUNWIND_DIR) $(GPERFTOOLS_DIR)
	$P MAKE libunwind gperftools
	cd $(TOP)/support/build && rm -f native_list.txt semistaged_list.txt staged_list.txt boost_list.txt post_boost_list.txt && touch native_list.txt semistaged_list.txt staged_list.txt boost_list.txt post_boost_list.txt && echo libunwind >> semistaged_list.txt && echo gperftools >> semistaged_list.txt && cp -pRP $(COLONIZE_SCRIPT_ABS) ./ && ( unset PREFIX && unset prefix && unset MAKEFLAGS && unset MFLAGS && unset DESTDIR && bash ./colonize.sh ; )
