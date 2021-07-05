##### Installing

# TODO: uninstall

PRODUCT_NAME := RethinkDB

ifneq ($(PVERSION),)
  RETHINKDB_VERSION := $(PVERSION)
  RETHINKDB_CODE_VERSION ?= $(shell $(TOP)/scripts/gen-version.sh)
  PACKAGING_ALTERNATIVES_PRIORITY := 0
else
  RETHINKDB_VERSION := $(shell $(TOP)/scripts/gen-version.sh)
  RETHINKDB_CODE_VERSION ?= $(RETHINKDB_VERSION)
  PACKAGING_ALTERNATIVES_PRIORITY = $(shell expr $$($(TOP)/scripts/gen-version.sh -r) / 100)
endif

RETHINKDB_SHORT_VERSION := $(shell echo $(RETHINKDB_VERSION) | sed -e 's/\([^.]\+\.[^.]\+\).*$$/\1/')

