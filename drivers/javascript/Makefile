# Makefile for various bits of the javascript driver

RETHINKDB_HOME=../../
EXTERNAL=$(RETHINKDB_HOME)external/
CLOSURE_LIB=$(EXTERNAL)google-closure-library/
CLOSURE_BUILDER=$(CLOSURE_LIB)closure/bin/build/closurebuilder.py
CLOSURE_COMPILER=$(EXTERNAL)google-closure-compiler/compiler.jar

PROTOC=protoc
PROTOC_JS_HOME_DIR=$(EXTERNAL)protobuf-plugin-closure/
PROTOC_JS_PLUGIN=$(PROTOC_JS_HOME_DIR)protoc-gen-js
PROTOC_JS_IMPORT_DIR=$(PROTOC_JS_HOME_DIR)js/
PROTOC_JS=$(PROTOC) --plugin=$(PROTOC_JS_PLUGIN) -I $(PROTOC_JS_IMPORT_DIR)

PROTO_FILE_DIR=../../src/rdb_protocol/
PROTO_FILE=$(PROTO_FILE_DIR)query_language.proto

OUTPUTMODE=compiled

JSDOC=/usr/share/jsdoc-toolkit/jsrun.jar

# Compile the rethinkdb library
lib: query_language.pb.js
	rm -rf rethinkdb.js
	rm -rf rethinkdb.map.js
	bash -c 'cat <(echo "(function(){") \
		<($(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=rethinkdb/ --namespace=topLevel \
		--compiler_jar=$(CLOSURE_COMPILER) \
		--compiler_flags="--compilation_level=ADVANCED_OPTIMIZATIONS" \
		--compiler_flags="--accept_const_keyword" \
		--compiler_flags="--generate_exports" \
		--compiler_flags="--externs=externs.js" \
		--compiler_flags="--warning_level=VERBOSE" \
		--compiler_flags="--create_source_map=./rethinkdb.js.map" \
		--compiler_flags="--source_map_format=V3" \
		--output_mode=$(OUTPUTMODE)) \
		<(echo "}).call(this);") > rethinkdb.js'

# Compile the javascript stubs for the rethinkdb protocol
query_language.pb.js: $(PROTO_FILE)
	$(PROTOC_JS) -I $(PROTO_FILE_DIR)  --js_out=rethinkdb $(PROTO_FILE)

docs: rethinkdb.js
	jsdoc --template=docs/_themes/jsdoc-for-sphinx -x=js,jsx --directory=./docs/jsdoc -E=query_language.pb.js -r=3 rethinkdb/
	make -C docs/ html
	cp -r docs/_build/* /var/www/jsdocs/

clean:
	rm -rf rethinkdb.js
	rm -rf rethinkdb.js.map
	rm -rf rethinkdb/query_language.pb.js

protodeps:
	$(QUIET) cd ../../external/protobuf-plugin-closure ; $(MAKE) $(MFLAGS) ;

