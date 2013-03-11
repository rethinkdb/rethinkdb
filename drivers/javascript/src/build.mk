# Copyright 2010-2013 RethinkDB, all rights reserved.

EXTERNAL := $(TOP)/external
CLOSURE_LIB := $(EXTERNAL)/google-closure-library/
CLOSURE_BUILDER := $(CLOSURE_LIB)closure/bin/build/closurebuilder.py
CLOSURE_COMPILER := $(EXTERNAL)/google-closure-compiler/compiler.jar

PROTOC_JS_HOME_DIR := $(EXTERNAL)/protobuf-plugin-closure
PROTOC_JS_PLUGIN := $(PROTOC_JS_HOME_DIR)/protoc-gen-js
PROTOC_JS_IMPORT_DIR := $(PROTOC_JS_HOME_DIR)/js
PROTOC_JS := $(PROTOC) --plugin=$(PROTOC_JS_PLUGIN) -I $(PROTOC_JS_IMPORT_DIR)

JS_SRC_DIR=$(TOP)/drivers/javascript/src
DRIVER_COFFEE_BUILD_DIR=$(JS_BUILD_DIR)/coffee

PROTO_FILE_DIR := $(TOP)/src/rdb_protocol
PROTO_BASE := ql2
PROTO_FILE := $(PROTO_FILE_DIR)/$(PROTO_BASE).proto
PB_JS_FILE := $(JS_BUILD_DIR)/$(PROTO_BASE).pb.js

DRIVER_COFFEE_FILES := $(wildcard $(JS_SRC_DIR)/*.coffee)
DRIVER_COMPILED_COFFEE := $(patsubst $(JS_SRC_DIR)/%.coffee,$(DRIVER_COFFEE_BUILD_DIR)/%.js,$(DRIVER_COFFEE_FILES))

JS_PKG_DIR := $(PACKAGES_DIR)/js

JS_OUTPUT_MODE := script

JS_DRIVER_LIB=$(JS_BUILD_DIR)/rethinkdb.js

$(PROTOC_JS_HOME_DIR)/protoc-gen-js:
	$P MAKE -C $(TOP)/external/protobuf-plugin-closure
	$(EXTERN_MAKE) -C $(TOP)/external/protobuf-plugin-closure SPREFIX="$(abspath $(PROTOC_BASE))"

$(JS_BUILD_DIR):
	$P MKDIR $(DRIVER_COFFEE_BUILD_DIR)
	mkdir -p $(DRIVER_COFFEE_BUILD_DIR)

$(PB_JS_FILE): $(PROTO_FILE) $(PROTOC_JS_HOME_DIR)/protoc-gen-js
	$P PROTOC-JS
	$(PROTOC_JS) -I $(PROTO_FILE_DIR) --js_out=$(JS_BUILD_DIR) $(PROTO_FILE)

$(DRIVER_COFFEE_BUILD_DIR)/%.js: $(JS_SRC_DIR)/%.coffee
	$P COFFEE
	coffee -b -p -c $< > $@

$(JS_DRIVER_LIB): $(JS_BUILD_DIR) $(PB_JS_FILE) $(DRIVER_COMPILED_COFFEE)
	$P CLOSURE
	( if [[ script = "$(JS_OUTPUT_MODE)" ]]; then \
	    echo 'CLOSURE_NO_DEPS=true;' ; \
	  fi ; \
	  $(CLOSURE_BUILDER) \
	    --root=$(CLOSURE_LIB) \
	    --root=$(JS_BUILD_DIR) \
	    --namespace=rethinkdb.root \
	    --output_mode=$(JS_OUTPUT_MODE) \
	) > $@

.PHONY: publish
publish: $(JS_DRIVER_LIB)
	$P PUBLISH-JS
	mkdir -p $(JS_PKG_DIR)
	cp package.json $(JS_PKG_DIR)
	cp README.md $(JS_PKG_DIR)
	cp $(JS_DRIVER_LIB) $(JS_PKG_DIR)
	cd $(JS_PKG_DIR); npm publish --force

.PHONY: test
test: $(JS_DRIVER_LIB)
	coffee test.coffee

.PHONY: testWeb
testWeb: $(JS_DRIVER_LIB)
	cd $(TOP); python -m SimpleHTTPServer

.PHONY: clean
clean:
	rm -rf $(JS_BUILD_DIR)

.PHONY: js-driver
js-driver: $(JS_DRIVER_LIB)

.PHONY: all
all: js-driver
