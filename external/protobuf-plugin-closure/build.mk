# protobuf-plugin-closure build

LDFLAGS ?=
CXXFLAGS ?=

PROTOC_JS_DIR := $(TOP)/external/protobuf-plugin-closure
PROTOC_JS_BUILD_DIR := $(BUILD_DIR)/protobuf-plugin-closure
PROTOC_JS_PLUGIN := $(PROTOC_JS_BUILD_DIR)/protoc-gen-js

PROTOC_LDFLAGS := $(LDFLAGS) $(PROTOBUF_LIBS) $(PTHREAD_LIBS)
PROTOC_CXXFLAGS := $(CXXFLAGS) -I $(PROTOC_JS_BUILD_DIR)

# TODO: don't assume that libprotoc is installed right next to libprotobuf
LIBPROTOC_LDFLAGS = $(subst protobuf,protoc,$(PROTOBUF_LIBS))

.PRECIOUS: $(PROTOC_JS_BUILD_DIR)/.

$(PROTOC_JS_BUILD_DIR)/js/%.pb.cc: $(PROTOC_JS_DIR)/js/%.proto | $(PROTOC_JS_BUILD_DIR)/. $(PROTOC_DEP)
	$P PROTOC
	$(PROTOC) \
	  -I $(PROTOC_JS_DIR) -I $(PROTOC_JS_DIR)/js \
	  --cpp_out=$(PROTOC_JS_BUILD_DIR) \
	  $<

$(PROTOC_JS_PLUGIN): $(PROTOC_JS_BUILD_DIR)/js/javascript_package.pb.cc
$(PROTOC_JS_PLUGIN): $(PROTOC_JS_BUILD_DIR)/js/int64_encoding.pb.cc | $(PROTOC_DEP)
	$P CC
	$(CXX) \
	  $(PROTOC_JS_DIR)/js/code_generator.cc \
	  $(PROTOC_JS_DIR)/js/protoc_gen_js.cc \
	  $^ \
	  -I $(PROTOC_JS_DIR) -I $(PROTOC_JS_DIR)/js \
	  $(PROTOC_LDFLAGS) $(PROTOC_CXXFLAGS) \
	  $(LIBPROTOC_LDFLAGS) \
	  -o $@

.PHONY: protoc-js
protoc-js: $(PROTOC_JS_PLUGIN)
