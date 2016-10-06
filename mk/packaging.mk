##### Packaging

DIST_DIR := $(PACKAGES_DIR)/$(PACKAGE_NAME)-$(RETHINKDB_VERSION)
DIST_PACKAGE_TGZ := $(PACKAGES_DIR)/$(PACKAGE_NAME)-$(RETHINKDB_VERSION).tgz

DSC_PACKAGE_DIR := $(PACKAGES_DIR)/dsc
DEB_PACKAGE_DIR := $(PACKAGES_DIR)/deb
OSX_PACKAGE_DIR := $(PACKAGES_DIR)/osx
OSX_PACKAGING_DIR := $(PACKAGING_DIR)/osx

DEBIAN_PKG_DIR := $(PACKAGING_DIR)/debian
SUPPRESSED_LINTIAN_TAGS := new-package-should-close-itp-bug
DEB_CONTROL_ROOT := $(DEB_PACKAGE_DIR)/DEBIAN

DIST_FILE_LIST_REL := admin demos drivers mk packaging scripts src test
DIST_FILE_LIST_REL += configure COPYRIGHT Makefile NOTES.md README.md

DIST_FILE_LIST := $(foreach x,$(DIST_FILE_LIST_REL),$/$x)

RETHINKDB_VERSION_DEB := $(subst -,+,$(RETHINKDB_VERSION))~$(PACKAGE_BUILD_NUMBER)$(UBUNTU_RELEASE)$(DEB_RELEASE)

.PHONY: prepare_deb_package_dirs
prepare_deb_package_dirs:
	$P MKDIR $(DEB_PACKAGE_DIR) $(DEB_CONTROL_ROOT)
	mkdir -p $(DEB_PACKAGE_DIR)
	mkdir -p $(DEB_CONTROL_ROOT)

DIST_SUPPORT_PACKAGES := re2 gtest v8 jemalloc
DIST_CUSTOM_MK_LINES :=
ifeq ($(BUILD_PORTABLE),1)
  DIST_SUPPORT_PACKAGES += protobuf boost
  DIST_CUSTOM_MK_LINES += 'BUILD_PORTABLE := 1'

  ifneq ($(CWD),$(TOP))
    DIST_CUSTOM_MK_LINES = $(error Portable packages need to be built from '$(TOP)')
  endif
endif

MISSING_DIST_SUPPORT_PACKAGES := $(filter-out $(FETCH_LIST), $(DIST_SUPPORT_PACKAGES))
DIST_SUPPORT_PACKAGES := $(filter $(FETCH_LIST), $(DIST_SUPPORT_PACKAGES))
DSC_CONFIGURE_DEFAULT = --prefix=/usr --sysconfdir=/etc --localstatedir=/var
DIST_CONFIGURE_DEFAULT_FETCH = $(foreach pkg, $(DIST_SUPPORT_PACKAGES), --fetch $(pkg))
DIST_SUPPORT = $(foreach pkg, $(DIST_SUPPORT_PACKAGES), $(SUPPORT_SRC_DIR)/$(pkg)_$($(pkg)_VERSION))

DEB_BUILD_DEPENDS := g++, libboost-dev, libssl-dev, curl, m4, debhelper
DEB_BUILD_DEPENDS += , fakeroot, python, libncurses5-dev, libcurl4-openssl-dev, libssl-dev

ifneq (1,$(BUILD_PORTABLE))
  DEB_BUILD_DEPENDS += , protobuf-compiler, libprotobuf-dev
endif

ifeq ($(BUILD_PORTABLE),1)
  LEGACY_PACKAGE := 1
else ifeq ($(LEGACY_LINUX),1)
  LEGACY_PACKAGE := 1
else
  LEGACY_PACKAGE := 0
endif

ifneq (1,$(SIGN_PACKAGE))
  DEBUILD_SIGN_OPTIONS := -us -uc
else
  DEBUILD_SIGN_OPTIONS :=
endif

