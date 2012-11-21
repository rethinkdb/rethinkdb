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

DEMO_SERVER_DIR=../build
TEMP_DIR=../temp
SERVER_SRC_DIR=server

all: test

$(PROTOC_JS_HOME_DIR)/protoc-gen-js:
	cd ../../external/protobuf-plugin-closure ; $(MAKE) $(MFLAGS) ;

$(JS_BUILD_DIR)/rethinkdb/query_language.pb.js: $(PROTO_FILE) $(PROTOC_JS_HOME_DIR)/protoc-gen-js
	cd $(JS_DRIVER_DIR); make pb

$(JS_BUILD_DIR)/rethinkdb.js:
	cd $(JS_DRIVER_DIR); make lib

$(TEMP_DIR)/js_server.js: $(SERVER_SRC_DIR)/*
	mkdir -p $(TEMP_DIR)
	cat $(SERVER_SRC_DIR)/* >> $(TEMP_DIR)/demo_server.coffee
	bash -c 'cat <(echo "goog.require(\"Query\");") \
				 <(echo "goog.require(\"goog.proto2.WireFormatSerializer\");") \
				 <(echo "goog.provide(\"demo_server\");") \
				 <(cat sha1.js) > $(TEMP_DIR)/js_server.js \
				 <(coffee -p $(TEMP_DIR)/demo_server.coffee)' > $(TEMP_DIR)/js_server.js

$(JS_BUILD_DIR)/demo-server.js: $(JS_BUILD_DIR)/rethinkdb/query_language.pb.js $(TEMP_DIR)/js_server.js
	$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=$(JS_BUILD_DIR)/rethinkdb --root=$(TEMP_DIR) --namespace=demo_server --output_mode=script --output_file=$(JS_BUILD_DIR)/demo-server.js

test: $(JS_BUILD_DIR)/demo-server.js $(JS_BUILD_DIR)/rethinkdb.js test.html
	mkdir -p $(DEMO_SERVER_DIR)
	coffee -p test.coffee > $(DEMO_SERVER_DIR)/test.js
	cp test.html $(DEMO_SERVER_DIR)
	cp style.css $(DEMO_SERVER_DIR)
	cp $(JS_BUILD_DIR)/rethinkdb.js $(DEMO_SERVER_DIR)
	cp underscore-min.js $(DEMO_SERVER_DIR)
	cp http_server.js $(DEMO_SERVER_DIR)
	cp jquery-1.8.2.min.js $(DEMO_SERVER_DIR)
	rm -f $(DEMO_SERVER_DIR)/demo-server.js
	cp $(JS_BUILD_DIR)/demo-server.js $(DEMO_SERVER_DIR)

clean:
	rm -rf $(DEMO_SERVER_DIR)/*
	rm -rf $(TEMP_DIR)
