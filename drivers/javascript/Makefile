# Makefile for various bits of the javascript driver

PROTOC=protoc
PROTOC_JS_HOME_DIR=/home/wmrowan/rethink/protobuf-plugin-closure/
PROTOC_JS_PLUGIN=$(PROTOC_JS_HOME_DIR)protoc-gen-js
PROTOC_JS_IMPORT_DIR=$(PROTOC_JS_HOME_DIR)js/
PROTOC_JS=$(PROTOC) --plugin=$(PROTOC_JS_PLUGIN) -I $(PROTOC_JS_IMPORT_DIR)

PROTO_FILE_DIR=../../src/rdb_protocol/
PROTO_FILE=$(PROTO_FILE_DIR)query_language.proto

CLOSURE_LIB=/home/wmrowan/rethink/google-closure-library/
CLOSURE_BUILDER=$(CLOSURE_LIB)closure/bin/build/closurebuilder.py
CLOSURE_COMPILER=/home/wmrowan/rethink/compiler.jar

JS_COMPILATION_MODE?=script

# Compile the javascript stubs for the rethinkdb protocol
proto: $(PROTO_FILE)
	$(PROTOC_JS) -I $(PROTO_FILE_DIR)  --js_out=. $(PROTO_FILE)

test: test.js
	rm -f test_compiled.js
	echo "(function(){this.goog = goog = {};" > /tmp/one
	$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=. --input=test.js \
		--compiler_jar=$(CLOSURE_COMPILER) \
		--compiler_flags="--externs externs.js --compilation_level=ADVANCED_OPTIMIZATIONS" \
		--output_mode=$(JS_COMPILATION_MODE) > /tmp/two
	echo "}).call(this.window ? window : global);" > /tmp/three
	cat /tmp/one /tmp/two /tmp/three > test_compiled.js

lib:
	$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=rethinkdb/ --namespace=rethinkdb \
		--compiler_jar=$(CLOSURE_COMPILER) \
		--compiler_flags="--compilation_level=ADVANCED_OPTIMIZATIONS" \
		--compiler_flags="--accept_const_keyword" \
		--compiler_flags="--generate_exports" \
		--compiler_flags="--externs=externs.js" \
		--output_mode=compiled > rethinkdb.js
