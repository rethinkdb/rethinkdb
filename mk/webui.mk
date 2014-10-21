#### Web UI sources

.PHONY: $(TOP)/admin/all
$(TOP)/admin/all: web-assets

.PRECIOUS: $(WEB_ASSETS_BUILD_DIR)/.

ifeq (1,$(USE_PRECOMPILED_WEB_ASSETS))

$(WEB_ASSETS_BUILD_DIR): $(PRECOMPILED_DIR)/web | $(BUILD_DIR)/.
	$P CP
	cp -pRP $< $@

.PHONY: web-assets
web-assets: $(WEB_ASSETS_BUILD_DIR)

else # Don't use precompiled assets

WEB_SOURCE_DIR := $(TOP)/admin
WEB_ASSETS_OBJ_DIR := $(BUILD_DIR)/webobj
WEB_ASSETS_RELATIVE := cluster-min.js cluster.css index.html js fonts images favicon.ico js/rethinkdb.js js/template.js
BUILD_WEB_ASSETS := $(foreach a,$(WEB_ASSETS_RELATIVE),$(WEB_ASSETS_BUILD_DIR)/$(a))

COFFEE_SOURCE_DIR := $(WEB_SOURCE_DIR)/static/coffee
# since we don't use requirejs or anything, we list out the dependencies here explicitly in order
# this means you should use $+ instead of $^ for rules with $(COFFEE_SOURCES) as dependencies since it preserves order!
COFFEE_SOURCES := $(patsubst %, $(WEB_SOURCE_DIR)/static/coffee/%,\
                       util.coffee \
                       body.coffee \
                       ui_components/modals.coffee ui_components/progressbar.coffee \
                       tables/database.coffee \
                       tables/index.coffee tables/replicas.coffee tables/shards.coffee tables/shard_assignments.coffee tables/table.coffee \
                       servers/index.coffee servers/server.coffee \
                       dashboard.coffee \
                       dataexplorer.coffee \
                       topbar.coffee \
                       resolve_issues.coffee \
                       log_view.coffee \
                       vis.coffee \
                       modals.coffee \
                       models.coffee \
                       navbar.coffee \
                       router.coffee \
                       app.coffee)
COFFEE_COMPILED_JS := $(patsubst $(COFFEE_SOURCE_DIR)/%.coffee, $(WEB_ASSETS_OBJ_DIR)/%.js, $(COFFEE_SOURCES))
JS_VERSION_FILE := $(WEB_ASSETS_OBJ_DIR)/version.js
LESS_SOURCES := $(shell find $(WEB_SOURCE_DIR)/static/less -name '*.less')
LESS_MAIN := $(WEB_SOURCE_DIR)/static/less/styles.less
CLUSTER_HTML := $(WEB_SOURCE_DIR)/templates/cluster.html
JS_EXTERNAL_DIR := $(WEB_SOURCE_DIR)/static/js
FONTS_EXTERNAL_DIR := $(WEB_SOURCE_DIR)/static/fonts
IMAGES_EXTERNAL_DIR := $(WEB_SOURCE_DIR)/static/images
FAVICON := $(WEB_SOURCE_DIR)/favicon.ico


HANDLEBAR_HTML_FILES := $(shell find $(WEB_SOURCE_DIR)/static/handlebars -name \*.html)

.PHONY: web-assets
web-assets: $(BUILD_WEB_ASSETS) | $(BUILD_DIR)/.

$(WEB_ASSETS_BUILD_DIR)/js/rethinkdb.js: $(JS_BUILD_DIR)/rethinkdb.js | $(WEB_ASSETS_BUILD_DIR)/js/.
	$P CP
	cp -pRP $< $@

$(WEB_ASSETS_BUILD_DIR)/js/template.js: $(HANDLEBAR_HTML_FILES) $(HANDLEBARS_BIN_DEP) $(TOP)/scripts/build_handlebars_templates.py | $(WEB_ASSETS_BUILD_DIR)/js/.
	$P HANDLEBARS $@
	env TC_HANDLEBARS_EXE=$(HANDLEBARS) $(TOP)/scripts/build_handlebars_templates.py $(WEB_SOURCE_DIR)/static/handlebars $(BUILD_DIR) $(WEB_ASSETS_BUILD_DIR)/js

$(WEB_ASSETS_BUILD_DIR)/cluster-min.js: $(JS_VERSION_FILE) $(COFFEE_COMPILED_JS) | $(WEB_ASSETS_BUILD_DIR)/.
	$P CONCAT $@
	cat $+ > $@

$(JS_VERSION_FILE): | $(WEB_ASSETS_OBJ_DIR)/.
	echo "window.VERSION = '$(RETHINKDB_VERSION)';" > $@

$(WEB_ASSETS_OBJ_DIR)/%.js: $(COFFEE_SOURCE_DIR)/%.coffee $(COFFEE_BIN_DEP) | $(WEB_ASSETS_OBJ_DIR)/.
	$P COFFEE $@
	mkdir -p $(@D)
	$(COFFEE) --bare --print --stdio < $< > $@

$(WEB_ASSETS_BUILD_DIR)/cluster.css: $(LESS_MAIN) $(LESSC_BIN_DEP) | $(WEB_ASSETS_BUILD_DIR)/.
	$P LESSC $@
	$(LESSC) $(LESS_MAIN) > $@

$(WEB_ASSETS_BUILD_DIR)/index.html: $(CLUSTER_HTML) | $(WEB_ASSETS_BUILD_DIR)/.
	$P SED
	sed "s/{RETHINKDB_VERSION}/$(RETHINKDB_VERSION)/" $(CLUSTER_HTML) > $@

$(WEB_ASSETS_BUILD_DIR)/js: | $(WEB_ASSETS_BUILD_DIR)/.
	$P CP $(JS_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)
	cp -RP $(JS_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)

$(WEB_ASSETS_BUILD_DIR)/fonts: | $(WEB_ASSETS_BUILD_DIR)/.
	$P CP $(FONTS_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)
	cp -RP $(FONTS_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)

$(WEB_ASSETS_BUILD_DIR)/images: | $(WEB_ASSETS_BUILD_DIR)/.
	$P CP $(IMAGES_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)
	cp -RP $(IMAGES_EXTERNAL_DIR) $(WEB_ASSETS_BUILD_DIR)

$(WEB_ASSETS_BUILD_DIR)/favicon.ico: $(FAVICON) | $(WEB_ASSETS_BUILD_DIR)/.
	$P CP $(FAVICON) $(WEB_ASSETS_BUILD_DIR)
	cp -P $(FAVICON) $(WEB_ASSETS_BUILD_DIR)

endif # USE_PRECOMPILED_WEB_ASSETS = 1
