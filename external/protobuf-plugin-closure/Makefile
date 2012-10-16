# protobuf-plugin-closure build

SPREFIX?=/usr
INCLUDE=$(SPREFIX)/include/
LIB=$(SPREFIX)/lib/
PROTOC=protoc

all: javascript_package int64_encoding js_plugin ccjs_plugin

javascript_package:
	$(PROTOC) \
    -I js \
    -I $(INCLUDE) \
    --cpp_out=js \
    js/javascript_package.proto

int64_encoding:
	$(PROTOC) \
    -I js \
    -I $(INCLUDE) \
    --cpp_out=js \
    js/int64_encoding.proto

js_plugin: javascript_package int64_encoding
	g++ -I $(INCLUDE) \
    -I . \
    ./js/code_generator.cc \
    ./js/protoc_gen_js.cc \
    ./js/javascript_package.pb.cc \
    ./js/int64_encoding.pb.cc \
    -l:$(LIB)libprotobuf.a \
    -l:$(LIB)libprotoc.a \
    -o ./protoc-gen-js \
    -lpthread

ccjs_plugin: javascript_package int64_encoding
	g++ -I $(INCLUDE) \
    -I . \
    ./ccjs/code_generator.cc \
    ./ccjs/protoc_gen_ccjs.cc \
    ./js/int64_encoding.pb.cc \
    -l:$(LIB)libprotobuf.a \
    -l:$(LIB)libprotoc.a \
    -o ./protoc-gen-ccjs \
    -lpthread

clean:
	rm js/javascript_package.pb.*
