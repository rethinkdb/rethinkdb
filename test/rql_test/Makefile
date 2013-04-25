SRC=src
DRIVERS=drivers

YAML_FILES=$(shell find $(SRC) -name '*.yaml')
DRIVER_FILES=$(wildcard $(DRIVERS)/*)
TEST_FILES=$(patsubst $(DRIVERS)/driver.%,build/test.%,$(DRIVER_FILES))

BUILD_DIR='../../build/release'
TEST_DEFAULT_PORT=0
SHARD=0
TEST=.*

SHELL=/bin/bash

.SILENT:

.PHONY: run
run: $(TEST_FILES) cursor connect
	./test-runner run \"$(BUILD_DIR)\"

.PHONY: py
py: py_connect py_cursor py_polyglot

.PHONY: js
js: js_connect js_cursor py_polyglot

.PHONY: rb
rb: rb_polyglot

.PHONY: py_polyglot js_polyglot rb_polyglot
py_polyglot js_polyglot rb_polyglot: $(TEST_FILES)
	./test-runner run \"$(BUILD_DIR)\" \"$(shell echo $@ | cut -b 1-2)\"

$(TEST_FILES): build/.stub
build/.stub: $(YAML_FILES) $(DRIVER_FILES)
	mkdir -p build
	RQL_TEST=$(TEST) ./test-runner build_test_scripts $(SHARD)
	touch $@

.PHONY: list
list:
	cd src && find -name \*.yaml | sed 's|^./||; s|/|-|g; s|.yaml$$||'

.PHONY: polyglot
polyglot: py_polyglot js_polyglot rb_polyglot

.PHONY: py_connect
py_connect: connections/connection.py
	python connections/connection.py $(BUILD_DIR) $(TEST_DEFAULT_PORT)

.PHONY: js_connect
js_connect: connections/connection.js
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
	rm -r build
