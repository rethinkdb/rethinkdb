# JavaScript engine configuration for RethinkDB
# This file handles the build configuration for different JS engines

# Default JS engine (can be overridden in config.mk)
# quickjs is default for compatibility; v8-jitless recommended for production
JS_ENGINE ?= quickjs

# Normalize engine name
ifeq ($(JS_ENGINE),v8jitless)
  JS_ENGINE := v8-jitless
endif
ifeq ($(JS_ENGINE),v8-full)
  JS_ENGINE := v8
endif

# Engine-specific settings
ifeq ($(JS_ENGINE),v8-jitless)
  JS_ENGINE_TYPE := V8_JITLESS
  JS_ENGINE_DEFINE := -DRETHINKDB_JS_ENGINE_V8 -DRETHINKDB_JS_ENGINE_V8_JITLESS
  JS_ENGINE_LDFLAGS := -lv8_monolith -licuuc
  JS_ENGINE_INCLUDES := -I$(SUPPORT_INCLUDE_DIR)/v8
  FETCH_LIST += v8
else ifeq ($(JS_ENGINE),v8)
  JS_ENGINE_TYPE := V8_FULL
  JS_ENGINE_DEFINE := -DRETHINKDB_JS_ENGINE_V8 -DRETHINKDB_JS_ENGINE_V8_FULL
  JS_ENGINE_LDFLAGS := -lv8_monolith -licuuc
  JS_ENGINE_INCLUDES := -I$(SUPPORT_INCLUDE_DIR)/v8
  FETCH_LIST += v8
else ifeq ($(JS_ENGINE),quickjs)
  JS_ENGINE_TYPE := QUICKJS
  JS_ENGINE_DEFINE := -DRETHINKDB_JS_ENGINE_QUICKJS
  JS_ENGINE_LDFLAGS := -L$(SUPPORT_LIB_DIR) -lquickjs
  JS_ENGINE_INCLUDES := -I$(SUPPORT_INCLUDE_DIR)/quickjs
  FETCH_LIST += quickjs
else ifeq ($(JS_ENGINE),quickjspp)
  # quickjspp is the C++ wrapper around QuickJS (same as quickjs but explicit)
  JS_ENGINE_TYPE := QUICKJS
  JS_ENGINE_DEFINE := -DRETHINKDB_JS_ENGINE_QUICKJS
  JS_ENGINE_LDFLAGS := -L$(SUPPORT_LIB_DIR) -lquickjs
  JS_ENGINE_INCLUDES := -I$(SUPPORT_INCLUDE_DIR)/quickjs
  FETCH_LIST += quickjs
else ifeq ($(JS_ENGINE),quickjs-ng)
  JS_ENGINE_TYPE := QUICKJS_NG
  JS_ENGINE_DEFINE := -DRETHINKDB_JS_ENGINE_QUICKJS_NG
  JS_ENGINE_LDFLAGS := -L$(SUPPORT_LIB_DIR) -lqjs
  JS_ENGINE_INCLUDES := -I$(SUPPORT_INCLUDE_DIR)/quickjs-ng
  FETCH_LIST += quickjs-ng
else ifeq ($(JS_ENGINE),duktape)
  JS_ENGINE_TYPE := DUKTAPE
  JS_ENGINE_DEFINE := -DRETHINKDB_JS_ENGINE_DUKTAPE
  JS_ENGINE_LDFLAGS := -L$(SUPPORT_LIB_DIR) -lduktape
  JS_ENGINE_INCLUDES := -I$(SUPPORT_INCLUDE_DIR)
  FETCH_LIST += duktape
else ifeq ($(JS_ENGINE),hermes)
  JS_ENGINE_TYPE := HERMES
  JS_ENGINE_DEFINE := -DRETHINKDB_JS_ENGINE_HERMES
  JS_ENGINE_LDFLAGS := -lhermes -ljsi
  JS_ENGINE_INCLUDES := -I$(SUPPORT_INCLUDE_DIR)/hermes -I$(SUPPORT_INCLUDE_DIR)/jsi
  FETCH_LIST += hermes
else
  $(error Unknown JavaScript engine: $(JS_ENGINE). Valid options: v8-jitless, v8, quickjs, quickjspp, quickjs-ng, duktape, hermes)
endif

# Default engine marker
ifeq ($(JS_ENGINE),v8-jitless)
  JS_ENGINE_DEFINE += -DRETHINKDB_JS_ENGINE_DEFAULT=js_engine_type_t::V8_JITLESS
endif

# Add to compiler flags
CXXFLAGS += $(JS_ENGINE_DEFINE) -DRETHINKDB_JS_ENGINE_$(JS_ENGINE_TYPE)
CXXFLAGS += $(JS_ENGINE_INCLUDES)
LDFLAGS += $(JS_ENGINE_LDFLAGS)

# Source files for JS engine support
JS_ENGINE_SOURCES := $(TOP)/src/extproc/js_engine_factory.cc

# All possible JS engine implementation files
ALL_JS_ENGINE_FILES := $(TOP)/src/extproc/js_engine_v8.cc $(TOP)/src/extproc/js_engine_quickjs.cc $(TOP)/src/extproc/js_engine_duktape.cc $(TOP)/src/extproc/js_engine_hermes.cc

# Add engine-specific source files based on selection
ifeq ($(JS_ENGINE),v8-jitless)
  JS_ENGINE_SOURCES += $(TOP)/src/extproc/js_engine_v8.cc
else ifeq ($(JS_ENGINE),v8)
  JS_ENGINE_SOURCES += $(TOP)/src/extproc/js_engine_v8.cc
else ifeq ($(JS_ENGINE),quickjs)
  JS_ENGINE_SOURCES += $(TOP)/src/extproc/js_engine_quickjs.cc
else ifeq ($(JS_ENGINE),quickjs-ng)
  JS_ENGINE_SOURCES += $(TOP)/src/extproc/js_engine_quickjs.cc
else ifeq ($(JS_ENGINE),duktape)
  JS_ENGINE_SOURCES += $(TOP)/src/extproc/js_engine_duktape.cc
else ifeq ($(JS_ENGINE),hermes)
  JS_ENGINE_SOURCES += $(TOP)/src/extproc/js_engine_hermes.cc
else ifeq ($(JS_ENGINE),quickjspp)
  JS_ENGINE_SOURCES += $(TOP)/src/extproc/js_engine_quickjs.cc
endif

# Determine which engine files to exclude from automatic source discovery
JS_ENGINE_EXCLUDE := $(filter-out $(JS_ENGINE_SOURCES),$(ALL_JS_ENGINE_FILES))

# Export variables
export JS_ENGINE JS_ENGINE_TYPE JS_ENGINE_DEFINE JS_ENGINE_LDFLAGS JS_ENGINE_INCLUDES JS_ENGINE_EXCLUDE
