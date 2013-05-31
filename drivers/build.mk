# Copyright 2010-2013 RethinkDB, all rights reserved.

DRIVERS_DIR := $(TOP)/drivers

PROTOC_BASE := $(dir $(patsubst %/,%,$(dir $(PROTOC))))

include $(DRIVERS_DIR)/javascript/src/build.mk

.PHONY: drivers
ifeq ($(BUILD_DRIVERS), 1)
  drivers: js-driver ruby-driver python-driver
else
  drivers: js-driver
endif

$(DRIVERS_DIR)/ruby/lib/ql2.pb.rb: $(TOP)/src/rdb_protocol/ql2.proto
	$(MAKE) -C $(DRIVERS_DIR)/ruby

.PHONY: ruby-driver
ruby-driver: $(DRIVERS_DIR)/ruby/lib/ql2.pb.rb

$(DRIVERS_DIR)/python/rethinkdb/ql2_pb2.py: $(TOP)/src/rdb_protocol/ql2.proto
	$(MAKE) -C $(DRIVERS_DIR)/python

.PHONY: python-driver
python-driver: $(DRIVERS_DIR)/python/rethinkdb/ql2_pb2.py

.PHONY: $(DRIVERS_DIR)/all
$(DRIVERS_DIR)/all: drivers
