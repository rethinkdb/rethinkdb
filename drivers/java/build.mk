# Copyright 2010-2015 RethinkDB

JAVA_SRC_DIR=$(TOP)/drivers/java
JAVA_BUILD_DIR=$(TOP)/build/drivers/java
JAVA_PKG_DIR=$(TOP)/build/package/java

METAJAVA=$(JAVA_SRC_DIR)/metajava.py

PACKAGE_DIR=$(JAVA_SRC_DIR)/src/main/java/com/rethinkdb
TEMPLATE_DIR=$(JAVA_SRC_DIR)/templates
PROTO_DIR=$(PACKAGE_DIR)/proto
AST_GEN_DIR=$(PACKAGE_DIR)/ast/gen
MODEL_DIR=$(PACKAGE_DIR)/model
PROTO_FILE=$(TOP)/src/rdb_protocol/ql2.proto
PROTO_JSON=$(JAVA_SRC_DIR)/proto_basic.json
TERM_INFO=$(JAVA_SRC_DIR)/term_info.json
GLOBAL_INFO=$(JAVA_SRC_DIR)/global_info.json

.PHONY: java-driver
java-driver: ast-files | $(JAVA_BUILD_DIR)/.

$(PROTO_JSON): $(PROTO_FILE)
	$P CONVERT PROTOFILE
	$(PYTHON) $(JAVA_SRC_DIR)/convert_protofile.py $(PROTO_FILE) $(PROTO_JSON)

$(TERM_INFO): $(PROTO_JSON)
	$P UPDATING $(notdir $@)
	$(PYTHON) $(METAJAVA) update-terminfo --term-info=$(TERM_INFO) --proto-json=$(PROTO_JSON)

.PHONY: ast-files
ast-files: $(TERM_INFO)
	$P GENERATING TERM AST CLASSES
	$(PYTHON) $(METAJAVA) render-term-ast --term-info=$(TERM_INFO)
