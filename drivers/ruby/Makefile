# Copyright 2010-2014 RethinkDB, all rights reserved.

RUBY_BUILD_DIR := ../../build/drivers/ruby
RUBY_PBFILE_STRIPPED := $(RUBY_BUILD_DIR)/ql2.pb.rb
GEMSPEC := rethinkdb.gemspec
PROTOFILE := ql2.proto

all: driver-ruby

driver-ruby: $(RUBY_PBFILE_STRIPPED)

$(RUBY_BUILD_DIR):
	mkdir -p $(RUBY_BUILD_DIR)

$(RUBY_PBFILE_STRIPPED): $(PROTOFILE) $(RUBY_BUILD_DIR)
	./strip-protofile < $< > $@ 

GEMFILES = $(wildcard rethinkdb-*.gem)
publish: $(GEMSPEC) driver-ruby
	$(if $(GEMFILES), $(error Can not publish: a gem file already exists: $(GEMFILES)))
	gem build $(GEMSPEC)
	gem push rethinkdb-*.gem

clean:
	rm -f $(RUBY_PBFILE_STRIPPED)
