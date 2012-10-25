ifeq ($(VERBOSE),1)
QUIET:=
else
QUIET:=@
endif

SOURCE_DIR?=../src
PYTHON_PBDIR:=../build/drivers/python
PYTHON_PBFILE:=query_language_pb2.py
RUBY_PBDIR:=../build/drivers/ruby
RUBY_PBFILE:=query_language.pb.rb
BUILD_DRIVERS?=1
PROTOCFLAGS:= --proto_path=$(SOURCE_DIR)
all: drivers
drivers: driver-javascript driver-python driver-ruby

driver-python:
	$(QUIET) cd python ; $(MAKE) $(MFLAGS) ;

driver-ruby:
	$(QUIET) cd ruby ; $(MAKE) $(MFLAGS) ;

driver-javascript:
	$(QUIET) cd javascript ; $(MAKE) $(MFLAGS) ; cd .. ;

driver-javascript-clean:
	$(QUIET) cd javascript ; $(MAKE) $(MFLAGS) clean ; cd .. ;

driver-python-clean:
	$(QUIET) cd python ; $(MAKE) $(MFLAGS) clean ; cd .. ;

driver-ruby-clean:
	$(QUIET) cd ruby ; $(MAKE) $(MFLAGS) clean ; cd .. ;

clean: driver-javascript-clean driver-python-clean driver-ruby-clean

