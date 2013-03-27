YAML_DIR=src
JSON_OUT=~/rethinkdb-www/_site/assets/js/reql_docs.json

YAML_FILES=$(shell find $(YAML_DIR) -name '*.yaml')

SRCIPTS=scripts
JSON_CONVERT=$(SRCIPTS)/json_convert.py

$(JSON_OUT): $(JSON_CONVERT) $(YAML_FILES)
	python $(JSON_CONVERT) $(YAML_DIR) $(JSON_OUT)

clean: $(JSON_OUT)
	rm -f $(JSON_OUT)

.PHONY: clean
