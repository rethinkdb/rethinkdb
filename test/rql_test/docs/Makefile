YAML_DIR=src
JSON_OUT=~/rethinkdb-www/_site/assets/js/reql_docs.json

YAML_FILES=$(shell find $(YAML_DIR) -name '*.yaml')

SCRIPTS=./scripts
JSON_CONVERT=$(SCRIPTS)/json_convert.py
BUILD_TESTS=$(SCRIPTS)/build_tests.py

BUILD_DIR=build

$(JSON_OUT): $(JSON_CONVERT) $(YAML_FILES)
	python $(JSON_CONVERT) $(YAML_DIR) $(JSON_OUT)

$(BUILD_DIR)/test.py $(BUILD_DIR)/test.js $(BUILD_DIR)/test.rb: $(BUILD_TESTS) $(YAML_FILES)
	mkdir -p $(BUILD_DIR)
	python $(BUILD_TESTS) $(YAML_DIR)

build_tests: $(BUILD_DIR)/test.py $(BUILD_DIR)/test.js $(BUILD_DIR)/test.rb

clean:
	rm -f $(JSON_OUT)
	rm -fr $(BUILD_DIR)

.PHONY: clean build_tests
