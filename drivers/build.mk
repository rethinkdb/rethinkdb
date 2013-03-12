# Copyright 2010-2013 RethinkDB, all rights reserved.

DRIVERS_DIR := $(TOP)/drivers

PROTOC_BASE := $(dir $(patsubst %/,%,$(dir $(PROTOC))))

include $(DRIVERS_DIR)/javascript/src/build.mk
# TODO
# include $(DRIVERS_DIR)/python/build.mk
# include $(DRIVERS_DIR)/ruby/build.mk

.PHONY: drivers
ifeq ($(BUILD_DRIVERS), 1)
  drivers: js-driver # TODO: driver-ruby driver-python 
else
  drivers: js-driver
endif

.PHONY: $(DRIVERS_DIR)/all
$(DRIVERS_DIR)/all: drivers
