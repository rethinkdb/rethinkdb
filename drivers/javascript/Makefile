# Makefile for various bits of the javascript driver

CLOSURE_LIB=/home/wmrowan/rethink/google-closure-library/
CLOSURE_BUILDER=$(CLOSURE_LIB)closure/bin/build/closurebuilder.py
CLOSURE_COMPILER=/home/wmrowan/rethink/compiler.jar

PROTOC=protoc
PROTOC_JS_HOME_DIR=/home/wmrowan/rethink/protobuf-plugin-closure/
PROTOC_JS_PLUGIN=$(PROTOC_JS_HOME_DIR)protoc-gen-js
PROTOC_JS_IMPORT_DIR=$(PROTOC_JS_HOME_DIR)js/
PROTOC_JS=$(PROTOC) --plugin=$(PROTOC_JS_PLUGIN) -I $(PROTOC_JS_IMPORT_DIR)

PROTO_FILE_DIR=../../src/rdb_protocol/
PROTO_FILE=$(PROTO_FILE_DIR)query_language.proto

# Compile the 
lib: query_language.pb.js
	rm -rf rethinkdb.js
	rm -rf rethinkdb.map.js
	$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=rethinkdb/ --namespace=rethinkdb \
		--compiler_jar=$(CLOSURE_COMPILER) \
		--compiler_flags="--compilation_level=ADVANCED_OPTIMIZATIONS" \
		--compiler_flags="--accept_const_keyword" \
		--compiler_flags="--generate_exports" \
		--compiler_flags="--externs=externs.js" \
		--compiler_flags="--warning_level=VERBOSE" \
		--compiler_flags="--create_source_map=./%outname%.map" \
		--compiler_flags="--source_map_format=V3" \
		--compiler_flags="--js_output_file=rethinkdb.js" \
		--output_mode=compiled

# Compile the javascript stubs for the rethinkdb protocol
query_language.pb.js: $(PROTO_FILE)
	$(PROTOC_JS) -I $(PROTO_FILE_DIR)  --js_out=rethinkdb $(PROTO_FILE)

clean:
	rm -rf rethinkdb.js
	rm -rf rethinkdb.js.map
	rm -rf rethinkdb/query_language.pb.js
