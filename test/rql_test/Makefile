SRC=src
DRIVERS=drivers

YAML_FILES=$(shell find $(SRC) -name '*.yaml')
DRIVER_FILES=$(wildcard $(DRIVERS)/*)
TEST_FILES=$(patsubst $(DRIVERS)/driver.%,build/test.%,$(DRIVER_FILES))

BUILD_DIR='../../build/release'
TEST_DEFAULT_PORT=0
SHARD=0
ifeq (1,$(SKIP_SLOW))
  TEST=[^_].*
else
  TEST=.*
endif

SHELL=/bin/bash

DEFAULT_LANGUAGE :=

PORT_OFFSET ?= 0
DRIVER_PORT ?= 28015+$(PORT_OFFSET)
CLUSTER_PORT ?= 29015+$(PORT_OFFSET)
EXECUTABLE := $(BUILD_DIR)/rethinkdb

.SILENT:

.PHONY: run
run: $(TEST_FILES) cursor connect
	./test-runner run \"$(BUILD_DIR)\"

.PHONY: py
py: py_connect py_cursor py_polyglot

.PHONY: js
js: js_connect js_cursor js_polyglot

.PHONY: rb
rb: rb_polyglot

.PHONY: py_polyglot
py_polyglot: $(TEST_FILES)
	./test-runner run \"$(BUILD_DIR)\" \"py\"

.PHONY: js_polyglot
js_polyglot: $(TEST_FILES)
	./test-runner run \"$(BUILD_DIR)\" \"js\"

.PHONY: rb_polyglot
rb_polyglot: $(TEST_FILES)
	./test-runner run \"$(BUILD_DIR)\" \"rb\"

.PHONY: attach
attach: $(TEST_FILES)
	./test-runner attach $(DRIVER_PORT) $(CLUSTER_PORT) '"$(EXECUTABLE)"' $(if $(DEFAULT_LANGUAGE),"'$(DEFAULT_LANGUAGE)'")

.PHONY: build_test_scripts
$(TEST_FILES): build_test_scripts
build_test_scripts: $(YAML_FILES) $(DRIVER_FILES)
	mkdir -p build
	RQL_TEST='$(TEST)' ./test-runner build_test_scripts $(SHARD)

.PHONY: list
list:
	cd src && find . -name \*.yaml | sed 's|^./||; s|/|-|g; s|.yaml$$||'

.PHONY: polyglot
polyglot: py_polyglot js_polyglot rb_polyglot

.PHONY: py_connect
py_connect: connections/connection.py
	python connections/connection.py $(BUILD_DIR) $(TEST_DEFAULT_PORT)

.PHONY: js_connect
js_connect: connections/connection.js
	mkdir -p run
	BUILD_DIR=$(BUILD_DIR) TEST_DEFAULT_PORT=$(TEST_DEFAULT_PORT) \
	    _mocha connections/connection.js

.PHONY: cursor
cursor:
	python connections/cursor_test.py $(BUILD_DIR)

.PHONY: js_cursor
js_cursor:
	python connections/cursor_test.py $(BUILD_DIR) js

.PHONY: py_cursor
py_cursor:
	python connections/cursor_test.py $(BUILD_DIR) py

.PHONY: connect
connect: js_connect py_connect

.PHONY: clean
clean:
	rm -rfv build run
