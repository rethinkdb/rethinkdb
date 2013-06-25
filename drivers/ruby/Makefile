# Copyright 2010-2012 RethinkDB, all rights reserved.
VERBOSE?=0

ifeq ($(VERBOSE),1)
QUIET:=
else
QUIET:=@
endif

SOURCE_DIR?=../../src
RUBY_PBDIR:=../../build/drivers/ruby
RUBY_PBFILE2:=ql2.pb.rb
BUILD_DRIVERS?=1
PROTOCFLAGS:=
GEMSPEC:=rethinkdb.gemspec

ALLOW_INTERNAL_TOOLS?=1
INSTALL_INTERNAL_TOOLS?=0
ifeq ($(ALLOW_INTERNAL_TOOLS),1)
ifeq ($(shell which ruby-protoc || true),)
TC_RPROTOC_EXE:=$(RUBY_PBDIR)/toolchain/bin/ruby-protoc
else
TC_RPROTOC_EXE:=$(shell which ruby-protoc || true)
endif
else
TC_RPROTOC_EXE:=$(shell which ruby-protoc || true)
endif

all: driver-ruby
$(RUBY_PBDIR):
	$(QUIET) mkdir -p $(RUBY_PBDIR)

driver-ruby: $(RUBY_PBDIR)/$(RUBY_PBFILE2)
PROTOFILE2:=ql2.proto
$(RUBY_PBDIR)/$(RUBY_PBFILE2): $(RUBY_PBDIR) $(PROTOFILE2) $(TC_RPROTOC_EXE)
ifeq ($(VERBOSE),0)
	@echo "    PROTOC[RB] $(PROTOFILE2)"
endif
	$(QUIET) $(TC_RPROTOC_EXE) $(PROTOCFLAGS) -o $(RUBY_PBDIR) $(PROTOFILE2)

publish: $(GEMSPEC) driver-ruby
	gem build $(GEMSPEC)
	gem push rethinkdb-*.gem

clean:
ifeq ($(VERBOSE),0)
	@echo "    RM $(RUBY_PBDIR)/$(RUBY_PBFILE)"
endif
	$(QUIET) if [ -e $(RUBY_PBDIR)/$(RUBY_PBFILE) ] ; then rm -f $(RUBY_PBDIR)/$(RUBY_PBFILE) ; fi ;
	$(QUIET) if [ -e $(RUBY_PBDIR)/toolchain ] ; then rm -rf $(RUBY_PBDIR)/toolchain ; fi ;
ifeq ($(VERBOSE),0)
	@echo "    RM $(RUBY_PBDIR)/$(RUBY_PBFILE2)"
endif
	$(QUIET) if [ -e $(RUBY_PBDIR)/$(RUBY_PBFILE2) ] ; then rm -f $(RUBY_PBDIR)/$(RUBY_PBFILE2) ; fi ;
	$(QUIET) if [ -e $(RUBY_PBDIR)/toolchain ] ; then rm -rf $(RUBY_PBDIR)/toolchain ; fi ;

$(TC_RPROTOC_EXE): $(RUBY_PBDIR)
ifeq ($(INSTALL_INTERNAL_TOOLS),1)
# ifeq ($(shell gem list --local | grep "^ruby_protobuf "),)
	if [ -d $(RUBY_PBDIR) ] && [ ! -e $(RUBY_PBDIR)/toolchain ] ; then mkdir -p $(RUBY_PBDIR)/toolchain ; fi ;
	gem install --install-dir $(RUBY_PBDIR)/toolchain ruby_protobuf ;
# endif
endif

