#### Web UI sources

WEBUI_DIST_DIR := $(BUILD_ROOT_DIR)/web_assets

WEBUI_STATIC_ASSETS := $(shell find $(TOP)/admin/assets -type f)
WEBUI_COPIED_ASSETS := $(patsubst $(TOP)/admin/assets/%, $(WEBUI_DIST_DIR)/%, $(WEBUI_STATIC_ASSETS))

WEBUI_JS_SRC := $(shell find $(TOP)/admin/src/{coffee,handlebars})

WEBUI_CSS_SRC := $(shell find $(TOP)/admin/src/less)

ALL_WEB_ASSETS := $(WEBUI_DIST_DIR).witness

WEBUI_NPM := NODE_PATH=$(abspath $(WEBUI_NODE_MODULES))
WEBUI_NPM += WEBUI_NODE_MODULES=$(abspath $(WEBUI_NODE_MODULES))/
WEBUI_NPM += NPM=$(abspath $(NPM)) BROWSERIFY=$(abspath $(BROWSERIFY)) LESSC=$(abspath $(LESSC))
WEBUI_NPM += WEBUI_DIST_DIR=$(abspath $(WEBUI_DIST_DIR))
WEBUI_NPM += WEBUI_BUNDLE=1
# ATN WEBUI_NPM += WEBUI_RETHINKDB_JS=$(abspath $(JS_BUILD_DIR))/rethinkdb.js
WEBUI_NPM += $(NPM) --prefix $(TOP)/admin --silent

$(WEBUI_DIST_DIR)/%: $(TOP)/admin/assets/%
	$P CP
	mkdir -p $(dir $@)
	cp $< $@

$(WEBUI_DIST_DIR)/cluster.js: $(WEBUI_JS_SRC) $(JS_BUILD_DIR)/rethinkdb.js | $(NPM_BIN_DEP) $(WEBUI_DEPS)
	$P COFFEE
	$(WEBUI_NPM) run build js $(call SUPPORT_LOG_REDIRECT, $(WEBUI_DIST_DIR)_js.log)

$(WEBUI_DIST_DIR)/cluster.css: $(WEBUI_CSS_SRC) | $(NPM_BIN_DEP) $(WEBUI_DEPS)
	$P LESSC
	$(WEBUI_NPM) run build css $(call SUPPORT_LOG_REDIRECT, $(WEBUI_DIST_DIR)_css.log)

$(WEBUI_DIST_DIR)/rethinkdb.js: $(JS_BUILD_DIR)/rethinkdb.js
	$P CP
	mkdir -p $(WEBUI_DIST_DIR)
	cp $< $@

$(ALL_WEB_ASSETS): $(WEBUI_COPIED_ASSETS) $(WEBUI_DIST_DIR)/cluster.js $(WEBUI_DIST_DIR)/cluster.css $(WEBUI_DIST_DIR)/rethinkdb.js
	touch $@

.PHONY: web-assets
web-assets: $(ALL_WEB_ASSETS)

ifeq (1,$(USE_PRECOMPILED_WEB_ASSETS))

$(BUILD_ROOT_DIR)/bundle_assets/web_assets.cc: $(PRECOMPILED_DIR)/bundle_assets/web_assets.cc | $(BUILD_ROOT_DIR)/bundle_assets/.
	$P CP
	cp -f $< $@

else # Don't use precompiled assets

ifeq ($(OS),Windows)
$(BUILD_ROOT_DIR)/bundle_assets/web_%.cc $(BUILD_ROOT_DIR)/bundle_assets/web_%.rc: $(TOP)/scripts/build-web-%-rc.py $(ALL_WEB_ASSETS) | $(BUILD_ROOT_DIR)/bundle_assets/.
	$P GENERATE
	$(TOP)/scripts/build-web-assets-rc.py $(WEB_ASSETS_BUILD_DIR) $(dir $@)
else
$(BUILD_ROOT_DIR)/bundle_assets/web_assets.cc: $(TOP)/scripts/compile-web-assets.py $(ALL_WEB_ASSETS) | $(BUILD_ROOT_DIR)/bundle_assets/.
	$P GENERATE
	$(TOP)/scripts/compile-web-assets.py $(WEB_ASSETS_BUILD_DIR) > $@
endif

endif
