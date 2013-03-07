#### Web UI sources

WEB_SOURCE_DIR := $(TOP)/admin
WEB_ASSETS_OBJ_DIR := $(BUILD_DIR)/webobj
WEB_ASSETS_RELATIVE := cluster-min.js cluster.css index.html js fonts images favicon.ico js/rethinkdb.js js/template.js
BUILD_WEB_ASSETS := $(foreach a,$(WEB_ASSETS_RELATIVE),$(WEB_ASSETS_BUILD_DIR)/$(a))

# coffee script can't handle dependencies.
COFFEE_SOURCES := $(patsubst %, $(WEB_SOURCE_DIR)/static/coffee/%,\
			util.coffee \
			loading.coffee \
			body.coffee \
			ui_components/modals.coffee ui_components/list.coffee ui_components/progressbar.coffee \
			namespaces/database.coffee \
			namespaces/index.coffee namespaces/replicas.coffee namespaces/shards.coffee namespaces/server_assignments.coffee namespaces/namespace.coffee \
			servers/index.coffee servers/machine.coffee servers/datacenter.coffee \
			dashboard.coffee \
			dataexplorer.coffee \
			sidebar.coffee \
			resolve_issues.coffee \
			log_view.coffee \
			vis.coffee \
			models.coffee \
			navbar.coffee \
			router.coffee \
			app.coffee)
LESS_SOURCES := $(shell find $(WEB_SOURCE_DIR)/static/less -name '*.less')
LESS_MAIN := $(WEB_SOURCE_DIR)/static/less/styles.less
CLUSTER_HTML := $(WEB_SOURCE_DIR)/templates/cluster.html
JS_EXTERNAL_DIR := $(WEB_SOURCE_DIR)/static/js
FONTS_EXTERNAL_DIR := $(WEB_SOURCE_DIR)/static/fonts
IMAGES_EXTERNAL_DIR := $(WEB_SOURCE_DIR)/static/images
FAVICON := $(WEB_SOURCE_DIR)/favicon.ico

.PHONY: $(TOP)/admin/all
$(TOP)/admin/all: web-assets

.PHONY: $(TOP)/admin/clean
$(TOP)/admin/clean:
	$P RM $(WEB_ASSETS_BUILD_DIR)
	rm -rf $(WEB_ASSETS_BUILD_DIR)
	$P RM $(WEB_ASSETS_OBJ_DIR)
	rm -rf $(WEB_ASSETS_OBJ_DIR)

.PHONY: web-assets
web-assets: $(BUILD_WEB_ASSETS) | $(BUILD_DIR)/.

ifeq (1,$(USE_PRECOMPILED_WEB_ASSETS))

$(WEB_ASSETS_BUILD_DIR):
	$P MKDIR
	mkdir -p $@

$(WEB_ASSETS_BUILD_DIR)/%: $(PRECOMPILED_DIR)/web/% | $(WEB_ASSETS_BUILD_DIR)
	$P CP
	mkdir -p $(dir $@)
	cp -pRP $< $@

$(PRECOMPILED_DIR)/web/%:
	$(error Missing file $@. Run ./configure with --disable-precompiled-web to build normally.)

else # Don't use precompiled assets

$(WEB_ASSETS_BUILD_DIR)/js/rethinkdb.js: $(JS_BUILD_DIR)/rethinkdb.js | $(WEB_ASSETS_BUILD_DIR)/js/.
	$P CP
	cp -pRP $< $@

$(WEB_ASSETS_BUILD_DIR)/js/template.js: $(WEB_SOURCE_DIR)/static/handlebars $(HANDLEBARS) $(TOP)/scripts/build_handlebars_templates.py | $(WEB_ASSETS_BUILD_DIR)/js/.
	$P HANDLEBARS $@
	env TC_HANDLEBARS_EXE=$(HANDLEBARS) $(TOP)/scripts/build_handlebars_templates.py $(WEB_SOURCE_DIR)/static/handlebars $(BUILD_DIR) $(WEB_ASSETS_BUILD_DIR)/js

$(WEB_ASSETS_OBJ_DIR)/cluster-min.concat.coffee: $(COFFEE_SOURCES) | $(WEB_ASSETS_OBJ_DIR)/.
	$P CONCAT $@
	cat $+ > $@

$(WEB_ASSETS_BUILD_DIR)/cluster-min.js: $(WEB_ASSETS_OBJ_DIR)/cluster-min.concat.coffee $(COFFEE) | $(WEB_ASSETS_BUILD_DIR)/.
	$P COFFEE $@
	$(COFFEE) -bp --stdio < $(WEB_ASSETS_OBJ_DIR)/cluster-min.concat.coffee > $@

$(WEB_ASSETS_BUILD_DIR)/cluster.css: $(LESS_MAIN) $(LESSC) | $(WEB_ASSETS_BUILD_DIR)/.
	$P LESSC $@
	@echo "    LESSC $@"
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