OS_RELEASE := $(if $(DEB_RELEASE),$(DEB_RELEASE),$(if $(UBUNTU_RELEASE),$(UBUNTU_RELEASE),unstable))

.PHONY: build-deb-src
build-deb-src: deb-src-dir
	$P DEBUILD ""
	cd $(DSC_PACKAGE_DIR) && yes | debuild -S -sa $(DEBUILD_SIGN_OPTIONS)

.PHONY: deb-src-dir
deb-src-dir: dist-dir
	$P MV $(DIST_DIR) $(DSC_PACKAGE_DIR)
	rm -rf $(DSC_PACKAGE_DIR)
	mv $(DIST_DIR) $(DSC_PACKAGE_DIR)
	echo $(DSC_CONFIGURE_DEFAULT) >> $(DSC_PACKAGE_DIR)/configure.default
	$P CP $(PACKAGING_DIR)/debian $(DSC_PACKAGE_DIR)/debian
	cp -pRP $(PACKAGING_DIR)/debian $(DSC_PACKAGE_DIR)/debian
	env PRODUCT_NAME=$(VERSIONED_QUALIFIED_PACKAGE_NAME) \
	    PRODUCT_VERSION=$(RETHINKDB_VERSION_DEB) \
	    OS_RELEASE=$(OS_RELEASE) \
	  $(TOP)/scripts/gen-changelog.sh \
	  > $(DSC_PACKAGE_DIR)/debian/changelog
	$P ECHO $(DSC_PACKAGE_DIR)/debian/rethinkdb.version
	echo $(RETHINKDB_VERSION_DEB) > $(DSC_PACKAGE_DIR)/debian/rethinkdb.version
	$P M4 $(DSC_PACKAGE_DIR)/debian/control
	m4 -D "PACKAGE_NAME=$(PACKAGE_NAME)" \
	   -D "PACKAGE_VERSION=$(RETHINKDB_VERSION_DEB)" \
	   -D "DEB_BUILD_DEPENDS=$(DEB_BUILD_DEPENDS)" \
	   -D "VERSIONED_QUALIFIED_PACKAGE_NAME=$(VERSIONED_QUALIFIED_PACKAGE_NAME)" \
	  $(DSC_PACKAGE_DIR)/debian/control.in \
	  > $(DSC_PACKAGE_DIR)/debian/control
	rm $(DSC_PACKAGE_DIR)/debian/control.in

.PHONY: build-deb
build-deb: deb-src-dir
	$P BUILD-DEB $(DSC_PACKAGE_DIR)
	cd $(DSC_PACKAGE_DIR) && dpkg-buildpackage -rfakeroot $(DEBUILD_SIGN_OPTIONS)

.PHONY: install-osx
install-osx: install-binaries

ifneq (Darwin,$(OS))
  OSX_DMG_BUILD = $(error MacOS package can only be built on that OS)
else ifneq ("","$(findstring $(OSX_SIGNATURE_NAME),$(shell /usr/bin/security find-identity -p macappstore -v | /usr/bin/awk '/[:blank:]+[:digit:]+[:graph:][:blank:]/'))")
  OSX_DMG_BUILD = $(TOP)/packaging/osx/create_dmg.py --server-root "$(OSX_PACKAGE_DIR)/pkg/$(bin_dir)" --install-path "$(bin_dir)" --ouptut-location "$(OSX_PACKAGE_DIR)/rethinkdb.dmg" --signing-name "$(OSX_SIGNATURE_NAME)"
else ifeq ($(REQUIRE_SIGNED),1)
  OSX_DMG_BUILD = $(error Certificate not found: $(OSX_SIGNATURE_NAME))
else
  OSX_DMG_BUILD = $(TOP)/packaging/osx/create_dmg.py --server-root "$(OSX_PACKAGE_DIR)/pkg/$(bin_dir)" --install-path "$(bin_dir)" --ouptut-location "$(OSX_PACKAGE_DIR)/rethinkdb.dmg"
endif

