
JSON_SRC=~/rethinkdb/admin/static/js/reql_docs.json
YAML_OUT=docs.yaml
YAML_SRC=$(YAML_OUT)
JSON_OUT=docs.json

SRCIPTS=scripts
YAML_CONVERT=$(SRCIPTS)/yaml_convert.py
JSON_CONVERT=$(SRCIPTS)/json_convert.py

$(JSON_OUT): $(JSON_CONVERT) $(YAML_SRC)
	python $(JSON_CONVERT) $(YAML_SRC) $(JSON_OUT)

$(YAML_OUT): $(YAML_CONVERT) $(JSON_SRC)
	python $(YAML_CONVERT) $(JSON_SRC) $(YAML_OUT)

clean: $(YAML_OUT) $(JSON_OUT)
	rm -f $(YAML_OUT)
	rm -f $(JSON_OUT)

.PHONY: clean
