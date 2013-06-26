
RE2_LIBS := $(BUILD_DIR)/re2/lib/libre2.a
RE2_INCLUDE_DIR := $(BUILD_DIR)/re2/include/
RE2_CXXFLAGS := -isystem $(RE2_INCLUDE_DIR)
RE2_DEP := $(RE2_LIBS) $(RE2_INCLUDE_DIR)
RE2_BUILD_LOG := $(BUILD_DIR)/re2/build.log

ifneq ($(VERBOSE),1)
  RE2_LOG_REDIRECT := > $(RE2_BUILD_LOG) 2>&1
  RE2_LOG_REDIRECT += || (echo Failed to build re2: ; cat $(RE2_BUILD_LOG) ; false )
else
  RE2_LOG_REDIRECT :=
endif

.PHONY: re2
re2: $(RE2_DEP)

$(RE2_LIBS) $(RE2_INCLUDE_DIR): $(BUILD_DIR)/re2/.build

$(BUILD_DIR)/re2/.build: | $(BUILD_DIR)/re2/.
	$P MAKE -C $(EXTERNAL_DIR)/re2
	$(EXTERN_MAKE) -C $(TOP)/external/re2 install --silent \
	  prefix=$(abspath $(BUILD_DIR)/re2) CXX=$(CXX) \
	  CXXFLAGS="-O3 $(CXXFLAGS)" LDFLAGS="$(LDFLAGS) $(PTHREAD_LIBS)" \
	  $(RE2_LOG_REDIRECT)
	touch $@
