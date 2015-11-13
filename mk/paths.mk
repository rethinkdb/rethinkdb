# Copyright 2010-2013 RethinkDB, all rights reserved.

# This makefile defines paths that are needed by the other Makefiles

SCRIPTS_DIR := $(TOP)/scripts
SOURCE_DIR := $(TOP)/src
BUILD_ROOT_DIR := $(TOP)/build
PACKAGING_DIR := $(TOP)/packaging
PACKAGES_DIR := $(BUILD_ROOT_DIR)/packages
JS_BUILD_DIR := $(TOP)/build/drivers/javascript
SUPPORT_SRC_DIR := $(TOP)/external
SUPPORT_BUILD_DIR := $(BUILD_ROOT_DIR)/external
SUPPORT_LOG_DIR := $(SUPPORT_BUILD_DIR)

# If the BUILD_DIR is not set, generate a name that depends on the different settings
ifeq ($(BUILD_DIR),)
  ifeq ($(DEBUG),1)
    BUILD_DIR := $(BUILD_ROOT_DIR)/debug
  else
    BUILD_DIR := $(BUILD_ROOT_DIR)/release
  endif

  ifeq ($(COMPILER), CLANG)
    BUILD_DIR += clang
  else ifeq ($(COMPILER), INTEL)
      BUILD_DIR += intel
  endif

  ifeq (1,$(LEGACY_LINUX))
    BUILD_DIR += legacy
  endif

  ifeq (1,$(LEGACY_GCC))
    BUILD_DIR += legacy-gcc
  endif

  ifeq (1,$(NO_EVENTFD))
    BUILD_DIR += noeventfd
  endif

  ifeq (1,$(NO_EPOLL))
    BUILD_DIR += noepoll
  endif

  ifeq (1,$(VALGRIND))
    BUILD_DIR += valgrind
  endif

  ifeq (1,$(THREADED_COROUTINES))
    BUILD_DIR += threaded
  endif

  ifeq (1,$(CORO_PROFILING))
    BUILD_DIR += coro-prof
  endif

  ifneq ($(DEFAULT_ALLOCATOR),$(ALLOCATOR))
    BUILD_DIR += $(ALLOCATOR)
  endif

  BUILD_DIR := $(subst $(space),_,$(BUILD_DIR))
endif
BUILD_DIR_ABS = $(realpath $(BUILD_DIR))

GDB_FUNCTIONS_NAME := rethinkdb-gdb.py

PACKAGE_NAME := $(VANILLA_PACKAGE_NAME)
SERVER_UNIT_TEST_NAME := $(SERVER_EXEC_NAME)-unittest

EXTERNAL_DIR := $(TOP)/external
EXTERNAL_DIR_ABS := $(abspath $(EXTERNAL_DIR))
COLONIZE_SCRIPT := $(EXTERNAL_DIR)/colonist/colonize.sh
COLONIZE_SCRIPT_ABS := $(EXTERNAL_DIR_ABS)/colonist/colonize.sh

PROTO_FILE_SRC := $(TOP)/src/rdb_protocol/ql2.proto
PROTO_DIR := $(BUILD_DIR)/proto

DEP_DIR := $(BUILD_DIR)/dep
OBJ_DIR := $(BUILD_DIR)/obj

WEB_ASSETS_DIR_NAME := web_assets
WEB_ASSETS_BUILD_DIR := $(BUILD_ROOT_DIR)/$(WEB_ASSETS_DIR_NAME)

PRECOMPILED_DIR := $(TOP)/precompiled

##### To rebuild when Makefiles change

ifeq ($(IGNORE_MAKEFILE_CHANGES),1)
  MAKEFILE_DEPENDENCY :=
else
  MAKEFILE_DEPENDENCY = $(filter %Makefile,$(MAKEFILE_LIST)) $(filter %.mk,$(MAKEFILE_LIST))
endif

##### Paths occasionally left undefined by the configure script

NPM ?= false
NPM_BIN_DEP ?= requires-missing-npm
COFFEE_BIN_DEP ?= requires-missing-coffee
BROWSERIFY_BIN_DEP ?= requires-missing-browserify

requires-missing-%:
	$(error '$*' is required but was not found)
