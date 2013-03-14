PROTOC=protoc
RETHINKDB_HOME=../..
PROTO_FILE_DIR=$(RETHINKDB_HOME)/src/rdb_protocol
PROTO_BASE=ql2
PROTO_FILE=$(PROTO_FILE_DIR)/$(PROTO_BASE).proto
BUILD_DIR=rethinkdb
PYTHON_PB_FILE=$(BUILD_DIR)/$(PROTO_BASE)_pb2.py

$(PYTHON_PB_FILE): $(PROTO_FILE)
	$(PROTOC) --python_out=$(BUILD_DIR) -I$(PROTO_FILE_DIR) $(PROTO_FILE)

clean:
	rm $(PYTHON_PB_FILE)

PY_PKG_DIR=$(RETHINKDB_HOME)/build/packages/python

publish: $(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE)
	mkdir -p $(PY_PKG_DIR)
	cp setup.py $(PY_PKG_DIR)
	cp -r rethinkdb $(PY_PKG_DIR)
	rm $(PY_PKG_DIR)/rethinkdb/$(PYTHON_PBFILE)
	cp $(PYTHON_PBDIR)/rdb_protocol/$(PYTHON_PBFILE) $(PY_PKG_DIR)/rethinkdb
	cd $(PY_PKG_DIR); python setup.py register sdist upload
