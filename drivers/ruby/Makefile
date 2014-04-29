# Copyright 2010-2014 RethinkDB, all rights reserved.

PROTOCFLAGS :=

RUBY_BUILD_DIR := ../../build/drivers/ruby/
RUBY_PBDIR := $(RUBY_BUILD_DIR)/pb/
RUBY_PBFILE := $(RUBY_PBDIR)/ql2.pb.rb
RUBY_PBFILE_STRIPPED := $(RUBY_BUILD_DIR)/ql2.pb.rb
GEMSPEC := rethinkdb.gemspec
PROTOFILE := ql2.proto

TC_RPROTOC_EXE:=$(shell which ruby-protoc || true)

ifeq (,$(TC_RPROTOC_EXE))
  $(error Could not locate ruby-protoc. Have you installed the ruby-protocol-buffers gem?)
endif

all: driver-ruby

$(RUBY_PBDIR):
	mkdir -p $(RUBY_PBDIR)

driver-ruby: $(RUBY_PBFILE_STRIPPED)

$(RUBY_PBFILE_STRIPPED): $(RUBY_PBFILE)
	./strip-pbfile < $< > $@

$(RUBY_PBFILE): $(RUBY_PBDIR) $(PROTOFILE) $(RUBY_PBDIR)
	$(TC_RPROTOC_EXE) $(PROTOCFLAGS) -o $(RUBY_PBDIR) $(PROTOFILE)

GEMFILES = $(wildcard rethinkdb-*.gem)
publish: $(GEMSPEC) driver-ruby
	$(if $(GEMFILES), $(error Can not publish: a gem file already exists: $(GEMFILES)))
	gem build $(GEMSPEC)
	gem push rethinkdb-*.gem

clean:
	rm -f $(RUBY_PBFILE)
