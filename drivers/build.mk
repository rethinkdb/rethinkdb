# Copyright 2010-2013 RethinkDB, all rights reserved.

DRIVERS_DIR := $(TOP)/drivers

PROTOC_BASE := $(dir $(patsubst %/,%,$(dir $(PROTOC))))

PYTHON_PBDIR := $(BUILD_DIR)/python
PYTHON_PBFILE := query_language_pb2.py
RUBY_PBDIR := $(BUILD_DIR)/ruby
RUBY_PBFILE := query_language.pb.rb
PROTOCFLAGS := --proto_path=$(SOURCE_DIR)

include $(DRIVERS_DIR)/javascript/build.mk
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
