#******************************************************************************
#
#   Copyright (C) 2010, International Business Machines Corporation and others.  All Rights Reserved.
#
#******************************************************************************
# This is to be called from ../Makefile.in
#
# This will only work if subversion is installed.

top_builddir = .

include $(top_builddir)/icudefs.mk


DISTY_TMP=dist/tmp
DISTY_ICU=$(DISTY_TMP)/icu
DISTY_DATA=$(DISTY_ICU)/source/data
DISTY_RMV=brkitr coll curr lang locales mappings rbnf region translit xml zone
DISTY_RMDIR=$(DISTY_RMV:%=$(DISTY_DATA)/%)
DISTY_IN=$(DISTY_DATA)/in
DOCZIP=icu-docs.zip

SVNTOP=$(top_srcdir)/..
SVNDOT=$(SVNTOP)/.svn
SVNVER=$(shell svnversion $(SVNTOP) | cut -d: -f1 | tr -cd 'a-zA-Z0-9')
SVNURL=$(shell svn info $(SVNTOP) | grep '^URL:' | cut -d: -f2-)
DISTY_VER=$(shell echo $(VERSION) | tr '.' '_' )
DISTY_PREFIX=icu4c
DISTY_FILE_DIR=$(shell pwd)/$(DISTY_DIR)
DISTY_FILE_TGZ=$(DISTY_FILE_DIR)/$(DISTY_PREFIX)-src-$(DISTY_VER)-r$(SVNVER).tgz
DISTY_FILE_ZIP=$(DISTY_FILE_DIR)/$(DISTY_PREFIX)-src-$(DISTY_VER)-r$(SVNVER).zip
DISTY_DOC_ZIP=$(DISTY_FILE_DIR)/$(DISTY_PREFIX)-docs-$(DISTY_VER)-r$(SVNVER).zip
DISTY_DAT=$(firstword $(wildcard data/out/tmp/icudt$(SO_TARGET_VERSION_MAJOR)*.dat))

DISTY_FILES_SRC=$(DISTY_FILE_TGZ) $(DISTY_FILE_ZIP)
DISTY_FILES=$(DISTY_FILES_SRC) $(DISTY_DOC_ZIP)

$(SVNDOT):
	@echo "ERROR: 'dist' will not work unless the parent of the top_srcdir ( $(SVNTOP) ) is checked out from svn, and svn is installed."
	false

$(DISTY_FILE_DIR):
	$(MKINSTALLDIRS) $(DISTY_FILE_DIR)

$(DISTY_TMP):
	$(MKINSTALLDIRS) $(DISTY_TMP)

$(DISTY_DOC_ZIP): $(SVNDOT) $(DOCZIP) $(DISTY_FILE_DIR)
	cp $(DOCZIP) $(DISTY_DOC_ZIP)

$(DISTY_DAT): 
	echo Missing $@
	/bin/false

$(DOCZIP):
	$(MAKE) -C . srcdir="$(srcdir)" top_srcdir="$(top_srcdir)" builddir=. $@

$(DISTY_FILE_TGZ) $(DISTY_FILE_ZIP): $(SVNDOT) $(DISTY_DAT) $(DISTY_TMP)
	@echo "svnversion of $(SVNTOP) is as follows (if this fails, make sure svn is installed..)"
	svnversion $(SVNTOP)
	-$(RMV) $(DISTY_FILE) $(DISTY_TMP)
	$(MKINSTALLDIRS) $(DISTY_TMP)
	svn export -r $(shell echo $(SVNVER) | tr -d 'a-zA-Z' ) $(SVNURL) "$(DISTY_TMP)/icu"
	$(RMV) $(DISTY_RMDIR)
	$(MKINSTALLDIRS) $(DISTY_IN)
	cp $(DISTY_DAT) $(DISTY_IN)
	( cd $(DISTY_TMP) ; tar cfpz $(DISTY_FILE_TGZ) icu )
	( cd $(DISTY_TMP) ; zip -rlq $(DISTY_FILE_ZIP) icu )
	ls -l $(DISTY_FILE)

dist-local: $(DISTY_FILES)

distcheck: distcheck-tgz

DISTY_CHECK=$(DISTY_TMP)/check

distcheck-tgz: $(DISTY_FILE_TGZ)
	@echo Checking $(DISTY_FILE_TGZ)
	@-$(RMV) $(DISTY_CHECK)
	@$(MKINSTALLDIRS) $(DISTY_CHECK)
	@(cd $(DISTY_CHECK) && tar xfpz $(DISTY_FILE_TGZ) && cd icu/source && $(SHELL) ./configure $(DISTCHECK_CONFIG_OPTIONS) && $(MAKE) check $(DISTCHECK_MAKE_OPTIONS) ) && (echo "!!! PASS: $(DISTY_FILE_TGZ)" )
