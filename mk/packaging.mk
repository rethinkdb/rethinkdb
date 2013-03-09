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

DIST_FILE_LIST_REL := admin assets bench demos docs docs_internal drivers external lib mk packaging scripts src test
DIST_FILE_LIST_REL += configure COPYRIGHT DEPENDENCIES Makefile NOTES README README.md

DIST_FILE_LIST := $(foreach x,$(DIST_FILE_LIST_REL),$/$x)

# Ubuntu quantal and later require nodejs-legacy.
ifeq ($(shell echo $(UBUNTU_RELEASE) | grep '^[q-zQ-Z]'),)
  NODEJS_NEW := 0
else
  NODEJS_NEW := 1
endif

# We have facilities for converting between toy names and Debian release numbers.

deb-release-numbers := 1:buzz 2:rex 3:bo 4:hamm 5:slink 6:potato 7:woody 8:sarge 9:etch 10:lenny 11:squeeze 12:wheezy 13:jessie
deb-num-to-toy = $(patsubst $1:%,%,$(filter $1:%,$(deb-release-numbers)))
deb-toy-to-num = $(patsubst %:$1,%,$(filter %:$1,$(deb-release-numbers)))

# We can accept a toy name via DEB_RELEASE or a number via DEB_NUM_RELEASE.
# Note that the numeric release does not correspond to the Debian version but to the sequence of major releases. Version 1.3 is number 3, and version 2 is number 4.
# DEB_REAL_NUM_RELEASE is a scrubbed version of DEB_NUM_RELEASE.

DEB_RELEASE_NUM := $(call deb-toy-to-num,$(DEB_RELEASE))

ifneq ($(DEB_RELEASE),)
  ifeq ($(DEB_RELEASE_NUM),)
    $(warning The Debian version specification is invalid. We will ignore it.)
  endif
  ifneq ($(UBUNTU_RELEASE),)
    $(warning We seem to have received an Ubuntu release specification and a Debian release specification. We will ignore the Debian release specification.)
  endif
endif

DEB_PACKAGE_REVISION := $(shell env UBUNTU_RELEASE="$(UBUNTU_RELEASE)" DEB_RELEASE="$(DEB_RELEASE)" DEB_RELEASE_NUM="$(DEB_RELEASE_NUM)" PACKAGE_VERSION="" $(TOP)/scripts/gen-trailer.sh)
RETHINKDB_VERSION_DEB := $(RETHINKDB_PACKAGING_VERSION)$(DEB_PACKAGE_REVISION)

.PHONY: prepare_deb_package_dirs
prepare_deb_package_dirs:
	$P MKDIR $(DEB_PACKAGE_DIR) $(DEB_CONTROL_ROOT)
	mkdir -p $(DEB_PACKAGE_DIR)
	mkdir -p $(DEB_CONTROL_ROOT)

.PHONY: install-deb
install-deb: DESTDIR = $(DEB_PACKAGE_DIR)
install-deb: install
	$P INSTALL $(DESTDIR)$(doc_dir)/changelog.Debian.gz
	install -m755 -d $(DESTDIR)$(doc_dir)
	sed -e 's/PACKAGING_VERSION/$(RETHINKDB_VERSION_DEB)/' $(ASSETS_DIR)/docs/changelog.Debian | \
	  gzip -c9 | \
	  install -m644 -T /dev/stdin $(DESTDIR)$(doc_dir)/changelog.Debian.gz

DSC_CUSTOM_MK_LINES := 'CONFIGURE_FLAGS += --disable-drivers'
DSC_CUSTOM_MK_LINES += 'CONFIGURE_FLAGS += --prefix=/usr'
DSC_CUSTOM_MK_LINES += 'CONFIGURE_FLAGS += --sysconfdir=/etc'
DSC_CUSTOM_MK_LINES += 'CONFIGURE_FLAGS += --localstatedir=/var'

