SOURCE_DIR?=../src
PYTHON_PBDIR:=../build/drivers/python
PYTHON_PBFILE:=query_language_pb2.py
RUBY_PBDIR:=../build/drivers/ruby
RUBY_PBFILE:=query_language.pb.rb
BUILD_DRIVERS?=1
PROTOCFLAGS:= --proto_path=$(SOURCE_DIR)
all: drivers
drivers: driver-javascript driver-python driver-ruby
$(PYTHON_PBDIR):
	$(QUIET) mkdir -p $(PYTHON_PBDIR)
$(RUBY_PBDIR):
	$(QUIET) mkdir -p $(RUBY_PBDIR)

driver-python: $(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE)
PROTOFILE:=$(SOURCE_DIR)/rdb_protocol/query_language.proto
PROTOPATH:=$(SOURCE_DIR)/rdb_protocol
$(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE): $(PYTHON_PBDIR) $(PROTOFILE)
ifeq ($(VERBOSE),0)
	@echo "    PROTOC[PY] $(PROTOFILE)"
endif
	$(QUIET) protoc $(PROTOCFLAGS) --python_out $(PYTHON_PBDIR) $(PROTOFILE)

driver-ruby: $(RUBY_PBDIR)/$(RUBY_PBFILE)
PROTOFILE:=$(SOURCE_DIR)/rdb_protocol/query_language.proto
PROTOPATH:=$(SOURCE_DIR)/rdb_protocol
$(RUBY_PBDIR)/$(RUBY_PBFILE): $(RUBY_PBDIR) $(PROTOFILE)
ifeq ($(VERBOSE),0)
	@echo "    PROTOC[RB] $(PROTOFILE)"
endif
	$(QUIET) rprotoc $(PROTOCFLAGS) --out $(RUBY_PBDIR) $(PROTOFILE) | \
		(grep -v 'writing...' || true)

driver-javascript:
	cd javascript ; $(MAKE) $(MFLAGS) ; cd .. ;

