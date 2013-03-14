SRC=src
DRIVERS=drivers

YAML_FILES=$(shell find $(SRC) -name '*.yaml')
DRIVER_FILES=$(wildcard $(DRIVERS)/*)
TEST_FILES=$(patsubst $(DRIVERS)/driver.%,$(BUILD)/test.%,$(DRIVER_FILES))

BUILD='release'
TEST_DEFAULT_PORT=0
SHARD=0

.SILENT:

run: $(TEST_FILES) cursor connect
	./test-runner run \"$(BUILD)\"

#py: py_connect py_cursor

js: js_connect js_cursor

py js rb: $(TEST_FILES)
	#python connections/cursor_test.py $(BUILD) $@
	./test-runner run \"$(BUILD)\" \"$@\"

$(TEST_FILES): $(YAML_FILES) $(DRIVER_FILES) test-runner
	mkdir -p build
	./test-runner build_test_scripts $(SHARD)

py_connect: connections/connection.py
	python connections/connection.py $(BUILD) $(TEST_DEFAULT_PORT)

js_connect: connections/connection.js
	BUILD=$(BUILD) TEST_DEFAULT_PORT=$(TEST_DEFAULT_PORT) \
	    _mocha -R spec connections/connection.js

cursor:
	python connections/cursor_test.py $(BUILD)

js_cursor:
	python connections/cursor_test.py $(BUILD) js

py_cursor:
	python connections/cursor_test.py $(BUILD) py


connect: js_connect py_connect

clean:
	rm -r build

.PHONY: run py js rb clean connect py_connect js_connect
