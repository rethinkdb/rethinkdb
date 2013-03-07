# Copyright 2010-2012 RethinkDB, all rights reserved.
# Makefile for various bits of the javascript driver

JS_DIR := $(TOP)/drivers/javascript

CLOSURE_LIB := $(EXTERNAL_DIR)/google-closure-library/
CLOSURE_BUILDER := $(CLOSURE_LIB)closure/bin/build/closurebuilder.py
CLOSURE_COMPILER := $(EXTERNAL_DIR)/google-closure-compiler/compiler.jar

PROTOC_JS_HOME_DIR := $(EXTERNAL_DIR)/protobuf-plugin-closure
PROTOC_JS_PLUGIN := $(PROTOC_JS_HOME_DIR)/protoc-gen-js
PROTOC_JS_IMPORT_DIR := $(PROTOC_JS_HOME_DIR)/js
PROTOC_JS := $(PROTOC_RUN) --plugin=$(PROTOC_JS_PLUGIN) -I $(PROTOC_JS_IMPORT_DIR)

PROTO_FILE_DIR := $(SOURCE_DIR)/rdb_protocol
PROTO_FILE := $(PROTO_FILE_DIR)/query_language.proto

DRIVERS_BUILD_DIR := $(BUILD_DIR)/drivers
JS_BUILD_DIR := $(BUILD_DIR)/javascript

JS_PACKAGE_DIR := $(PACKAGES_DIR)/javascript

ifeq ($(NO_COMPILE_JS),1)
  OUTPUTMODE := script
else
  OUTPUTMODE := compiled
endif

ALL += $(JS_DIR)
all-$(JS_DIR): js-driver

# Compile the rethinkdb library
.PHONY: js-driver
js-driver: $(JS_BUILD_DIR)/rethinkdb.js

$(JS_BUILD_DIR)/rethinkdb.js: $(wildcard $(JS_DIR)/rethinkdb/*) $(JS_BUILD_DIR)/rethinkdb/query_language.pb.js $(PROTOC_JS_PLUGIN)
	$P CP "$(JS_DIR)/rethinkdb/*.js" $(JS_BUILD_DIR)/rethinkdb/
	cp -pRP $(JS_DIR)/rethinkdb/*.js $(JS_BUILD_DIR)/rethinkdb/
	$P CLOSURE-BUILD $@
	( echo "(function(){" ; \
	  $(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=$(JS_BUILD_DIR)/rethinkdb/ --namespace=topLevel \
	    --compiler_jar=$(CLOSURE_COMPILER) \
	    --compiler_flags="--compilation_level=ADVANCED_OPTIMIZATIONS" \
	    --compiler_flags="--accept_const_keyword" \
	    --compiler_flags="--generate_exports" \
	    --compiler_flags="--externs=$(JS_DIR)/externs.js" \
	    --compiler_flags="--warning_level=VERBOSE" \
	    --compiler_flags="--create_source_map=$(JS_BUILD_DIR)/rethinkdb.js.map" \
	    --compiler_flags="--source_map_format=V3" \
	    --output_mode=$(OUTPUTMODE) && \
	  echo "})();" ; \
	) > $(JS_BUILD_DIR)/rethinkdb.js

js-publish: $(JS_BUILD_DIR)/rethinkdb.js $(JS_DIR)/package.json
	$P MKDIR $(JS_PACKAGE_DIR)
	mkdir -p $(JS_PACKAGE_DIR)
	$P CP $^ $(JS_PACKAGE_DIR)
	cp $^ $(JS_PACKAGE_DIR)
	$P NPM-PUBLISH rethinkdb
	cd $(JS_PACKAGE_DIR); npm publish --force

$(JS_BUILD_DIR)/rethinkdb:
	$P MKDIR $@
	mkdir -p $@

# Compile the javascript stubs for the rethinkdb protocol
$(JS_BUILD_DIR)/rethinkdb/query_language.pb.js: $(PROTO_FILE) $(PROTOC_JS_HOME_DIR)/protoc-gen-js | $(JS_BUILD_DIR)/rethinkdb
	$P PROTOC $<
	$(PROTOC_JS) -I $(PROTO_FILE_DIR) --js_out=$(JS_BUILD_DIR)/rethinkdb $(PROTO_FILE)

.PHONY: js-docs
js-docs: $(JS_BUILD_DIR)/rethinkdb.js
	$P JSDOC $(JS_DIR)/rethinkdb
	jsdoc --template=$(JS_DIR)/docs/_themes/jsdoc-for-sphinx -x=js,jsx --directory=$(JS_BUILD_DIR)/docs/jsdoc -E=query_language.pb.js -r=3 $(JS_DIR)/rethinkdb/
	$P MAKE -C $(JS_DIR)/docs/ html
	make -C $(JS_DIR)/docs/ html

CLEAN += $(JS_DIR)
clean-$(JS_DIR): js-clean

.PHOHNY: js-clean
js-clean: js-protodeps-clean
	$P RM $(JS_BUILD_DIR)
	rm -rf $(JS_BUILD_DIR)

.PHONY: js-protodeps
js-protodeps: $(PROTOC_JS_HOME_DIR)/protoc-gen-js

$(PROTOC_JS_HOME_DIR)/protoc-gen-js: $(PROTOC_DEP)
	$P MAKE -C $(EXTERNAL_DIR)/protobuf-plugin-closure
	$(EXTERN_MAKE) -C $(EXTERNAL_DIR)/protobuf-plugin-closure SPREFIX="$(abspath $(PROTOC_BASE))"

.PHONY: js-protodeps-clean
js-protodeps-clean:
	$P MAKE -C $(EXTERNAL_DIR)/protobuf-plugin-closure clean
	$(EXTERN_MAKE) -C $(EXTERNAL_DIR)/protobuf-plugin-closure SPREFIX="$(abspath $(PROTOC_BASE))" clean

