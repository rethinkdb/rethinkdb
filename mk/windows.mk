# Windows build

ifeq (1,$(DEBUG))
  CONFIGURATION := Debug
else
  CONFIGURATION := Release
endif

$(TOP)/rethinkdb.vcxproj.xml:
	test -e $@ || sed "s/RETHINKDB_VERSION/`$(TOP)/scripts/gen-version.sh`/" < $(TOP)/mk/rethinkdb.vcxproj.xml > $@

$(TOP)/%.vcxproj $(TOP)/%-unittest.vcxproj: $(TOP)/%.vcxproj.xml $(TOP)/mk/%.vcxproj.xsl
	$P GEN
	cscript /nologo $(TOP)/mk/gen-vs-project.js

.PHONY: windows-all
windows-all: $(TOP)/build/$(CONFIGURATION)_$(PLATFORM)/rethinkdb.exe
ifeq (1,$(DEBUG))
  windows-all: $(TOP)/build/$(CONFIGURATION)_$(PLATFORM)/rethinkdb-unittest.exe
endif

SOURCES := $(shell find $(TOP)/src \( -name '*.cc' -or -name '*.hpp' -or -name '*.tcc' \) -and -not -name '\.*')

SOURCES_NOUNIT := $(filter-out $(TOP)/src/unittest/%,$(SOURCES))

LIB_DEPS := $(foreach dep, $(FETCH_LIST), $(SUPPORT_BUILD_DIR)/$(dep)_$($(dep)_VERSION)/$(INSTALL_WITNESS))

PROTO_DEPS := $(PROTO_DIR)/rdb_protocol/ql2.pb.h $(PROTO_DIR)/rdb_protocol/ql2.pb.cc

MSBUILD_FLAGS := /nologo /maxcpucount
MSBUILD_FLAGS += /p:Configuration=$(CONFIGURATION)
MSBUILD_FLAGS += /p:Platform=$(PLATFORM)
MSBUILD_FLAGS += $(if $(ALWAYS_MAKE),/t:rebuild)
ifneq (1,$(VERBOSE))
  MSBUILD_FLAGS += /verbosity:minimal
endif

$(TOP)/build/$(CONFIGURATION)_$(PLATFORM)/rethinkdb.exe: $(TOP)/rethinkdb.vcxproj $(SOURCES_NOUNIT) $(LIB_DEPS) $(PROTO_DEPS)
	$P MSBUILD
	"$(MSBUILD)" $(MSBUILD_FLAGS) $<

$(TOP)/build/$(CONFIGURATION)_$(PLATFORM)/rethinkdb-unittest.exe: $(TOP)/rethinkdb-unittest.vcxproj $(SOURCES) $(LIB_DEPS) $(PROTO_DEPS)
	$P MSBUILD
	"$(MSBUILD)" $(MSBUILD_FLAGS) $<

.PHONY: build-clean
build-clean:
	$P RM $(BUILD_ROOT_DIR)
	rm -rf $(BUILD_ROOT_DIR)

$(PROTO_DIR)/%.pb.h $(PROTO_DIR)/%.pb.cc: $(TOP)/src/%.proto $(PROTOC_BIN_DEP) | $(PROTO_DIR)/.
	$P PROTOC
	+rm -f $(PROTO_DIR)/$*.pb.h $(PROTO_DIR)/$*.pb.cc
	$(PROTOC) --proto_path="$(shell cygpath -w '$(TOP)/src')" --cpp_out "$(shell cygpath -w '$(PROTO_DIR)')" $<
