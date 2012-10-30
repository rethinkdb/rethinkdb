# Copyright 2010-2012 RethinkDB, all rights reserved.
# Makefile for various bits of the javascript driver

VERBOSE?=0

ifeq ($(VERBOSE),1)
QUIET:=
else
QUIET:=@
endif

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

MAIN_BUILD_DIR=$(RETHINKDB_HOME)/build
DRIVERS_BUILD_DIR=$(MAIN_BUILD_DIR)/drivers
JS_BUILD_DIR=$(DRIVERS_BUILD_DIR)/javascript

ifeq ($(NO_COMPILE),1)
OUTPUTMODE=script
else
OUTPUTMODE=compiled
endif

JSDOC=/usr/share/jsdoc-toolkit/jsrun.jar

all: lib



# Compile the rethinkdb library
lib: $(JS_BUILD_DIR)/rethinkdb.js

# SILENCER_1=2> /dev/null || ( echo "      Build failure." ; false ; )
SILENCER_1=

$(JS_BUILD_DIR)/rethinkdb.js: $(JS_BUILD_DIR)/rethinkdb/query_language.pb.js $(PROTOC_JS_HOME_DIR)/protoc-gen-js
ifeq ($(VERBOSE),0)
	@echo "    CAT > $(JS_BUILD_DIR)/rethinkdb.js"
endif
	$(QUIET) rm -rf $(JS_BUILD_DIR)/rethinkdb.js
	$(QUIET) rm -rf $(JS_BUILD_DIR)/rethinkdb.map.js
	$(QUIET) cp -pRP rethinkdb/*.js $(JS_BUILD_DIR)/rethinkdb/ ;
	$(QUIET) bash -c 'cat <(echo "(function(){") \
		<($(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=$(JS_BUILD_DIR)/rethinkdb/ --namespace=topLevel \
		--compiler_jar=$(CLOSURE_COMPILER) \
		--compiler_flags="--compilation_level=ADVANCED_OPTIMIZATIONS" \
		--compiler_flags="--accept_const_keyword" \
		--compiler_flags="--generate_exports" \
		--compiler_flags="--externs=externs.js" \
		--compiler_flags="--warning_level=VERBOSE" \
		--compiler_flags="--create_source_map=$(JS_BUILD_DIR)/rethinkdb.js.map" \
		--compiler_flags="--source_map_format=V3" \
		--output_mode=$(OUTPUTMODE)) \
		<(echo "})();") > $(JS_BUILD_DIR)/rethinkdb.js' $(SILENCER_1) || ( echo "      Build failure." ; rm $(JS_BUILD_DIR)/rethinkdb.js ; false ; )

$(JS_BUILD_DIR)/rethinkdb: $(JS_BUILD_DIR)
	$(QUIET) mkdir -p $(JS_BUILD_DIR)/rethinkdb ;

$(JS_BUILD_DIR): $(DRIVERS_BUILD_DIR)
	$(QUIET) mkdir -p  $(JS_BUILD_DIR) ;

$(DRIVERS_BUILD_DIR): $(MAIN_BUILD_DIR)
	$(QUIET) mkdir -p  $(DRIVERS_BUILD_DIR) ;

$(MAIN_BUILD_DIR):
	$(QUIET) mkdir -p $(MAIN_BUILD_DIR) ;

# Compile the javascript stubs for the rethinkdb protocol
$(JS_BUILD_DIR)/rethinkdb/query_language.pb.js: $(PROTO_FILE) $(PROTOC_JS_HOME_DIR)/protoc-gen-js $(JS_BUILD_DIR)/rethinkdb
ifeq ($(VERBOSE),0)
	@echo "    PROTOC $@"
endif
	$(QUIET) $(PROTOC_JS) -I $(PROTO_FILE_DIR)  --js_out=$(JS_BUILD_DIR)/rethinkdb $(PROTO_FILE)

docs: $(JS_BUILD_DIR)/rethinkdb.js
	$(QUIET) jsdoc --template=docs/_themes/jsdoc-for-sphinx -x=js,jsx --directory=$(JS_BUILD_DIR)/docs/jsdoc -E=query_language.pb.js -r=3 rethinkdb/
	$(QUIET) make -C docs/ html
	$(QUIET) cp -r docs/_build/* /var/www/jsdocs/

clean: protodeps-clean
ifeq ($(VERBOSE),0)
	@echo "    RM -rf $(JS_BUILD_DIR)/rethinkdb.js"
endif
	$(QUIET) if [ -e $(JS_BUILD_DIR)/rethinkdb.js ] ; then rm $(JS_BUILD_DIR)/rethinkdb.js ; fi ;
ifeq ($(VERBOSE),0)
	@echo "    RM -rf $(JS_BUILD_DIR)/rethinkdb.js.map"
endif
	$(QUIET) if [ -e $(JS_BUILD_DIR)/rethinkdb.js.map ] ; then rm $(JS_BUILD_DIR)/rethinkdb.js.map ; fi ;
ifeq ($(VERBOSE),0)
	@echo "    RM -rf $(JS_BUILD_DIR)/rethinkdb/query_language.pb.js"
endif
	$(QUIET) if [ -e $(JS_BUILD_DIR)/rethinkdb/query_language.pb.js ] ; then rm $(JS_BUILD_DIR)/rethinkdb/query_language.pb.js ; fi ;

protodeps: $(PROTOC_JS_HOME_DIR)/protoc-gen-js

$(PROTOC_JS_HOME_DIR)/protoc-gen-js:
	$(QUIET) cd ../../external/protobuf-plugin-closure ; $(MAKE) $(MFLAGS) ;

protodeps-clean:
	$(QUIET) cd ../../external/protobuf-plugin-closure ; $(MAKE) $(MFLAGS) clean ;

