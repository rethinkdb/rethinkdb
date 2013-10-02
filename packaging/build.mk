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

DIST_FILE_LIST_REL := admin bench demos docs drivers external lib mk packaging scripts src test
DIST_FILE_LIST_REL += configure COPYRIGHT DEPENDENCIES Makefile NOTES README.md

DIST_FILE_LIST := $(foreach x,$(DIST_FILE_LIST_REL),$/$x)

# Ubuntu quantal and later require nodejs-legacy.
ifeq ($(shell echo $(UBUNTU_RELEASE) | grep '^[q-zQ-Z]'),)
  NODEJS_NEW := 0
else
  NODEJS_NEW := 1
endif

RETHINKDB_VERSION_DEB := $(RETHINKDB_PACKAGING_VERSION)ubuntu1~$(UBUNTU_RELEASE)

.PHONY: prepare_deb_package_dirs
prepare_deb_package_dirs:
	$P MKDIR $(DEB_PACKAGE_DIR) $(DEB_CONTROL_ROOT)
	mkdir -p $(DEB_PACKAGE_DIR)
	mkdir -p $(DEB_CONTROL_ROOT)

.PHONY: install-deb
install-deb: DESTDIR = $(DEB_PACKAGE_DIR)
install-deb: install install-deb-changelog

.PHONY: install-deb-changelog
install-deb-changelog:
	$P INSTALL $(DESTDIR)$(doc_dir)/changelog.Debian.gz
	install -m755 -d $(DESTDIR)$(doc_dir)
	sed -e 's/PACKAGING_VERSION/$(RETHINKDB_VERSION_DEB)/' $(ASSETS_DIR)/docs/changelog.Debian | \
	  gzip -c9 | \
	  install -m644 -T /dev/stdin $(DESTDIR)$(doc_dir)/changelog.Debian.gz

DSC_CONFIGURE_DEFAULT += --prefix=/usr --sysconfdir=/etc --localstatedir=/var

ifeq ($(BUILD_PORTABLE),1)
  DIST_SUPPORT := $(V8_SRC_DIR) $(PROTOC_SRC_DIR) $(GPERFTOOLS_SRC_DIR) $(LIBUNWIND_SRC_DIR)

  DIST_CUSTOM_MK_LINES :=
  ifneq ($(CWD),$(TOP))
    DIST_CUSTOM_LINES = $(error Portable packages need to be built from '$(TOP)')
  endif
  DIST_CUSTOM_MK_LINES += 'BUILD_PORTABLE := 1'
  DIST_CONFIGURE_DEFAULT += --fetch v8 --fetch protoc --fetch tcmalloc_minimal
else
  DIST_SUPPORT :=
  DIST_CUSTOM_MK_LINES :=
endif

BASE_DEPENDS := g++, libboost-dev, libssl-dev, curl, exuberant-ctags, m4, debhelper,
BASE_DEPENDS += fakeroot, python, libncurses5-dev
NODEJS_DEPENDS_EXTRA := ifelse(NODEJS_NEW,1,`, nodejs-legacy',`')')dnl
# define(`V8_DEPENDS_EXTRA',`ifelse(STATIC_V8,0,`, libv8-dev',`')')dnl
# define(`PROTOC_DEPENDS_EXTRA',`ifelse(TC_BUNDLED,0,`, protobuf-compiler, protobuf-c-compiler, libprotobuf-dev, libprotobuf-c0-dev, libprotoc-dev',`')')dnl
# define(`NODE_DEPENDS_EXTRA',`ifelse(TC_BUNDLED,0,`, npm',`')')dnl
# define(`GPERFTOOLS_DEPENDS_EXTRA',`ifelse(TC_BUNDLED,0,`, libgoogle-perftools-dev',`')')dnl
# define(`RUBY_DEPENDS_EXTRA',`ifelse(BUILD_DRIVERS,0,`, ruby, rubygems',`')')dnl
DEB_BUILD_DEPENDS := $(BASE_DEPENDS) $(NODEJS_DEPENDS_EXTRA} $(V8_DEPENDS_EXTRA)
DEB_BUILD_DEPENDS += $(PROTOC_DEPENDS_EXTRA) $(NODE_DEPENDS_EXTRA)
DEB_BUILD_DEPENDS += $(GPERFTOOLS_DEPENDS_EXTRA) $(RUBY_DEPENDS_EXTRA)