.PHONY: build-osx
build-osx: DESTDIR = $(OSX_PACKAGE_DIR)/pkg
build-osx: SPLIT_SYMBOLS = 1
build-osx: install-osx
	set -e; /usr/bin/otool -L $(OSX_PACKAGE_DIR)/pkg/$(FULL_SERVER_EXEC_NAME) | \
		awk '/^\t/ { sub(/ \(.+\)/, ""); print }' | while read LINE; do \
			case "$$LINE" in $(BUILD_DIR_ABS)/*) \
				echo '***' rethinkdb binary links to non-system dylib: $$LINE; \
				exit 1;; \
			esac \
		done
	$P CREATE $(OSX_PACKAGE_DIR)/rethinkdb.dmg
	$(OSX_DMG_BUILD)

##### Source distribution

.PHONY: clean-dist-dir
clean-dist-dir:
	$P RM $(DIST_DIR)
	rm -rf $(DIST_DIR)

.PHONY: reset-dist-dir
reset-dist-dir: FORCE | web-assets
	test ! -e $(DIST_DIR) || { echo "error: $(DIST_DIR) exists. make clean-dist-dir first"; false; }
	$P CP $(DIST_DIR)
	mkdir -p $(DIST_DIR)
	cp -pRP $(DIST_FILE_LIST) $(DIST_DIR)

$(DIST_DIR)/custom.mk: FORCE | reset-dist-dir
	$P ECHO "> $@"
	for line in $(DIST_CUSTOM_MK_LINES); do \
	  echo "$$line" >> $(DIST_DIR)/custom.mk ; \
	done

$(DIST_DIR)/configure.default: FORCE | reset-dist-dir
	$P ECHO "> $@"
	echo $(DIST_CONFIGURE_DEFAULT) >> $(DIST_DIR)/configure.default
	echo $(DIST_CONFIGURE_DEFAULT_FETCH) >> $(DIST_DIR)/configure.default

$(DIST_DIR)/precompiled/%: $(BUILD_ROOT_DIR)/% | reset-dist-dir
	$P CP
	mkdir -p $(@D)
	cp -f $< $@

$(DIST_DIR)/VERSION.OVERRIDE: FORCE | reset-dist-dir
	$P ECHO "> $@"
	echo -n $(RETHINKDB_CODE_VERSION) > $@

PRECOMPILED_ASSETS := $(DIST_DIR)/precompiled/bundle_assets/web_assets.cc
PRECOMPILED_ASSETS += $(DIST_DIR)/precompiled/proto/rdb_protocol/ql2.pb.h
PRECOMPILED_ASSETS += $(DIST_DIR)/precompiled/proto/rdb_protocol/ql2.pb.cc

.PRECIOUS: $(PRECOMPILED_ASSETS)

.PHONY: dist-dir
dist-dir: reset-dist-dir $(DIST_DIR)/custom.mk $(PRECOMPILED_ASSETS)
dist-dir: $(DIST_DIR)/VERSION.OVERRIDE $(DIST_SUPPORT) $(DIST_DIR)/configure.default
ifneq (,$(MISSING_DIST_SUPPORT_PACKAGES))
	$(error Missing source packages in external. Please configure with '$(patsubst %,--fetch %,$(MISSING_DIST_SUPPORT_PACKAGES))')
endif
	$P CP $(DIST_SUPPORT) "->" $(DIST_DIR)/external
	$(foreach path,$(DIST_SUPPORT), \
	  $(foreach dir,$(DIST_DIR)/external/$(patsubst $(SUPPORT_SRC_DIR)/%,%,$(path)), \
	    mkdir -p $(dir) $(newline) \
	    cp -pPR $(path)/. $(dir) $(newline) ))

$(DIST_PACKAGE_TGZ): dist-dir
	$P TAR $@ $(DIST_DIR)
	cd $(dir $(DIST_DIR)) && tar zfc $(notdir $@) $(notdir $(DIST_DIR))

.PHONY: dist
dist: $(DIST_PACKAGE_TGZ)
