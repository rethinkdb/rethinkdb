# Copyright 2010-2013 RethinkDB, all rights reserved.

EXTERNAL := $(TOP)/external

JS_SRC_DIR=$(TOP)/drivers/javascript/src
DRIVER_COFFEE_BUILD_DIR=$(JS_BUILD_DIR)/coffee

PROTO_FILE_DIR := $(TOP)/src/rdb_protocol
PROTO_BASE := ql2
PROTO_FILE := $(PROTO_FILE_DIR)/$(PROTO_BASE).proto
PB_BIN_FILE := $(JS_BUILD_DIR)/$(PROTO_BASE).desc

DRIVER_COFFEE_FILES := $(wildcard $(JS_SRC_DIR)/*.coffee)
DRIVER_COMPILED_COFFEE := $(patsubst $(JS_SRC_DIR)/%.coffee,$(DRIVER_COFFEE_BUILD_DIR)/%.js,$(DRIVER_COFFEE_FILES))

JS_PKG_DIR := $(PACKAGES_DIR)/js

JS_OUTPUT_MODE := script

JS_DRIVER_LIB=$(JS_BUILD_DIR)/rethinkdb.js

$(PB_BIN_FILE): $(PROTO_FILE) | $(JS_BUILD_DIR)/.
	$(PROTOC) -I $(PROTO_FILE_DIR) -o $(JS_BUILD_DIR)/ql2.desc $(PROTO_FILE)

.SECONDARY: $(DRIVER_COFFEE_BUILD_DIR)/.
$(DRIVER_COFFEE_BUILD_DIR)/%.js: $(JS_SRC_DIR)/%.coffee | $(DRIVER_COFFEE_BUILD_DIR)/. $(COFFEE)
	$P COFFEE
	$(COFFEE) -b -p -c $< > $@

$(JS_DRIVER_LIB): $(PB_BIN_FILE) $(DRIVER_COMPILED_COFFEE) | $(JS_BUILD_DIR)/.
	cp $(DRIVER_COFFEE_BUILD_DIR)/rethinkdb.js $(JS_BUILD_DIR)

.PHONY: js-dist
js-dist: $(JS_DRIVER_LIB) $(PB_BIN_FILE)
	$P DIST-JS $(JS_PKG_DIR)
	rm -rf $(JS_PKG_DIR)
	mkdir -p $(JS_PKG_DIR)
	cp $(JS_SRC_DIR)/package.json $(JS_PKG_DIR)
	cp $(JS_SRC_DIR)/README.md $(JS_PKG_DIR)
	cp $(JS_DRIVER_LIB) $(JS_PKG_DIR)
	cp $(PB_BIN_FILE) $(JS_PKG_DIR)

.PHONY: js-publish
js-publish: js-dist
	$P PUBLISH-JS $(JS_PKG_DIR)
	cd $(JS_PKG_DIR) && npm publish --force

.PHONY: test
test-js: $(JS_DRIVER_LIB)
	coffee test.coffee

.PHONY: testWeb
testWeb: $(JS_DRIVER_LIB)
	cd $(TOP); python -m SimpleHTTPServer

.PHONY: js-clean
js-clean:
	$P RM $(JS_BUILD_DIR)
	rm -rf $(JS_BUILD_DIR)

.PHONY: js-driver
js-driver: $(JS_DRIVER_LIB)

.PHONY: all
all: js-driver
