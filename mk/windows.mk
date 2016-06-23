# Windows build

ifeq (1,$(DEBUG))
  CONFIGURATION := Debug
else
  CONFIGURATION := Release
endif

$/rethinkdb.vcxproj.xml:
	test -e $@ || sed "s/RETHINKDB_VERSION/`$/scripts/gen-version.sh`/" < $/mk/rethinkdb.vcxproj.xml > $@

$/%.vcxproj $/%-unittest.vcxproj: $/%.vcxproj.xml $/mk/%.vcxproj.xsl
	$P GEN
	cscript /nologo $/mk/gen-vs-project.js

.PHONY: windows-all
windows-all: $/build/$(CONFIGURATION)_$(PLATFORM)/rethinkdb.exe
ifeq (1,$(DEBUG))
  windows-all: $/build/$(CONFIGURATION)_$(PLATFORM)/rethinkdb-unittest.exe
endif

SOURCES := $(shell find $(SOURCE_DIR) \( -name '*.cc' -or -name '*.hpp' -or -name '*.tcc' \) -and -not -name '\.*')

SOURCES_NOUNIT := $(filter-out $(SOURCE_DIR)/unittest/%,$(SOURCES))

LIB_DEPS := $(foreach dep, $(FETCH_LIST), $(SUPPORT_BUILD_DIR)/$(dep)_$($(dep)_VERSION)/$(INSTALL_WITNESS))

PROTO_DEPS := $(PROTO_DIR)/rdb_protocol/ql2.pb.h $(PROTO_DIR)/rdb_protocol/ql2.pb.cc

WEB_ASSETS_DEPS := $(BUILD_ROOT_DIR)/bundle_assets/web_assets.cc $(BUILD_ROOT_DIR)/bundle_assets/web_assets.rc

MSBUILD_FLAGS := /nologo /maxcpucount
MSBUILD_FLAGS += /p:Configuration=$(CONFIGURATION)
MSBUILD_FLAGS += /p:Platform=$(PLATFORM)
MSBUILD_FLAGS += $(if $(ALWAYS_MAKE),/t:rebuild)
ifneq (1,$(VERBOSE))
  MSBUILD_FLAGS += /verbosity:minimal
endif

$/build/$(CONFIGURATION)_$(PLATFORM)/rethinkdb.exe: $/rethinkdb.vcxproj $(SOURCES_NOUNIT) $(LIB_DEPS) $(PROTO_DEPS) $(WEB_ASSETS_DEPS)
	$P MSBUILD
	"$(MSBUILD)" $(MSBUILD_FLAGS) $<

$/build/$(CONFIGURATION)_$(PLATFORM)/rethinkdb-unittest.exe: $/rethinkdb-unittest.vcxproj $(SOURCES) $(LIB_DEPS) $(PROTO_DEPS) $(WEB_ASSETS_DEPS)
	$P MSBUILD
	"$(MSBUILD)" $(MSBUILD_FLAGS) $<

.PHONY: build-clean
build-clean:
	$P RM $(BUILD_ROOT_DIR)
	rm -rf $(BUILD_ROOT_DIR)

$(PROTO_DIR)/%.pb.h $(PROTO_DIR)/%.pb.cc: $(SOURCE_DIR)/%.proto $(PROTOC_BIN_DEP) | $(PROTO_DIR)/.
	$P PROTOC
	+rm -f $(PROTO_DIR)/$*.pb.h $(PROTO_DIR)/$*.pb.cc
	$(PROTOC) --proto_path="$(shell cygpath -w '$(SOURCE_DIR)')" --cpp_out "$(shell cygpath -w '$(PROTO_DIR)')" $<
