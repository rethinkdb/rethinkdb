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
PROTO_FILE=$(PROTO_FILE_DIR)/query_language.proto

MAIN_BUILD_DIR=$(RETHINKDB_HOME)/build
DRIVERS_BUILD_DIR=$(MAIN_BUILD_DIR)/drivers
JS_BUILD_DIR=$(DRIVERS_BUILD_DIR)/javascript

JS_DRIVER_DIR=$(RETHINKDB_HOME)/drivers/javascript

BUILD_DIR=../build
SRC_DIR=server
TEST_DIR=../test

COFFEE_FILES=$(wildcard $(SRC_DIR)/*.coffee)
COMPILED_COFFEE=$(patsubst $(SRC_DIR)/%.coffee,$(BUILD_DIR)/%.js,$(COFFEE_FILES))

all: test

$(PROTOC_JS_HOME_DIR)/protoc-gen-js:
	cd ../../external/protobuf-plugin-closure ; $(MAKE) $(MFLAGS) ;

$(JS_BUILD_DIR)/rethinkdb/query_language.pb.js: $(PROTO_FILE) $(PROTOC_JS_HOME_DIR)/protoc-gen-js
	cd $(JS_DRIVER_DIR); make pb

$(JS_BUILD_DIR)/rethinkdb.js:
	cd $(JS_DRIVER_DIR); make lib

$(BUILD_DIR)/%.js: $(SRC_DIR)/%.coffee
	mkdir -p $(BUILD_DIR)
	coffee -b -p -c $< > $@

$(JS_BUILD_DIR)/demo-server.js: $(JS_BUILD_DIR)/rethinkdb/query_language.pb.js $(COMPILED_COFFEE)
	$(CLOSURE_BUILDER) \
		--root=$(CLOSURE_LIB) \
		--root=$(JS_BUILD_DIR)/rethinkdb \
		--root=$(BUILD_DIR) \
		--namespace=rethinkdb.server.DemoServer \
		--output_mode=script \
		--output_file=$(JS_BUILD_DIR)/demo-server.js

test: $(JS_BUILD_DIR)/demo-server.js $(JS_BUILD_DIR)/rethinkdb.js test.html test.coffee
	mkdir -p $(TEST_DIR)
	coffee -p test.coffee > $(TEST_DIR)/test.js
	cp test.html $(TEST_DIR)
	cp style.css $(TEST_DIR)
	cp $(JS_BUILD_DIR)/rethinkdb.js $(TEST_DIR)
	cp underscore-min.js $(TEST_DIR)
	cp jquery-1.8.2.min.js $(TEST_DIR)
	cp $(JS_BUILD_DIR)/demo-server.js $(TEST_DIR)

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(TEST_DIR)
