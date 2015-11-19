# Copyright 2010-2013 RethinkDB, all rights reserved.

DRIVERS_DIR := $(TOP)/drivers

include $(DRIVERS_DIR)/javascript/build.mk
include $(DRIVERS_DIR)/python/build.mk
include $(DRIVERS_DIR)/ruby/build.mk
include $(DRIVERS_DIR)/java/build.mk

.PHONY: drivers
drivers: js-driver rb-driver py-driver java-driver

.PHONY: $(DRIVERS_DIR)/all
ifeq (1,$(USE_PRECOMPILED_WEB_ASSETS))
  $(DRIVERS_DIR)/all: rb-driver py-driver
else
  $(DRIVERS_DIR)/all: drivers
endif
