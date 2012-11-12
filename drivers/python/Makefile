# Copyright 2010-2012 RethinkDB, all rights reserved.
VERBOSE?=0

ifeq ($(VERBOSE),1)
QUIET:=
else
QUIET:=@
endif

SOURCE_DIR?=../../src
PYTHON_PBDIR:=../../build/drivers/python
PYTHON_PBFILE:=query_language_pb2.py
RUBY_PBDIR:=../../build/drivers/ruby
RUBY_PBFILE:=query_language.pb.rb
BUILD_DRIVERS?=1
PROTOCFLAGS:= --proto_path=$(SOURCE_DIR)
all: driver-python
$(PYTHON_PBDIR):
	$(QUIET) mkdir -p $(PYTHON_PBDIR)

driver-python: $(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE)
PROTOFILE:=$(SOURCE_DIR)/rdb_protocol/query_language.proto
PROTOPATH:=$(SOURCE_DIR)/rdb_protocol
$(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE): $(PYTHON_PBDIR) $(PROTOFILE)
ifeq ($(VERBOSE),0)
	@echo "    PROTOC[PY] $(PROTOFILE)"
endif
	$(QUIET) protoc $(PROTOCFLAGS) --python_out $(PYTHON_PBDIR) $(PROTOFILE)

TMP_PKG_DIR=/tmp/py_pkg

publish: $(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE)
	mkdir -p $(TMP_PKG_DIR)
	cp -r setup.py $(TMP_PKG_DIR)
	cp -r rethinkdb $(TMP_PKG_DIR)
	rm $(TMP_PKG_DIR)/rethinkdb/$(PYTHON_PBFILE)
	cp $(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE) $(TMP_PKG_DIR)/rethinkdb
	cd $(TMP_PKG_DIR); python setup.py register sdist upload

clean:
ifeq ($(VERBOSE),0)
	@echo "    RM $(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE)"
endif
	$(QUIET) if [ -e $(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE) ] ; then rm $(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE) ; fi ;