ifeq ($(BUILD_PORTABLE),1)
  DIST_SUPPORT := $(V8_SRC_DIR) $(PROTOC_SRC_DIR) $(GPERFTOOLS_SRC_DIR) $(LIBUNWIND_SRC_DIR)

  DIST_CUSTOM_MK_LINES :=
  ifneq ($(CWD),$(TOP))
    DIST_CUSTOM_LINES = $(error Portable packages need to be built from '$(TOP)')
  endif
  DIST_CUSTOM_MK_LINES += 'BUILD_PORTABLE := 1'
  DIST_CUSTOM_MK_LINES += 'CONFIGURE_FLAGS += V8=$(V8_INT_LIB)'
  DIST_CUSTOM_MK_LINES += 'CONFIGURE_FLAGS += PROTOC=$(TC_PROTOC_INT_EXE)'
  DIST_CUSTOM_MK_LINES += 'CONFIGURE_FLAGS += PROTOBUF=$(PROTOBUF_INT_LIB)'
  DIST_CUSTOM_MK_LINES += 'CONFIGURE_FLAGS += TCMALLOC_MINIMAL=$(TCMALLOC_MINIMAL_INT_LIB)'
  DIST_CUSTOM_MK_LINES += 'CONFIGURE_FLAGS += UNWIND=$(UNWIND_INT_LIB)'
else
  DIST_SUPPORT :=
  DIST_CUSTOM_MK_LINES :=
endif

.PHONY: build-deb-src
build-deb-src: deb-src-dir build-deb-src-control
	$P DEBUILD ""
	cd $(DSC_PACKAGE_DIR) && yes | debuild -S -sa

.PHONY: deb-src-dir
deb-src-dir: dist-dir
	$P MV $(DIST_DIR) $(DSC_PACKAGE_DIR)
	rm -rf $(DSC_PACKAGE_DIR)
	mv $(DIST_DIR) $(DSC_PACKAGE_DIR)
	for line in $(DSC_CUSTOM_MK_LINES); do \
	  echo "$$line" >> $(DSC_PACKAGE_DIR)/custom.mk ; \
	done

.PHONY: build-deb-src-control
build-deb-src-control: | deb-src-dir
	$P CP $(PACKAGING_DIR)/debian.template $(DSC_PACKAGE_DIR)/debian
	cp -pRP $(PACKAGING_DIR)/debian.template $(DSC_PACKAGE_DIR)/debian
	env UBUNTU_RELEASE=$(UBUNTU_RELEASE) \
	    DEB_RELEASE=$(DEB_RELEASE) \
	    DEB_RELEASE_NUM=$(DEB_RELEASE_NUM) \
	    VERSIONED_QUALIFIED_PACKAGE_NAME=$(VERSIONED_QUALIFIED_PACKAGE_NAME) \
	    PACKAGE_VERSION=$(RETHINKDB_PACKAGING_VERSION) \
	  $(TOP)/scripts/gen-changelog.sh \
	  > $(DSC_PACKAGE_DIR)/debian/changelog
	$P M4 $(DEBIAN_PKG_DIR)/control $(DSC_PACKAGE_DIR)/debian/control
	env disk_size=0 \
	  m4 -D "PACKAGE_NAME=$(PACKAGE_NAME)" \
	     -D "VERSIONED_PACKAGE_NAME=$(VERSIONED_PACKAGE_NAME)" \
	     -D "VANILLA_PACKAGE_NAME=$(VANILLA_PACKAGE_NAME)" \
	     -D "VERSIONED_QUALIFIED_PACKAGE_NAME=$(VERSIONED_QUALIFIED_PACKAGE_NAME)" \
	     -D "PACKAGE_VERSION=$(RETHINKDB_VERSION_DEB)" \
	     -D "LEGACY_PACKAGE=$(LEGACY_PACKAGE)" \
	     -D "STATIC_V8=$(STATIC_V8)" \
	     -D "TC_BUNDLED=$(BUILD_PORTABLE)" \
	     -D "BUILD_DRIVERS=$(BUILD_DRIVERS)" \
	     -D "DISK_SIZE=$${disk_size}" \
	     -D "SOURCEBUILD=1" \
	     -D "NODEJS_NEW=$(NODEJS_NEW)" \
	     -D "CURRENT_ARCH=$(DEB_ARCH)" \
	     $(DEBIAN_PKG_DIR)/control > $(DSC_PACKAGE_DIR)/debian/control
	$P M4 preinst postinst prerm postrm "->" $(DSC_PACKAGE_DIR)/debian
	for script in preinst postinst prerm postrm; do \
	  m4 -D "BIN_DIR=$(bin_dir)" \
	     -D "MAN1_DIR=$(man1_dir)" \
	     -D "BASH_COMPLETION_DIR=$(bash_completion_dir)" \
	     -D "INTERNAL_BASH_COMPLETION_DIR=$(internal_bash_completion_dir)" \
	     -D "SERVER_EXEC_NAME=$(SERVER_EXEC_NAME)" \
	     -D "SERVER_EXEC_NAME_VERSIONED=$(SERVER_EXEC_NAME_VERSIONED)" \
	     -D "UPDATE_ALTERNATIVES=$(NAMEVERSIONED)" \
	     -D "PRIORITY=$(PACKAGING_ALTERNATIVES_PRIORITY)" \
	     $(DEBIAN_PKG_DIR)/$${script} > $(DSC_PACKAGE_DIR)/debian/$${script}; \
	  chmod 0755 $(DSC_PACKAGE_DIR)/debian/$${script}; \
	done
	$P CAT $(DEBIAN_PKG_DIR)/copyright ">" $(DSC_PACKAGE_DIR)/debian/copyright
	cat $(DEBIAN_PKG_DIR)/copyright > $(DSC_PACKAGE_DIR)/debian/copyright

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