# TODO:
# DEB_DEPENDS = libncurses5, ifelse(LEGACY_PACKAGE, 1,
#   `libc6 (>= 2.5), libstdc++6 (>= 4.4), libgcc1',
#   `libc6 (>= 2.10.1), libstdc++6 (>= 4.6), libgcc1, libprotobuf7')`'dnl
# ifelse(STATIC_V8, 0, `, libv8-dev (>= 3.1)', `')

.PHONY: build-deb-src
build-deb-src: deb-src-dir
	$P DEBUILD ""
	cd $(DSC_PACKAGE_DIR) && yes | debuild -S -sa

.PHONY: deb-src-dir
deb-src-dir: dist-dir
	$P MV $(DIST_DIR) $(DSC_PACKAGE_DIR)
	rm -rf $(DSC_PACKAGE_DIR)
	mv $(DIST_DIR) $(DSC_PACKAGE_DIR)
	echo $(DSC_CONFIGURE_DEFAULT) >> $(DSC_PACKAGE_DIR)/configure.default
	$P CP $(PACKAGING_DIR)/debian $(DSC_PACKAGE_DIR)/debian
	cp -pRP $(PACKAGING_DIR)/debian $(DSC_PACKAGE_DIR)/debian
	env UBUNTU_RELEASE=$(UBUNTU_RELEASE) \
	    DEB_RELEASE=$(DEB_RELEASE) \
	    DEB_RELEASE_NUM=$(DEB_RELEASE_NUM) \
	    VERSIONED_QUALIFIED_PACKAGE_NAME=$(VERSIONED_QUALIFIED_PACKAGE_NAME) \
	    PACKAGE_VERSION=$(RETHINKDB_PACKAGING_VERSION) \
	  $(TOP)/scripts/gen-changelog.sh \
	  > $(DSC_PACKAGE_DIR)/debian/changelog
	$P ECHO $(DSC_PACKAGE_DIR)/debian/rethinkdb.substvars
	( echo "PACKAGE_NAME=$(PACKAGE_NAME)" \
	  echo "PACKAGE_VERSION=$(RETHINKDB_VERSION_DEB)" \
	  echo "DEB_BUILD_DEPENDS=$(DEB_BUILD_DEPENDS)" \
	  echo "DEB_DEPENDS=$(DEB_DEPENDS)" \
	) > $(DSC_PACKAGE_DIR)/debian/rethinkdb.substvars
	$P SED $(DSC_PACKAGE_DIR)/debian/control
	sed "s/VERSIONED_QUALIFIED_PACKAGE_NAME/$(VERSIONED_QUALIFIED_PACKAGE_NAME)/" \
	  $(DSC_PACKAGE_DIR)/debian/control.in > $(DSC_PACKAGE_DIR)/debian/control
	rm $(DSC_PACKAGE_DIR)/debian/control.in

