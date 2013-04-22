
RETHINKDB_HOME=..

DRIVER_DIR=$(RETHINKDB_HOME)/drivers/javascript2
DRIVER_MAKE_DIR=$(DRIVER_DIR)/src
DRIVER_MAKE_TARGET=$(DRIVER_DIR)/build/rethinkdb.js
DRIVER_SRC_ROOT=$(DRIVER_DIR)/build/coffee

PROTOC=protoc
PROTOC_JS_HOME_DIR=$(EXTERNAL)/protobuf-plugin-closure
PROTOC_JS_PLUGIN=$(PROTOC_JS_HOME_DIR)/protoc-gen-js
PROTOC_JS_IMPORT_DIR=$(PROTOC_JS_HOME_DIR)/js
PROTOC_JS=$(PROTOC) --plugin=$(PROTOC_JS_PLUGIN) -I $(PROTOC_JS_IMPORT_DIR)

PROTO_FILE_DIR=$(RETHINKDB_HOME)/src/rdb_protocol
PROTO_BASE=ql2
PROTO_FILE=$(PROTO_FILE_DIR)/$(PROTO_BASE).proto
PB_JS_FILE=$(PROTO_BASE).pb.js

EXTERNAL=$(RETHINKDB_HOME)/external
CLOSURE_LIB=$(EXTERNAL)/google-closure-library
CLOSURE_BUILDER=$(CLOSURE_LIB)/closure/bin/build/closurebuilder.py

BUILD=build
SRC=src

OUTPUT_MODE=script

EMBEDDED=$(BUILD)/rethinkdb-web-demo.js

all: $(EMBEDDED)

driver-files: $(DRIVER_MAKE_TARGET)
	$(MAKE) -C $(DRIVER_MAKE_DIR) $(DRIVER_MAKE_TARGET)

$(PROTOC_JS_HOME_DIR)/protoc-gen-js:
	$(MAKE) -C $(EXTERNAL)/protobuf-plugin-closure

$(PB_JS_FILE): $(PROTO_FILE) $(PROTOC_JS_HOME_DIR)/protoc-gen-js
	$(PROTOC_JS) -I $(PROTO_FILE_DIR) --js_out=$(BUILD) $(PROTO_FILE)

$(EMBEDDED): $(PB_JS_FILE)
	$(CLOSURE_BUILDER) \
		--root=$(CLOSURE_LIB) \
		--root=$(DRIVER_SRC_ROOT) \
		--root=$(BUILD) \
		--root=$(SRC) \
		--namespace=rethinkdb.Embedded \
		--output_mode=$(OUTPUT_MODE) \
		--output_file=$@

clean:
	rm $(EMBEDDED)

.PHONY: all driver-files clean