.PHONY: deb
deb: prepare_deb_package_dirs
	$P MAKE WAY=deb
	$(MAKE) WAY=deb
	for deb_name in $(PACKAGES_DIR)/*.deb; do \
	  if [ $(LINTIAN) = 1]; then \
	    lintian --color auto --suppress-tags "no-copyright-file,$(subst $(space),$(comma),$(SUPPRESSED_LINTIAN_TAGS))" $${deb_name} || true ; \
	  fi \
	done
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
	hdiutil create -volname RethinkDB -srcfolder $(OSX_PACKAGE_DIR)/dmg -ov $(OSX_PACKAGE_DIR)/rethinkdb.dmg

.PHONY: osx
osx:
	$(MAKE) WAY=osx

##### Source distribution

.PHONY: reset-dist-dir
reset-dist-dir: FORCE | web-assets
	$P CP $(DIST_FILE_LIST) $(DIST_DIR)
# TODO: have a seperate external-clean phony target
	rm -rf $(PROTOC_JS_PLUGIN)
	$(EXTERN_MAKE) -C $(TOP)/external/gtest-1.6.0/make clean
	$(EXTERN_MAKE) -C $(TOP)/external/protobuf-plugin-closure clean
	rm -rf $(DIST_DIR)
	mkdir -p $(DIST_DIR)
	cp -pRP $(DIST_FILE_LIST) $(DIST_DIR)

$(DIST_DIR)/custom.mk: FORCE | reset-dist-dir
	$P ECHO "> $@"
	echo 'CONFIGURE_FLAGS += --enable-precompiled-web' > $@
	for line in $(DIST_CUSTOM_MK_LINES); do \
	  echo "$$line" >> $(DIST_DIR)/custom.mk ; \
	done

$(DIST_DIR)/precompiled/web: web-assets | reset-dist-dir
	$P CP $(WEB_ASSETS_BUILD_DIR) $@
	mkdir -p $(DIST_DIR)/precompiled
	rm -rf $@
	cp -pRP $(WEB_ASSETS_BUILD_DIR) $@

$(DIST_DIR)/VERSION.OVERRIDE: FORCE | reset-dist-dir
	$P ECHO "> $@"
	echo -n $(RETHINKDB_CODE_VERSION) > $@

.PHONY: dist-dir
dist-dir: reset-dist-dir $(DIST_DIR)/custom.mk $(DIST_DIR)/precompiled/web $(DIST_DIR)/VERSION.OVERRIDE $(DIST_SUPPORT)
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
