##### Installing

# TODO: uninstall

PRODUCT_NAME := RethinkDB

ifneq ($(PVERSION),)
  RETHINKDB_VERSION := $(PVERSION)
  RETHINKDB_CODE_VERSION ?= $(shell $(TOP)/scripts/gen-version.sh)
  RETHINKDB_SHORT_VERSION := $(shell echo $(RETHINKDB_VERSION) | sed 's/\([^.]\+\.[^.]\+\).*$$//\1/')
  PACKAGING_ALTERNATIVES_PRIORITY := 0
else
  RETHINKDB_FALLBACK_VERSION := $(shell if [ -e $(TOP)/NOTES ] ; then cat $(TOP)/NOTES | grep '^. Release' | head -n 1 | awk '{ printf "%s" , $$3 ; }' ; fi ; )
  RETHINKDB_VERSION := $(shell env FALLBACK_VERSION=$(RETHINKDB_FALLBACK_VERSION) $(TOP)/scripts/gen-version.sh)
  RETHINKDB_CODE_VERSION ?= $(shell $(TOP)/scripts/gen-version.sh)
  RETHINKDB_SHORT_VERSION := $(shell echo $(RETHINKDB_VERSION) | sed 's/\([^.]\+\.[^.]\+\).*$$/\1/')
  PACKAGING_ALTERNATIVES_PRIORITY = $(shell expr $$($(TOP)/scripts/gen-version.sh -r) / 100)
endif

ifeq ($(NAMEVERSIONED),1)
  SERVER_EXEC_NAME_VERSIONED := $(SERVER_EXEC_NAME)-$(RETHINKDB_SHORT_VERSION)
else
  SERVER_EXEC_NAME_VERSIONED := $(SERVER_EXEC_NAME)
endif

RETHINKDB_PACKAGING_VERSION := $(RETHINKDB_VERSION)

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
web_res_dir := $(share_dir)/web
pidfile_dir := $(var_dir)/run/rethinkdb
data_dir := $(var_dir)/lib/rethinkdb
language_drivers_dir := $(share_dir)/drivers

FULL_SERVER_EXEC_NAME := $(bin_dir)/$(SERVER_EXEC_NAME)
FULL_SERVER_EXEC_NAME_VERSIONED := $(bin_dir)/$(SERVER_EXEC_NAME_VERSIONED)

ASSETS_DIR:=$(PACKAGING_DIR)/assets
ASSET_SCRIPTS:=$(ASSETS_DIR)/scripts/rdb_migrate
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
	install -m755 -d $(DESTDIR)$(bin_dir)
	install -m755 $(BUILD_DIR)/$(SERVER_EXEC_NAME) $(DESTDIR)$(FULL_SERVER_EXEC_NAME_VERSIONED)
ifeq ($(STRIP_ON_INSTALL),1)
	$P STRIP $(DESTDIR)$(FULL_SERVER_EXEC_NAME_VERSIONED)
	$(STRIP_UNNEEDED) $(DESTDIR)$(FULL_SERVER_EXEC_NAME_VERSIONED)
endif

.PHONY: install-manpages
install-manpages: $(ASSETS_DIR)/man/rethinkdb.1
	$P INSTALL $^ $(DESTDIR)$(man1_dir)
	install -m755 -d $(DESTDIR)$(man1_dir)
	m4 -D "SHORT_VERSION=$(RETHINKDB_SHORT_VERSION)" \
	   -D "CURRENT_DATE=$(shell date +%F)" \
	   < $(ASSETS_DIR)/man/rethinkdb.1 | gzip -9 | \
	   install -m644 /dev/stdin $(DESTDIR)$(man1_dir)/$(VERSIONED_PACKAGE_NAME).1.gz

.PHONY: install-tools
install-tools: $(ASSETS_DIR)/scripts/rethinkdb.bash $(ASSET_SCRIPTS)
	$P INSTALL $< $(DESTDIR)$(internal_bash_completion_dir) \
	                 $(DESTDIR)$(bash_completion_dir)
	install -m755 -d $(DESTDIR)$(internal_bash_completion_dir)
	install -m755 -d $(DESTDIR)$(bash_completion_dir)
	m4 -D "SERVER_EXEC_NAME=$(SERVER_EXEC_NAME)" \
	   -D "SERVER_EXEC_NAME_VERSIONED=$(SERVER_EXEC_NAME_VERSIONED)" \
	   $(ASSETS_DIR)/scripts/rethinkdb.bash | \
	  install -m644 /dev/stdin $(DESTDIR)$(internal_bash_completion_dir)/$(SERVER_EXEC_NAME).bash
	install -m644 $(DESTDIR)$(internal_bash_completion_dir)/$(SERVER_EXEC_NAME).bash \
	              $(DESTDIR)$(bash_completion_dir)/$(SERVER_EXEC_NAME).bash
	$P INSTALL $(ASSET_SCRIPTS) $(DESTDIR)$(scripts_dir)
	install -m755 -d $(DESTDIR)$(scripts_dir)
	for s in $(ASSET_SCRIPTS); do install -m755 "$$s" $(DESTDIR)$(scripts_dir)/$$(basename $$s); done
	$P INSTALL $(INIT_SCRIPTS) $(DESTDIR)$(init_dir)
	install -m755 -d $(DESTDIR)$(init_dir)
	for s in $(INIT_SCRIPTS); do install -m755 "$$s" $(DESTDIR)$(init_dir)/$$(basename $$s); done

.PHONY: install-config
install-config:
	$P INSTALL $(DESTDIR)$(conf_dir)/default.conf.sample
	install -m755 -d $(DESTDIR)$(conf_dir)
	install -m755 -d $(DESTDIR)$(conf_instance_dir)
	install -m644 $(ASSETS_DIR)/config/default.conf.sample $(DESTDIR)$(conf_dir)/default.conf.sample

.PHONY: install-data
install-data:
	$P INSTALL $(DESTDIR)$(data_dir)/instances.d
	install -m755 -d $(DESTDIR)$(data_dir)
	install -m755 -d $(DESTDIR)$(data_dir)/instances.d

.PHONY: install-web
install-web: web-assets
	$P INSTALL $(DESTDIR)$(web_res_dir)
	install -m755 -d $(DESTDIR)$(web_res_dir)
# This might break some ownership or permissions stuff.
	cp -pRP $(WEB_ASSETS_BUILD_DIR)/* $(DESTDIR)$(web_res_dir)/

.PHONY: install-docs
install-docs:
	$P INSTALL $(ASSETS_DIR)/docs/LICENSE $(DESTDIR)$(doc_dir)/copyright
	install -m755 -d $(DESTDIR)$(doc_dir)
	install -m644 $(ASSETS_DIR)/docs/LICENSE $(DESTDIR)$(doc_dir)/copyright

.PHONY: install
install: install-binaries install-manpages install-docs install-tools install-web install-data install-config
