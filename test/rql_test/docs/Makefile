YAML_DIR=src
JSON_OUT=./rql_docs.json

YAML_FILES=$(shell find $(YAML_DIR) -name '*.yaml')

SRCIPTS=scripts
JSON_CONVERT=$(SRCIPTS)/json_convert.py
VALIDATE_SCRIPT=$(SRCIPTS)/validate_examples.py

$(JSON_OUT): $(JSON_CONVERT) $(YAML_FILES)
	python $(JSON_CONVERT) $(YAML_DIR) $(JSON_OUT)

validate: $(YAML_FILES)
	python $(VALIDATE_SCRIPT) $(YAML_DIR)

clean: $(JSON_OUT)
	rm -f $(JSON_OUT)

.PHONY: clean validate
