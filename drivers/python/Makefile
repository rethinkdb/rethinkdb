RETHINKDB_HOME=../..
PROTO_FILE_SRC=$(RETHINKDB_HOME)/src/rdb_protocol/ql2.proto

PYTHON_PB_FILE=rethinkdb/ql2_pb2.py
PROTO_FILE=ql2.proto
PYTHON_DOCS=rethinkdb/docs.py

all: $(PYTHON_PB_FILE) $(PYTHON_DOCS) $(PROTO_FILE)

$(PYTHON_DOCS): $(RETHINKDB_HOME)/docs/rql/py_docs.json
	python gendocs.py > $@

$(PYTHON_PB_FILE): $(PROTO_FILE)
	protoc --python_out=rethinkdb $(PROTO_FILE)

$(PROTO_FILE): $(PROTO_FILE_SRC)
	cp $< $@

clean:
	rm -f $(PYTHON_PB_FILE)
	rm -f $(PROTO_FILE)
	rm -f $(PYTHON_DOCS)
	rm -rf ./build
	rm -rf ./dist
	rm -rf ./rethinkdb.egg-info
	rm -rf ./protobuf

PY_PKG_DIR=$(RETHINKDB_HOME)/build/packages/python

sdist: $(PYTHON_PB_FILE) $(PYTHON_DOCS) $(PROTO_FILE)
	rm -rf $(PY_PKG_DIR)
	mkdir -p $(PY_PKG_DIR)
	cp setup.py $(PY_PKG_DIR)
	cp MANIFEST.in $(PY_PKG_DIR)
	cp -r rethinkdb $(PY_PKG_DIR)
	cp $(PYTHON_PB_FILE) $(PY_PKG_DIR)/rethinkdb
	cp $(PROTO_FILE) $(PY_PKG_DIR)/$(PROTO_FILE)
	cd $(PY_PKG_DIR) && python setup.py sdist

publish: sdist
	cd $(PY_PKG_DIR) && python setup.py register upload

install: sdist
	cd $(PY_PKG_DIR) && PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=cpp python setup.py install

.PHONY: all clean publish sdist install
