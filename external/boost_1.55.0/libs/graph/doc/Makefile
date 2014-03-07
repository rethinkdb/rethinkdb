# Copyright (c) 2002 Trustees of Indiana University
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

DVIPS		= dvips

LATEX		= latex
LATEXOUT	= dvi
RESULT		= ps

LATEX		= pdflatex
LATEXOUT	= pdf
RESULT		= pdf

.SUFFIXES: .tex .dvi .ps .pdf .c .lg .eps .fig .gif .dot .w .cpp .o .exe

.tex.$(LATEXOUT):
	@ if test ! -f $*.ind; then echo "" > $*.ind; fi
	@ $(LATEX) $*
	@ if ( grep 'LaTeX Warning: Label(s) may' $*.log > /dev/null ); \
	then $(LATEX) $* ; fi
	@ if ( grep 'LaTeX Warning: Citation' $*.log > /dev/null ); \
	then bibtex $* ; $(LATEX) $* ; fi
	@ if ( grep 'LaTeX Warning: Label(s) may' $*.log > /dev/null ); \
	then $(LATEX) $* ; fi
	@ if ( grep 'LaTeX Warning: Label(s) may' $*.log > /dev/null ); \
	then $(LATEX) $* ; fi
	@ if ( grep 'Writing index file' $*.log > /dev/null ); \
	then makeindex $* ; $(LATEX) $* ; fi
	@ if ( grep 'LaTeX Warning: Label(s) may' $*.log > /dev/null ); \
	then $(LATEX) $* ; fi
	@ if ( grep 'LaTeX Warning: Label(s) may' $*.log > /dev/null ); \
	then $(LATEX) $* ; fi

.dvi.ps:
	$(DVIPS) -t letter -o $*.ps $*

.eps.pdf:
	epstopdf $*.eps

.dot.eps:
	dot -Tps -o $*.eps $*.dot 

.w.tex: /afs/nd.edu/user11/jsiek/.jweb
	jweb -np $*.w

BOOST		= $(HOME)/boost
BOOST_ALGO	= $(HOME)/boost.algorithm
CXX		= c++
INCLUDES	= -I$(BOOST) -I$(BOOST_ALGO)
#OPT		= +K2 --display_error_number --diag_suppress=186
OPT		= -Wall -Wno-long-double -ftemplate-depth-30
CXXFLAGS	= $(OPT) $(INCLUDES)

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -c


JWEB	= isomorphism-impl.w isomorphism-impl-v3.w
DOT	= out.dot in.dot iso-eg.dot iso-eg2.dot
EPS_GEN	= $(DOT:.dot=.eps)
WEBTEX	= $(JWEB:.w=.tex)
PDFPICT	= $(EPS_GEN:.eps=.pdf)

default: isomorphism-impl-v3.$(RESULT)

final: isomorphism-impl-v3.$(RESULT)
	$(LATEX) isomorphism-impl-v3
	csh ./mungeaux.csh *.aux
	$(LATEX) isomorphism-impl-v3

#isomorphism-impl.dvi: $(EPS_GEN) $(WEBTEX)
#isomorphism-impl.ps: isomorphism-impl.dvi
#isomorphism-impl.pdf: $(EPS_GEN) $(PDFPICT) $(WEBTEX)
#isomorphism.hpp: isomorphism-impl.w isomorphism-impl.tex

isomorphism-impl-v3.dvi: $(DOT) $(EPS_GEN) $(WEBTEX)
isomorphism-impl-v3.ps: isomorphism-impl-v3.dvi
isomorphism-impl-v3.pdf: $(EPS_GEN) $(PDFPICT) $(WEBTEX)

isomorphism-v3.hpp: isomorphism-impl-v3.w isomorphism-impl-v3.tex

out.dot: isomorphism-impl.tex
in.dot: isomorphism-impl.tex

isomorphism.o: isomorphism-v3.hpp isomorphism-impl-v3.w

isomorphism: isomorphism.o
	$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)

isomorphism-v3.hpp: isomorphism-impl-v3.tex isomorphism-impl-v3.w
eg1-iso.o: isomorphism-impl-v3.tex isomorphism-impl-v3.w

eg1-iso: eg1-iso.o
	$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)


clean:
	rm *.aux *.dvi $(EPS_GEN) $(PDFPICT)
