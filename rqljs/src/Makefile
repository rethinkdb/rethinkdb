RETHINKDB_HOME=../..
EXTERNAL=$(RETHINKDB_HOME)/external
CLOSURE_LIB=$(EXTERNAL)/google-closure-library
CLOSURE_BUILDER=$(CLOSURE_LIB)/closure/bin/build/closurebuilder.py
CLOSURE_COMPILER=$(EXTERNAL)/google-closure-compiler/compiler.jar

PROTOC=protoc
PROTOC_JS_HOME_DIR=$(EXTERNAL)/protobuf-plugin-closure
PROTOC_JS_PLUGIN=$(PROTOC_JS_HOME_DIR)/protoc-gen-js
PROTOC_JS_IMPORT_DIR=$(PROTOC_JS_HOME_DIR)/js
PROTOC_JS=$(PROTOC) --plugin=$(PROTOC_JS_PLUGIN) -I $(PROTOC_JS_IMPORT_DIR)

PROTO_FILE_DIR=$(RETHINKDB_HOME)/src/rdb_protocol
PROTO_BASE=ql2
PROTO_FILE=$(PROTO_FILE_DIR)/$(PROTO_BASE).proto
PB_JS_FILE=$(BUILD_DIR)/$(PROTO_BASE).pb.js

BUILD_DIR=../build
COFFEE_BUILD_DIR=$(BUILD_DIR)/coffee

COFFEE_FILES=$(wildcard *.coffee)
COMPILED_COFFEE=$(patsubst %.coffee,$(COFFEE_BUILD_DIR)/%.js,$(COFFEE_FILES))

OUTPUT_MODE=script

RQLJS=$(BUILD_DIR)/rqljs

all: test

$(PROTOC_JS_HOME_DIR)/protoc-gen-js:
	cd ../../external/protobuf-plugin-closure ; $(MAKE);

$(BUILD_DIR):
	mkdir -p $(COFFEE_BUILD_DIR)

$(PB_JS_FILE): $(PROTO_FILE) $(PROTOC_JS_HOME_DIR)/protoc-gen-js
	$(PROTOC_JS) -I $(PROTO_FILE_DIR) --js_out=$(BUILD_DIR) $(PROTO_FILE)

$(COFFEE_BUILD_DIR)/%.js: %.coffee
	coffee -b -p -c $< > $@

$(RQLJS): $(BUILD_DIR) $(PB_JS_FILE) $(COMPILED_COFFEE)
	$(CLOSURE_BUILDER) \
		--root=$(CLOSURE_LIB) \
		--root=$(BUILD_DIR) \
		--namespace=rethinkdb.RQLJS \
		--output_mode=$(OUTPUT_MODE) \
		--output_file=$@

test: $(RQLJS)
	node --debug $(RQLJS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean all test
