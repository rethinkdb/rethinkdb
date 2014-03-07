#*******************************************************************************
#*   Copyright (C) 2008-2010, International Business Machines
#*   Corporation and others.  All Rights Reserved.
#*******************************************************************************

#sorry, this won't be friendly to out of source builds

srcdir=.
top_srcdir=../..
top_builddir=../..

include $(top_builddir)/icudefs.mk

LOCALHEADERS= udeprctd.h udraft.h uintrnal.h usystem.h

COMMONHDR= $(top_srcdir)/common/unicode
DOCDIR= $(top_builddir)/doc/html
EXCLUDE=$(srcdir)/exclude.txt

all:
	@echo Usage:  'make install-headers'  to update headers.
	@echo 'be sure to verify the headers (in soure/common/unicode) before checkin!'
	@exit 1

clean:
	-$(RMV) $(LOCALHEADERS)

$(DOCDIR):
	( cd $(top_builddir) ; $(MAKE) doc )

local-headers: $(LOCALHEADERS)

install-headers: $(DOCDIR)
	perl ./genheaders.pl --srcdir=$(DOCDIR) --destdir=$(COMMONHDR) --version=$(VERSION) --exclusion-list=$(EXCLUDE)
	( cd $(COMMONHDR) ; ls -l $(LOCALHEADERS) )

%.h:	$(COMMONHDR)/%.h
	cp $< $@

