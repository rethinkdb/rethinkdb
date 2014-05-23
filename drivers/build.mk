# Copyright 2010-2013 RethinkDB, all rights reserved.

DRIVERS_DIR := $(TOP)/drivers

PROTOC_BASE := $(dir $(patsubst %/,%,$(dir $(PROTOC))))

include $(DRIVERS_DIR)/javascript/build.mk

.PHONY: drivers
drivers: js-driver ruby-driver python-driver

.PHONY: ruby-driver
ruby-driver:
	$(MAKE) -C $(DRIVERS_DIR)/ruby

$(DRIVERS_DIR)/python/rethinkdb/ql2_pb2.py: $(TOP)/src/rdb_protocol/ql2.proto
	$(MAKE) -C $(DRIVERS_DIR)/python

.PHONY: python-driver
python-driver: $(DRIVERS_DIR)/python/rethinkdb/ql2_pb2.py

.PHONY: $(DRIVERS_DIR)/all
ifeq ($(BUILD_DRIVERS), 1)
  $(DRIVERS_DIR)/all: drivers
else
  $(DRIVERS_DIR)/all:
endif
