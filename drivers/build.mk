# Copyright 2010-2015 RethinkDB, all rights reserved.

DRIVERS_DIR := $(TOP)/drivers

include $(DRIVERS_DIR)/javascript/build.mk
include $(DRIVERS_DIR)/python/build.mk
include $(DRIVERS_DIR)/ruby/build.mk
include $(DRIVERS_DIR)/java/build.mk

.PHONY: drivers
drivers: js-driver rb-driver py-driver

.PHONY: $(DRIVERS_DIR)/all
$(DRIVERS_DIR)/all: drivers
