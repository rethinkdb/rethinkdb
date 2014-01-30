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

DIST_FILE_LIST_REL := admin bench demos docs drivers lib mk packaging scripts src test
DIST_FILE_LIST_REL += configure COPYRIGHT Makefile NOTES.md README.md

DIST_FILE_LIST := $(foreach x,$(DIST_FILE_LIST_REL),$/$x)

# Ubuntu quantal and later require nodejs-legacy.
ifeq ($(shell echo $(UBUNTU_RELEASE) | grep '^[q-zQ-Z]'),)
  NODEJS_NEW := 0
else
  NODEJS_NEW := 1
endif

ifneq (,$(UBUNTU_RELEASE))
  RETHINKDB_VERSION_DEB := $(RETHINKDB_VERSION)-$(PACKAGE_BUILD_NUMBER)ubuntu1~$(UBUNTU_RELEASE)
else
  RETHINKDB_VERSION_DEB := $(RETHINKDB_VERSION)-$(PACKAGE_BUILD_NUMBER)
endif

.PHONY: prepare_deb_package_dirs
prepare_deb_package_dirs:
	$P MKDIR $(DEB_PACKAGE_DIR) $(DEB_CONTROL_ROOT)
	mkdir -p $(DEB_PACKAGE_DIR)
	mkdir -p $(DEB_CONTROL_ROOT)

DSC_CONFIGURE_DEFAULT += --prefix=/usr --sysconfdir=/etc --localstatedir=/var

ifeq ($(BUILD_PORTABLE),1)
  DIST_SUPPORT_PACKAGES := v8 protobuf gperftools libunwind re2 gtest

  DIST_CUSTOM_MK_LINES :=
  ifneq ($(CWD),$(TOP))
    DIST_CUSTOM_LINES = $(error Portable packages need to be built from '$(TOP)')
  endif
  DIST_CUSTOM_MK_LINES += 'BUILD_PORTABLE := 1'
  DIST_CONFIGURE_DEFAULT += --fetch v8 --fetch protobuf --fetch gperftools
else
  DIST_SUPPORT_PACKAGES := re2 gtest
  DIST_CUSTOM_MK_LINES :=
endif

DIST_SUPPORT = $(foreach pkg, $(DIST_SUPPORT_PACKAGES), $(SUPPORT_SRC_DIR)/$(pkg)_$($(pkg)_VERSION))

DEB_BUILD_DEPENDS := g++, libboost-dev, libssl-dev, curl, exuberant-ctags, m4, debhelper
DEB_BUILD_DEPENDS += , fakeroot, python, libncurses5-dev
ifneq ($(shell echo $(UBUNTU_RELEASE) | grep '^[q-zQ-Z]'),)
  DEB_BUILD_DEPENDS += , nodejs-legacy
endif
ifneq (1,$(STATIC_V8))
  ifeq ($(UBUNTU_RELEASE),saucy)
    DEB_BUILD_DEPENDS += , libv8-3.14-dev
  else
    DEB_BUILD_DEPENDS += , libv8-dev
  endif
endif
ifneq (1,$(BUILD_PORTABLE))
  DEB_BUILD_DEPENDS += , protobuf-compiler, protobuf-c-compiler, libprotobuf-dev
  DEB_BUILD_DEPENDS += , libprotobuf-c0-dev, libprotoc-dev, npm, libgoogle-perftools-dev
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
install-osx: install-binaries install-web

.PHONY: build-osx
build-osx: DESTDIR = $(OSX_PACKAGE_DIR)/pkg
build-osx: install-osx
	mkdir -p $(OSX_PACKAGE_DIR)/install
	pkgbuild --root $(OSX_PACKAGE_DIR)/pkg --identifier rethinkdb $(OSX_PACKAGE_DIR)/install/rethinkdb.pkg
	mkdir $(OSX_PACKAGE_DIR)/dmg
	productbuild --distribution $(OSX_PACKAGING_DIR)/Distribution.xml --package-path $(OSX_PACKAGE_DIR)/install/ $(OSX_PACKAGE_DIR)/dmg/rethinkdb-$(RETHINKDB_VERSION).pkg
# TODO: the PREFIX should not be hardcoded in the uninstall script
	cp $(OSX_PACKAGING_DIR)/uninstall-rethinkdb.sh $(OSX_PACKAGE_DIR)/dmg/uninstall-rethinkdb.sh
	chmod +x $(OSX_PACKAGE_DIR)/dmg/uninstall-rethinkdb.sh
	cp $(TOP)/NOTES.md $(OSX_PACKAGE_DIR)/dmg/
	cp $(TOP)/COPYRIGHT $(OSX_PACKAGE_DIR)/dmg/
	hdiutil create -volname RethinkDB-$(RETHINKDB_VERSION) -srcfolder $(OSX_PACKAGE_DIR)/dmg -ov $(OSX_PACKAGE_DIR)/rethinkdb.dmg

##### Source distribution

.PHONY: reset-dist-dir
reset-dist-dir: FORCE | web-assets
	$P CP $(DIST_FILE_LIST) $(DIST_DIR)
	rm -rf $(DIST_DIR)
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

$(DIST_DIR)/precompiled/web: web-assets | reset-dist-dir
	$P CP $(WEB_ASSETS_BUILD_DIR) $@
	mkdir -p $(DIST_DIR)/precompiled
	rm -rf $@
	cp -pRP $(WEB_ASSETS_BUILD_DIR) $@

$(DIST_DIR)/VERSION.OVERRIDE: FORCE | reset-dist-dir
	$P ECHO "> $@"
	echo -n $(RETHINKDB_CODE_VERSION) > $@

.PHONY: dist-dir
dist-dir: reset-dist-dir $(DIST_DIR)/custom.mk $(DIST_DIR)/precompiled/web
dist-dir: $(DIST_DIR)/VERSION.OVERRIDE $(DIST_SUPPORT) $(DIST_DIR)/configure.default
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
