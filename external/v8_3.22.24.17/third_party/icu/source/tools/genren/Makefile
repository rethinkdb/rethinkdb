#******************************************************************************
#
#   Copyright (C) 2002-2009, International Business Machines
#   Corporation and others.  All Rights Reserved.
#
#******************************************************************************

TOP=../..
srcdir = .
top_srcdir = $(TOP)

top_builddir = $(TOP)

include $(TOP)/icudefs.mk
ICUDIR=ICUunrenamed
#SO=so

COM=$(ICUDIR)/lib/libicuuc.$(SO)
I18=$(ICUDIR)/lib/libicui18n.$(SO)
LAY=$(ICUDIR)/lib/libicule.$(SO)
LEX=$(ICUDIR)/lib/libiculx.$(SO)
DAT=$(ICUDIR)/stubdata/libicudata.$(SO)
UIO=$(ICUDIR)/lib/libicuio.$(SO)

LIBS=$(COM) $(I18) $(LAY) $(LEX) $(UIO)

# Extra flags to prevent internal API from being hidden.
# This is important because ELF (Linux) based platforms that don't hide internal
# API will allow a duplicate internal name to resolve to an external library.
# See the gcc manual on the "visibility" attribute for details.
FLAG_OVERRIDE= LIBCFLAGS= LIBCXXFLAGS= $(EXTRA_MAKE_OPTIONS)

all:
	@cat README

clean:
	-rm -rf $(ICUDIR) urename.* *~

# We use config.status to mean we have a valid out of source tree.

$(ICUDIR)/config.status:
	-mv $(ICUDIR) $(ICUDIR)old
	-(rm -rf $(ICUDIR)old &)
	mkdir $(ICUDIR)
	( cd $(ICUDIR) ; CPPFLAGS=-DURENAME_H $(TOP)/../configure --with-data-packaging=archive --enable-tests=no --prefix=`pwd` $(GENREN_CONFIGURE_OPTS) )
    # cause lib and bin to be created, and any other general sanity
	$(MAKE) $(FLAG_OVERRIDE) -C $(ICUDIR) clean
	$(MAKE) $(FLAG_OVERRIDE) -C $(ICUDIR) all-local

# build the libraries
$(DAT): $(ICUDIR)/config.status Makefile
	$(MAKE) $(FLAG_OVERRIDE) -C $(ICUDIR)/stubdata  all-local

$(COM): $(DAT) $(ICUDIR)/config.status Makefile
	$(MAKE) $(FLAG_OVERRIDE) -C $(ICUDIR)/common all-local

$(I18): $(DAT) $(COM) $(ICUDIR)/config.status Makefile
	$(MAKE) $(FLAG_OVERRIDE) -C $(ICUDIR)/i18n all-local

$(LAY): $(DAT) $(I18) $(COM) $(ICUDIR)/config.status Makefile
	$(MAKE) $(FLAG_OVERRIDE) -C $(ICUDIR)/layout all-local

$(LEX): $(DAT) $(I18) $(COM) $(ICUDIR)/config.status Makefile
	$(MAKE) $(FLAG_OVERRIDE) -C $(ICUDIR)/layoutex all-local

$(UIO):	$(DAT) $(I18) $(COM) $(ICUDIR)/config.status Makefile
	$(MAKE) $(FLAG_OVERRIDE) -C $(ICUDIR)/io all-local

# the header itself
urename.h: $(LIBS) genren.pl
	-cp urename.h urename.h.old
	perl ./genren.pl $(GENREN_PL_OPTS) $(LIBS)

sorts:	urename.sort urename.old.sort
	@echo "*** Please check urename.h manually before committing it."
	@echo "Try 'diff --side-by-side urename.old.sort urename.sort'"

urename.sort: urename.h
	sort urename.h > $@

urename.old.sort: $(TOP)/common/unicode/urename.h
	sort  $(TOP)/common/unicode/urename.h > $@

install-header: urename.h sorts
	cp $(TOP)/common/unicode/urename.h $(TOP)/common/unicode/urename.h.old
	cp urename.h $(TOP)/common/unicode/

-include Makefile.local

