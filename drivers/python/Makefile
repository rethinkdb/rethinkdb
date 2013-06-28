PROTOC=protoc
RETHINKDB_HOME=../..
PROTO_FILE_DIR=$(RETHINKDB_HOME)/src/rdb_protocol
PROTO_BASE=ql2
PROTO_FILE=$(PROTO_FILE_DIR)/$(PROTO_BASE).proto

PYTHON_SRC=rethinkdb
PBCPP_SRC=rethinkdb
PYTHON_PB_FILE=$(PYTHON_SRC)/$(PROTO_BASE)_pb2.py
CPP_PB_FILE=$(PBCPP_SRC)/$(PROTO_BASE).pb.cc
PBCPP=pbcpp.so
PBCPP_BUILT=./build/lib.linux-x86_64-2.7/pbcpp.so

all: $(PYTHON_PB_FILE) $(PBCPP)

$(PYTHON_PB_FILE): $(PROTO_FILE)
	$(PROTOC) --python_out=$(PYTHON_SRC) -I$(PROTO_FILE_DIR) $(PROTO_FILE)

$(CPP_PB_FILE): $(PROTO_FILE)
	$(PROTOC) --cpp_out=$(PBCPP_SRC) -I$(PROTO_FILE_DIR) $(PROTO_FILE)

$(PBCPP): $(PBCPP_BUILT)
	cp $(PBCPP_BUILT) $(PBCPP)

$(PBCPP_BUILT): $(CPP_PB_FILE)
	python setup.py build 

clean:
	rm -f $(PYTHON_PB_FILE)
	rm -f $(CPP_PB_FILE)
	rm -f $(PBCPP)
	rm -rf ./build
	rm -rf ./dist
	rm -rf ./rethinkdb.egg-info
	rm -rf ./protobuf

PY_PKG_DIR=$(RETHINKDB_HOME)/build/packages/python

sdist: $(PYTHON_PB_FILE) $(CPP_PB_FILE)
	/usr/bin/python setup.py sdist

publish: $(PYTHON_PB_FILE)
	mkdir -p $(PY_PKG_DIR)
	cp setup.py $(PY_PKG_DIR)
	cp -r rethinkdb $(PY_PKG_DIR)
	cp $(PYTHON_PB_FILE) $(PY_PKG_DIR)/rethinkdb
	cd $(PY_PKG_DIR); python setup.py register sdist upload

.PHONY: all clean publish sdist