.PHONY: build-deb
build-deb: all prepare_deb_package_dirs install-deb
	$P MD5SUMS $(DEB_PACKAGE_DIR)
	find $(DEB_PACKAGE_DIR) -path $(DEB_CONTROL_ROOT) -prune -o -path $(DEB_PACKAGE_DIR)/etc -prune -o -type f -printf "%P\\0" | \
	   (cd $(DEB_PACKAGE_DIR) && xargs -0 md5sum) > $(DEB_CONTROL_ROOT)/md5sums
	$P FIND $(DEB_CONTROL_ROOT)/conffiles
	find $(DEB_PACKAGE_DIR) -type f -printf "/%P\n" | (grep '^/etc/' | grep -v '^/etc/init\.d' || true) > $(DEB_CONTROL_ROOT)/conffiles
	$P M4 preinst postinst prerm postrm $(DEB_CONTROL_ROOT)
	for script in preinst postinst prerm postrm; do \
	  m4 -D "BIN_DIR=$(bin_dir)" \
	     -D "MAN1_DIR=$(man1_dir)" \
	     -D "BASH_COMPLETION_DIR=$(bash_completion_dir)" \
	     -D "INTERNAL_BASH_COMPLETION_DIR=$(internal_bash_completion_dir)" \
	     -D "SERVER_EXEC_NAME=$(SERVER_EXEC_NAME)" \
	     -D "SERVER_EXEC_NAME_VERSIONED=$(SERVER_EXEC_NAME_VERSIONED)" \
	     -D "UPDATE_ALTERNATIVES=$(NAMEVERSIONED)" \
	     -D "PRIORITY=$(PACKAGING_ALTERNATIVES_PRIORITY)" \
	     $(DEBIAN_PKG_DIR)/$${script} > $(DEB_CONTROL_ROOT)/$${script}; \
	  chmod 0755 $(DEB_CONTROL_ROOT)/$${script}; \
	done
	$P M4 $(DEBIAN_PKG_DIR)/control $(DEB_CONTROL_ROOT)/control
	env disk_size=$$(du -s -k $(DEB_PACKAGE_DIR) | cut -f1); \
	  m4 -D "PACKAGE_NAME=$(PACKAGE_NAME)" \
	     -D "VERSIONED_PACKAGE_NAME=$(VERSIONED_PACKAGE_NAME)" \
	     -D "VANILLA_PACKAGE_NAME=$(VANILLA_PACKAGE_NAME)" \
	     -D "PACKAGE_VERSION=$(RETHINKDB_VERSION_DEB)" \
	     -D "VERSIONED_QUALIFIED_PACKAGE_NAME=$(VERSIONED_QUALIFIED_PACKAGE_NAME)" \
	     -D "LEGACY_PACKAGE=$(LEGACY_PACKAGE)" \
	     -D "STATIC_V8=$(STATIC_V8)" \
	     -D "TC_BUNDLED=$(BUILD_PORTABLE)" \
	     -D "BUILD_DRIVERS=$(BUILD_DRIVERS)" \
	     -D "DISK_SIZE=$${disk_size}" \
	     -D "SOURCEBUILD=0" \
	     -D "NODEJS_NEW=$(NODEJS_NEW)" \
	     -D "CURRENT_ARCH=$(DEB_ARCH)" \
	     $(DEBIAN_PKG_DIR)/control > $(DEB_CONTROL_ROOT)/control
	$P CP $(DEBIAN_PKG_DIR)/copyright $(DEB_CONTROL_ROOT)/copyright
	cat $(DEBIAN_PKG_DIR)/copyright > $(DEB_CONTROL_ROOT)/copyright
	$P DPKG-DEB $(DEB_PACKAGE_DIR) $(PACKAGES_DIR)
	fakeroot dpkg-deb -b $(DEB_PACKAGE_DIR) $(PACKAGES_DIR)

.PHONY: install-osx
install-osx: install-binaries install-web

.PHONY: build-osx
build-osx: DESTDIR = $(OSX_PACKAGE_DIR)/pkg
build-osx: install-osx
	mkdir -p $(OSX_PACKAGE_DIR)/install
	pkgbuild --root $(OSX_PACKAGE_DIR)/pkg --identifier rethinkdb $(OSX_PACKAGE_DIR)/install/rethinkdb.pkg
	mkdir $(OSX_PACKAGE_DIR)/dmg
	productbuild --distribution $(OSX_PACKAGING_DIR)/Distribution.xml --package-path $(OSX_PACKAGE_DIR)/install/ $(OSX_PACKAGE_DIR)/dmg/rethinkdb.pkg
# TODO: the PREFIX should not be hardcoded in the uninstall script
	cp $(OSX_PACKAGING_DIR)/uninstall-rethinkdb.sh $(OSX_PACKAGE_DIR)/dmg/uninstall-rethinkdb.sh
	chmod +x $(OSX_PACKAGE_DIR)/dmg/uninstall-rethinkdb.sh
	cp $(TOP)/NOTES $(OSX_PACKAGE_DIR)/dmg/
	hdiutil create -volname RethinkDB -srcfolder $(OSX_PACKAGE_DIR)/dmg -ov $(OSX_PACKAGE_DIR)/rethinkdb.dmg

##### Source distribution

.PHONY: reset-dist-dir
reset-dist-dir: FORCE | web-assets
	$P CP $(DIST_FILE_LIST) $(DIST_DIR)
	rm -rf $(PROTOC_JS_PLUGIN)
	$(EXTERN_MAKE) -C $(TOP)/external/gtest/make clean
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
	$P CP $(DIST_SUPPORT) "->" $(DIST_DIR)
	$(foreach path,$(DIST_SUPPORT), \
	  $(foreach dir,$(DIST_DIR)/support/$(patsubst $(SUPPORT_DIR)/%,%,$(dir $(path))), \
	    mkdir -p $(dir) $(newline) \
	    cp -pPR $(path) $(dir) $(newline) ))

$(DIST_PACKAGE_TGZ): dist-dir
	$P TAR $@ $(DIST_DIR)
	cd $(dir $(DIST_DIR)) && tar zfc $(notdir $@) $(notdir $(DIST_DIR))

.PHONY: dist
dist: $(DIST_PACKAGE_TGZ)
