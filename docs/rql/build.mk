
RQL_DOCS_DIR := $(TOP)/docs/rql
DOCS_YAML_DIR := $(RQL_DOCS_DIR)/src
DOCS_JSON_OUT := $(WEB_ASSETS_BUILD_DIR)/js/reql_docs.json

DOCS_YAML_FILES := $(shell find $(DOCS_YAML_DIR) -name '*.yaml')

DOCS_SCRIPTS := $(RQL_DOCS_DIR)/scripts
DOCS_JSON_CONVERT := $(DOCS_SCRIPTS)/json_convert.py
DOCS_VALIDATE_SCRIPT := $(DOCS_SCRIPTS)/validate_examples.py
DOCS_HEADER := $(RQL_DOCS_DIR)/header.json

.PHONY: docs
docs: $(DOCS_JSON_OUT)

ifneq (1,$(USE_PRECOMPILED_WEB_ASSETS))
  $(DOCS_JSON_OUT): $(DOCS_JSON_CONVERT) $(DOCS_YAML_FILES) $(DOCS_HEADER) | $(WEB_ASSETS_BUILD_DIR)/js/.
	$P JSON_CONVERT
	python $(DOCS_JSON_CONVERT) $(DOCS_YAML_DIR) $@ $(DOCS_HEADER)
endif

.PHONY: docs-validate
docs-validate: $(DOCS_YAML_FILES) | $(RQL_DOCS_DIR)/build/.
	cd $(RQL_DOCS_DIR); \
	  python $(abspath $(DOCS_VALIDATE_SCRIPT)) $(abspath $(DOCS_YAML_DIR))

.PHONY: docs-clean
docs-clean:
	$P RM $(DOCS_JSON_OUT)
	$P RM $(RQL_DOCS_DIR)/build
	rm -f $(DOCS_JSON_OUT)
	rm -rf $(RQL_DOCS_DIR)/build

