# RethinkDB configuration for building a deb package

STRIP_ON_INSTALL ?= 1
CONFIGURE_FLAGS ?= --disable-drivers

DEFAULT_GOAL := build-deb