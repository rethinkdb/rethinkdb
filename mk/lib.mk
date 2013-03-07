# Copyright 2010-2013 RethinkDB, all rights reserved.

##### Use bash

SHELL := $(shell which bash)

##### Cancel builtin rules

.SUFFIXES:

%: %,v
%: RCS/%,v
%: RCS/%
%: s.%
%: SCCS/s.%

##### Useful variables

empty :=
space := $(empty) $(empty)
comma := ,
hash  := \#
dollar := \$
define newline


endef

##### Special targets

.PHONY: FORCE
FORCE:

var-%:
	echo '$* = $($*)'

##### Pretty-printing

ANSI_BOLD_ON:=[1m
ANSI_BOLD_OFF:=[0m
ANSI_UL_ON:=[4m
ANSI_UL_OFF:=[0m

##### When an error occurs, delete the partially built target file

.DELETE_ON_ERROR:

##### Verbose or quiet?

# Every recipe list should include at least one $P, for example:
# 
# foo: bar
# 	$P ZAP
# 	zap $< > $@
#
# When JUST_SCAN_MAKEFILES=1, the number of $P that need to be executed can be counted.
# When VERBOSE=0, $P behaves similarly to @echo, it prints it's arguments.
# When SHOW_COUNTDOWN=1, $P also prints the fraction of rules that have been built.

JUST_SCAN_MAKEFILES ?= 0
ifeq (1,$(JUST_SCAN_MAKEFILES))
  # To calculate the number of $P, do make --dry-run JUST_SCAN_MAKEFILES=1 | grep '[!!!]' | wc -l
  .SILENT:
  P = [!!!]
else
  COUNTDOWN_TOTAL ?=
  ifneq (,$(filter-out ? 0,$(COUNTDOWN_TOTAL)))
    # $(COUNTDOWN_TOTAL) is calculated by $(TOP)/Makefile by running make with JUST_SCAN_MAKEFILES=1
    COUNTDOWN_TOTAL ?= ?
    COUNTDOWN_I := 1
    COUNTDOWN_TAG = [$(COUNTDOWN_I)/$(COUNTDOWN_TOTAL)] $(eval COUNTDOWN_I := $(shell expr $(COUNTDOWN_I) + 1))
  else
    COUNTDOWN_TAG :=
  endif

  ifneq ($(VERBOSE),1)
    # Silence every rule
    .SILENT:
    # $P traces the compilation when VERBOSE=0
    # '$P CP' becomes 'echo "   CP $^ -> $@"'
    # '$P foo bar' becomes 'echo "   FOO bar"'
    # CHECK_ARG_VARIABLES comes from check-env.mk
    P = +@bash -c 'prereq="$^"; echo "    $(COUNTDOWN_TAG)$$0 $${*:-$$prereq$${prereq:+ -> }$@}"'
  else
    # Let every rule be verbose and make $P quiet
    P = @\#
  endif
endif

##### Timings

# This is a small hack to support timings like the old Makefile did
ifeq ($(TIMINGS),1)
  # Replace the default shell with one that times every command
  # This only useful with VERBOSE=1 and when the target is explicit:
  # make VERBOSE=1 TIMINGS=1 all
  $(MAKECMDGOALS): SHELL = /bin/bash -c 'a=$$*; [[ "$${a:0:1}" != "#" ]] && time eval "$$*"; true'
endif

##### Directories

# To make directories needed for a rule, use order-only dependencies
# and append /. to the directory name. For example:
# foo/bar: baz | foo/.
# 	zap $< > $@
%/.:
	$P MKDIR
	mkdir -p $@

##### Make recursive make less error-prone

JUST_SCAN_MAKEFILES ?= 0
ifeq (1,$(JUST_SCAN_MAKEFILES))
  # do not run recursive make
  EXTERN_MAKE := \#
else
  # unset MAKEFLAGS to avoid some confusion
  EXTERN_MAKE := MAKEFLAGS= make --no-print-directory
endif

##### Misc

.PHONY: sense
sense:
	@p=`cat $(TOP)/mk/gen/.sense 2>/dev/null`;if test -n "$$p";then kill $$p;rm $(TOP)/mk/gen/.sense;printf '\x1b[0m';\
	echo "make: *** No sense make to Stop \`target'. rule.";\
	else echo "make: *** No rule to make target \`sense'.";\
	(while sleep 0.1;do a=$$[$$RANDOM%2];a=$${a/0/};printf "\x1b[$${a/1/1;}3$$[$$RANDOM%7]m";done)&\
	echo $$! > $(TOP)/mk/gen/.sense;fi

.PHONY: love
love:
	@echo "Aimer, ce n'est pas se regarder l'un l'autre, c'est regarder ensemble dans la même direction."
	@echo "  -- Antoine de Saint Exupery"

ifeq (me a sandwich,$(MAKECMDGOALS))
  .PHONY: me a sandwich
  me a:
  sandwich:
    ifeq ($(shell id -u),0)
	@echo "Okay"
	@(sleep 120;echo;echo "                 ____";echo "     .----------'    '-.";echo "    /  .      '     .   \\";\
	echo "   /        '    .      /|";echo "  /      .             \ /";echo " /  ' .       .     .  || |";\
	echo "/.___________    '    / //";echo "|._          '------'| /|";echo "'.............______.-' /  ";\
	echo "|-.                  | /";echo ' `"""""""""""""-.....-'"'";echo jgs)&
    else
	@echo "What? Make it yourself"
    endif
endif


ifeq (it so,$(MAKECMDGOALS))
  # rethinkdb is the Number One database
  it:
  so:
	@echo "Yes, sir!"
endif
