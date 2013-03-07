BUILD=build
SRC=src
DRIVERS=drivers

YAML_FILES=$(shell find $(SRC) -name '*.yaml')
DRIVER_FILES=$(wildcard $(DRIVERS)/*)
TEST_FILES=$(patsubst $(DRIVERS)/driver.%,$(BUILD)/test.%,$(DRIVER_FILES))

.SILENT:

run: $(TEST_FILES)
	./test-runner run

py js rb: $(TEST_FILES)
	./test-runner run_$@

$(TEST_FILES): $(YAML_FILES) $(DRIVER_FILES) test-runner
	mkdir -p build
	./test-runner build_test_scripts

py_connect: connections/connection.py
	cd connections/; python connection.py

js_connect: connections/connection.js
	cd connections/; node connection.js

connect: js_connect py_connect

clean:
	rm -r build

.PHONY: run py js rb clean connect py_connect js_connect
