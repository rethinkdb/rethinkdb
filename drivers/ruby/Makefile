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
all: driver-ruby
$(RUBY_PBDIR):
	$(QUIET) mkdir -p $(RUBY_PBDIR)

driver-ruby: $(RUBY_PBDIR)/$(RUBY_PBFILE)
PROTOFILE:=$(SOURCE_DIR)/rdb_protocol/query_language.proto
PROTOPATH:=$(SOURCE_DIR)/rdb_protocol
$(RUBY_PBDIR)/$(RUBY_PBFILE): $(RUBY_PBDIR) $(PROTOFILE)
ifeq ($(VERBOSE),0)
	@echo "    PROTOC[RB] $(PROTOFILE)"
endif
	$(QUIET) rprotoc $(PROTOCFLAGS) --out $(RUBY_PBDIR) $(PROTOFILE) | \
		(grep -v 'writing...' || true)

clean:
ifeq ($(VERBOSE),0)
	@echo "    RM $(RUBY_PBDIR)/$(RUBY_PBFILE)"
endif
	$(QUIET) if [ -e $(RUBY_PBDIR)/$(RUBY_PBFILE) ] ; then rm $(RUBY_PBDIR)/$(RUBY_PBFILE) ; fi ;

