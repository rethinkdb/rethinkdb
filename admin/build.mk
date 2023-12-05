#### Web UI sources

WEB_ASSETS_SRC_FILES := $(shell find $(TOP)/admin -path $(TOP)/admin/node_modules -prune -o -print)

ALL_WEB_ASSETS := $(BUILD_ROOT_DIR)/web-assets

$(BUILD_ROOT_DIR)/web-assets: $(WEB_ASSETS_SRC_FILES) $(JS_BUILD_DIR)/rethinkdb.js | $(GULP_BIN_DEP)
	$P GULP
	$(GULP) build --cwd $(TOP)/admin $(if $(filter $(VERBOSE),0), --silent) --version $(RETHINKDB_VERSION) $(if $(filter $(UGLIFY),1), --uglify)
	touch $@

.PHONY: web-assets-watch
web-assets-watch:
	$(GULP) watch --cwd $(TOP)/admin --version $(RETHINKDB_VERSION)

.PHONY: web-assets
web-assets: $(ALL_WEB_ASSETS)

.PHONY: generate-web-assets-cc
generate-web-assets-cc: web-assets
	mkdir -p $(TOP)/src/gen
	$(TOP)/scripts/compile-web-assets.py $(TOP)/build/web_assets > $(TOP)/src/gen/web_assets.cc
	grep -E 'module\.exports=\\"[0-9]+(\.[0-9]+)+\\";' $(TOP)/src/gen/web_assets.cc | xargs -0 echo -n 'Double-check version:'
