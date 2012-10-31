RETHINKDB_HOME=../
EXTERNAL=$(RETHINKDB_HOME)external/
CLOSURE_LIB=$(EXTERNAL)google-closure-library/
CLOSURE_BUILDER=$(CLOSURE_LIB)closure/bin/build/closurebuilder.py
CLOSURE_COMPILER=$(EXTERNAL)google-closure-compiler/compiler.jar

PROTOC=protoc
PROTOC_JS_HOME_DIR=$(EXTERNAL)protobuf-plugin-closure/
PROTOC_JS_PLUGIN=$(PROTOC_JS_HOME_DIR)protoc-gen-js
PROTOC_JS_IMPORT_DIR=$(PROTOC_JS_HOME_DIR)js/
PROTOC_JS=$(PROTOC) --plugin=$(PROTOC_JS_PLUGIN) -I $(PROTOC_JS_IMPORT_DIR)

PROTO_FILE_DIR=../src/rdb_protocol/
PROTO_FILE=$(PROTO_FILE_DIR)query_language.proto

MAIN_BUILD_DIR=$(RETHINKDB_HOME)/build
DRIVERS_BUILD_DIR=$(MAIN_BUILD_DIR)/drivers
JS_BUILD_DIR=$(DRIVERS_BUILD_DIR)/javascript

all: test

$(JS_BUILD_DIR)/rethinkdb: $(JS_BUILD_DIR)
	$(QUIET) mkdir -p $(JS_BUILD_DIR);

$(PROTOC_JS_HOME_DIR)/protoc-gen-js:
	cd ../external/protobuf-plugin-closure ; $(MAKE) $(MFLAGS) ;

# Compile the javascript stubs for the rethinkdb protocol
$(JS_BUILD_DIR)/rethinkdb/query_language.pb.js: $(PROTO_FILE) $(PROTOC_JS_HOME_DIR)/protoc-gen-js
	$(PROTOC_JS) -I $(PROTO_FILE_DIR)  --js_out=$(JS_BUILD_DIR) $(PROTO_FILE)

js_server.js: js_server.coffee
	bash -c 'cat <(echo "goog.require(\"Query\");") \
				 <(echo "goog.provide(\"demo-server\");") \
				 <(coffee -p js_server.coffee)' > js_server.js

$(JS_BUILD_DIR)/demo-server.js: $(JS_BUILD_DIR)/rethinkdb/query_language.pb.js js_server.js
	$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=$(JS_BUILD_DIR) --root=. --namespace=demo-server --output_mode=script > $(JS_BUILD_DIR)/demo-server.js

test: $(JS_BUILD_DIR)/demo-server.js test.html
	rm -f demo-server.js
	cp $(JS_BUILD_DIR)/demo-server.js .
	node http_server.js 2300

clean:
	rm -f js_server.js
	rm -f demo-server.js
	rm -f $(JS_BUILD_DIR)/demo-server.js
