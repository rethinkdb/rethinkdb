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

ifeq ($(NAMEVERSIONED),1)
  SERVER_EXEC_NAME_VERSIONED := $(SERVER_EXEC_NAME)-$(RETHINKDB_SHORT_VERSION)
else
  SERVER_EXEC_NAME_VERSIONED := $(SERVER_EXEC_NAME)
endif

ifeq ($(NAMEVERSIONED),1)
  VERSIONED_QUALIFIED_PACKAGE_NAME := $(PACKAGE_NAME)-$(RETHINKDB_SHORT_VERSION)
  VERSIONED_PACKAGE_NAME := $(PACKAGE_NAME)-$(RETHINKDB_SHORT_VERSION)
  VERSIONED_PRODUCT_SHARE_DIR := /usr/share/$(VERSIONED_PACKAGE_NAME)
else
  VERSIONED_QUALIFIED_PACKAGE_NAME := $(PACKAGE_NAME)
  VERSIONED_PACKAGE_NAME := $(PACKAGE_NAME)
  VERSIONED_PRODUCT_SHARE_DIR := /usr/share/$(VERSIONED_PACKAGE_NAME)
endif

prefix ?= $(PREFIX)
etc_dir ?= $(SYSCONFDIR)
var_dir ?= $(LOCALSTATEDIR)
bin_dir := $(prefix)/bin
doc_dir := $(prefix)/share/doc/$(VERSIONED_PACKAGE_NAME)
man_dir := $(prefix)/share/man
man1_dir := $(man_dir)/man1
share_dir := $(prefix)/share/$(VERSIONED_PACKAGE_NAME)
bash_completion_dir := $(etc_dir)/bash_completion.d
internal_bash_completion_dir := $(share_dir)/etc/bash_completion.d
scripts_dir := $(share_dir)/scripts
init_dir := $(etc_dir)/init.d
conf_dir := $(etc_dir)/rethinkdb
conf_instance_dir := $(conf_dir)/instances.d
lib_dir := $(prefix)/lib/rethinkdb
pidfile_dir := $(var_dir)/run/rethinkdb
data_dir := $(var_dir)/lib/rethinkdb
language_drivers_dir := $(share_dir)/drivers

FULL_SERVER_EXEC_NAME := $(bin_dir)/$(SERVER_EXEC_NAME)
FULL_SERVER_EXEC_NAME_VERSIONED := $(bin_dir)/$(SERVER_EXEC_NAME_VERSIONED)
ASSETS_DIR:=$(PACKAGING_DIR)/assets
INIT_SCRIPTS:=$(ASSETS_DIR)/init/rethinkdb

##### Install

ifeq ($(OS),Darwin)
  STRIP_UNNEEDED := strip -u -r
else
  STRIP_UNNEEDED := strip --strip-unneeded
endif

.PHONY: install-binaries
install-binaries: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$P INSTALL $^ $(DESTDIR)$(bin_dir)
	umask 022 && install -m755 -d $(DESTDIR)$(bin_dir)
	install -m755 $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(DESTDIR)$(FULL_SERVER_EXEC_NAME_VERSIONED)
ifeq ($(STRIP_ON_INSTALL),1)
	$P STRIP $(DESTDIR)$(FULL_SERVER_EXEC_NAME_VERSIONED)
	$(STRIP_UNNEEDED) $(DESTDIR)$(FULL_SERVER_EXEC_NAME_VERSIONED)
endif

$(BUILD_DIR)/assets/rethinkdb.1: $(ASSETS_DIR)/man/rethinkdb.1 | $(BUILD_DIR)/assets/.
	$P M4
	m4 -D "SHORT_VERSION=$(RETHINKDB_SHORT_VERSION)" \
	   -D "CURRENT_DATE=$(shell date +%F)" \
	   < $< > $@

%.gz: %
	$P GZIP
	gzip -9 < $< > $@

.PHONY: install-manpages
install-manpages: $(BUILD_DIR)/assets/rethinkdb.1.gz
	$P INSTALL $^ $(DESTDIR)$(man1_dir)
	umask 022 && install -m755 -d $(DESTDIR)$(man1_dir)
	install -m644 $< $(DESTDIR)$(man1_dir)/$(VERSIONED_PACKAGE_NAME).1.gz

$(BUILD_DIR)/assets/rethinkdb.bash: $(ASSETS_DIR)/scripts/rethinkdb.bash | $(BUILD_DIR)/assets/.
	m4 -D "SERVER_EXEC_NAME=$(SERVER_EXEC_NAME)" \
	   -D "SERVER_EXEC_NAME_VERSIONED=$(SERVER_EXEC_NAME_VERSIONED)" \
	   $< > $@

.PHONY: install-tools
install-tools: $(BUILD_DIR)/assets/rethinkdb.bash
	$P INSTALL $< $(DESTDIR)$(internal_bash_completion_dir) $(DESTDIR)$(bash_completion_dir)
	umask 022 && install -m755 -d $(DESTDIR)$(internal_bash_completion_dir)
	umask 022 && install -m755 -d $(DESTDIR)$(bash_completion_dir)
	install -m644 $(BUILD_DIR)/assets/rethinkdb.bash \
	   $(DESTDIR)$(internal_bash_completion_dir)/$(SERVER_EXEC_NAME).bash
	install -m644 $(BUILD_DIR)/assets/rethinkdb.bash \
           $(DESTDIR)$(bash_completion_dir)/$(SERVER_EXEC_NAME).bash
	$P INSTALL $(INIT_SCRIPTS) $(DESTDIR)$(init_dir)
	umask 022 && install -m755 -d $(DESTDIR)$(init_dir)
	for s in $(INIT_SCRIPTS); do install -m755 "$$s" $(DESTDIR)$(init_dir)/$$(basename $$s); done

.PHONY: install-config
install-config:
	$P INSTALL $(DESTDIR)$(conf_dir)/default.conf.sample
	umask 022 && install -m755 -d $(DESTDIR)$(conf_dir)
	umask 022 && install -m755 -d $(DESTDIR)$(conf_instance_dir)
	install -m644 $(ASSETS_DIR)/config/default.conf.sample $(DESTDIR)$(conf_dir)/default.conf.sample

.PHONY: install-data
install-data:
	$P INSTALL $(DESTDIR)$(data_dir)/instances.d
	umask 022 && install -m755 -d $(DESTDIR)$(data_dir)/instances.d

.PHONY: install-docs
install-docs:
	$P INSTALL $(ASSETS_DIR)/docs/LICENSE $(DESTDIR)$(doc_dir)/copyright
	umask 022 && install -m755 -d $(DESTDIR)$(doc_dir)
	install -m644 $(ASSETS_DIR)/docs/LICENSE $(DESTDIR)$(doc_dir)/copyright

.PHONY: install
install: install-binaries install-manpages install-docs install-tools install-data install-config
