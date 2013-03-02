# build configuration for the osx package

BUILD_PORTABLE ?= 1
STATIC ?= 1

.PHONY: $(TOP)/all
$(TOP)/all: build-osx