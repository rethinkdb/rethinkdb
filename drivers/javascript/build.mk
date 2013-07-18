# Copyright 2010-2013 RethinkDB, all rights reserved.

EXTERNAL := $(TOP)/external

JS_SRC_DIR=$(TOP)/drivers/javascript
DRIVER_COFFEE_BUILD_DIR=$(JS_BUILD_DIR)/coffee

PROTO_FILE_DIR := $(TOP)/src/rdb_protocol
PROTO_BASE := ql2
PROTO_FILE := $(PROTO_FILE_DIR)/$(PROTO_BASE).proto
PB_BIN_FILE := $(JS_BUILD_DIR)/$(PROTO_BASE).desc

DRIVER_COFFEE_FILES := $(wildcard $(JS_SRC_DIR)/*.coffee)
DRIVER_COMPILED_COFFEE := $(patsubst $(JS_SRC_DIR)/%.coffee,$(DRIVER_COFFEE_BUILD_DIR)/%.js,$(DRIVER_COFFEE_FILES))

JS_PKG_DIR := $(PACKAGES_DIR)/js

$(PB_BIN_FILE): $(PROTO_FILE) | $(JS_BUILD_DIR)/.
	$(PROTOC) -I $(PROTO_FILE_DIR) -o $(JS_BUILD_DIR)/ql2.desc $(PROTO_FILE)

# Must be synced with the list in package.json
JS_PKG_FILES := $(DRIVER_COMPILED_COFFEE) $(JS_SRC_DIR)/README.md $(PB_BIN_FILE) $(JS_SRC_DIR)/package.json

.SECONDARY: $(DRIVER_COFFEE_BUILD_DIR)/.
$(DRIVER_COFFEE_BUILD_DIR)/%.js: $(JS_SRC_DIR)/%.coffee | $(DRIVER_COFFEE_BUILD_DIR)/. $(COFFEE)
	$P COFFEE
	$(COFFEE) -b -p -c $< > $@

.PHONY: js-dist
js-dist: $(JS_PKG_FILES)
	$P DIST-JS $(JS_PKG_DIR)
	rm -rf $(JS_PKG_DIR)
	mkdir -p $(JS_PKG_DIR)
	cp $(JS_PKG_FILES) $(JS_PKG_DIR)

.PHONY: js-publish
js-publish: js-dist
	$P PUBLISH-JS $(JS_PKG_DIR)
	cd $(JS_PKG_DIR) && npm publish --force

.PHONY: js-clean
js-clean:
	$P RM $(JS_BUILD_DIR)
	rm -rf $(JS_BUILD_DIR)

.PHONY: js-driver
js-driver: js-dist
	$P NPM INSTALL $(JS_PKG_DIR)
	#TODO This currently won't run without erroring because it recursively calls make
	# which results in a make error
	#npm install $(JS_PKG_DIR)

$(JS_BUILD_DIR)/rethinkdb.js: $(JS_BUILD_DIR)/.
	echo "throw new Error('Web version of JS driver not available');" > $(JS_BUILD_DIR)/rethinkdb.js

.PHONY: all
all: js-driver
