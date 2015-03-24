#### Web UI sources

ifeq (1,$(USE_PRECOMPILED_WEB_ASSETS))

ALL_WEB_ASSETS := $(WEB_ASSETS_BUILD_DIR)

$(WEB_ASSETS_BUILD_DIR): $(PRECOMPILED_DIR)/web | $(BUILD_DIR)/.
	$P CP
	rm -rf $@
	cp -pRP $< $@

else # Don't use precompiled assets

WEB_ASSETS_SRC_FILES := $(shell find $(TOP)/admin -path $(TOP)/admin/node_modules -prune -o -print)

ALL_WEB_ASSETS := $(BUILD_ROOT_DIR)/web-assets

$(BUILD_ROOT_DIR)/web-assets: $(WEB_ASSETS_SRC_FILES) $(JS_BUILD_DIR)/rethinkdb.js | $(GULP_BIN_DEP)
	$P GULP
	$(GULP) build --cwd $(TOP)/admin $(if $(filter $(VERBOSE),0), --silent) --version $(RETHINKDB_VERSION) $(if $(filter $(UGLIFY),1), --uglify)
	touch $@

.PHONY: web-assets-watch
web-assets-watch:
	$(GULP) --cwd $(TOP)/admin --version $(RETHINKDB_VERSION)

endif # USE_PRECOMPILED_WEB_ASSETS = 1

.PHONY: web-assets
web-assets: $(ALL_WEB_ASSETS)
