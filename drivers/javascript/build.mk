# Copyright 2010-2013 RethinkDB, all rights reserved.

JS_SRC_DIR=$(TOP)/drivers/javascript
DRIVER_COFFEE_BUILD_DIR=$(JS_BUILD_DIR)/coffee

PROTO_FILE_DIR := $(TOP)/src/rdb_protocol
PROTO_BASE := ql2
PROTO_FILE := $(PROTO_FILE_DIR)/$(PROTO_BASE).proto
PROTO_MODULE := $(JS_BUILD_DIR)/proto-def.js

DRIVER_COFFEE_FILES := $(wildcard $(JS_SRC_DIR)/*.coffee)
DRIVER_COMPILED_COFFEE := $(patsubst $(JS_SRC_DIR)/%.coffee,$(DRIVER_COFFEE_BUILD_DIR)/%.js,$(DRIVER_COFFEE_FILES))

JS_PKG_DIR := $(PACKAGES_DIR)/js

$(PROTO_MODULE): $(PROTO_FILE) | $(JS_BUILD_DIR)/.
	$P CONVERT_PROTOFILE
	$(JS_SRC_DIR)/../convert_protofile --language javascript --input-file $(PROTO_FILE) --output-file $(PROTO_MODULE)

# Must be synced with the list in package.json
JS_PKG_FILES := $(DRIVER_COMPILED_COFFEE) $(JS_SRC_DIR)/README.md $(PROTO_MODULE) $(JS_SRC_DIR)/package.json

.SECONDARY: $(DRIVER_COFFEE_BUILD_DIR)/.
$(DRIVER_COFFEE_BUILD_DIR)/%.js: $(JS_SRC_DIR)/%.coffee | $(DRIVER_COFFEE_BUILD_DIR)/. $(COFFEE_BIN_DEP)
	$P COFFEE
	$(COFFEE) -b -p -c $< > $@

.PHONY: js-dist
js-dist: $(JS_PKG_DIR) $(JS_PKG_DIR)/node_modules

$(JS_PKG_DIR): $(JS_PKG_FILES)
	$P CP $(JS_PKG_DIR)
	rm -rf $(JS_PKG_DIR)
	mkdir -p $(JS_PKG_DIR)
	cp $(JS_PKG_FILES) $(JS_PKG_DIR)

.PHONY: js-publish
js-publish: TMPFILE=$(shell mktemp)
js-publish: $(JS_PKG_DIR)
	$P PUBLISH-JS $(JS_PKG_DIR)
	cd $(JS_PKG_DIR) && npm publish

.PHONY: js-clean
js-clean:
	$P RM $(JS_BUILD_DIR)
	rm -rf $(JS_BUILD_DIR)
	$P RM $(JS_PKG_DIR)
	rm -rf $(JS_PKG_DIR)

.PHONY: js-install
js-install: NPM_PREFIX=.
js-install: $(JS_PKG_DIR) | $(NPM_BIN_DEP)
	$P NPM-INSTALL $(JS_PKG_DIR)
	MAKEFLAGS= $(NPM) install $(JS_PKG_DIR) --prefix $(NPM_PREFIX)

.PHONY: js-dependencies
js-dependencies: $(JS_PKG_DIR)/node_modules

BLUEBIRD_MODULE_DIR := $(SUPPORT_BUILD_DIR)/bluebird_$(bluebird_VERSION)/node_modules/packed-bluebird/node_modules/bluebird

$(BLUEBIRD_MODULE_DIR): $(SUPPORT_BUILD_DIR)/bluebird_$(bluebird_VERSION)/$(INSTALL_WITNESS)
	touch $@

$(JS_PKG_DIR)/node_modules: $(BLUEBIRD_MODULE_DIR) $(JS_PKG_DIR) | $(NPM_BIN_DEP)
	$P CP $@/bluebird
	mkdir -p $@/bluebird
	cp -a $(BLUEBIRD_MODULE_DIR)/. $@/bluebird
	touch $@

$(JS_BUILD_DIR)/rethinkdb.js: $(JS_PKG_DIR) $(JS_PKG_DIR)/node_modules | $(BROWSERIFY_BIN_DEP)
	$P BROWSERIFY
ifeq (Windows, $(OS))
	cd $(JS_PKG_DIR) && \
	  $(abspath $(BROWSERIFY)) --require ./rethinkdb:rethinkdb --ignore tls --outfile "$(shell cygpath -w "$(abspath $@)")"
else
	cd $(JS_PKG_DIR) && \
	  $(abspath $(BROWSERIFY)) --require ./rethinkdb:rethinkdb --ignore tls --outfile $(abspath $@)
endif

.PHONY: js-driver
js-driver: $(JS_BUILD_DIR)/rethinkdb.js
