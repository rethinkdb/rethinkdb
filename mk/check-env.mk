# Copyright 2013 RethinkDB, all rights reserved.

# Code in this Makefile is used to check for unknown variables specified
# on the make command line or in the config files.

# This file is included by both Makefile and by mk/main.mk

# Generate the list of variables defined in mk/way/default.mk
# All variables that can be set by users should be listed and documented in mk/way/default.mk
$(TOP)/mk/gen/allowed-variables.mk: $(TOP)/mk/way/default.mk
	-@echo "allowed-variables :=" `cat $< | egrep -o '[ \t]*[A-Z_0-9]+[ \t]*[:?+]?=' | egrep -o '[A-Z_0-9]+'` > $@

allowed-variables :=
-include $(TOP)/mk/gen/allowed-variables.mk
allowed-variables += WAY TOP CWD NO_CONFIGURE JUST_SCAN_MAKEFILES COUNTDOWN_TOTAL

STRICT_MAKE_VARIABLE_CHECK ?= 0

# CHECK_ARG_VARIABLES checks the variables set on the command line for unknown variables
# These variables are retrieved from MAKEFLAGS within a recipe
# CHECK_ARG_VARIABLES is used by $(TOP)/Makefile
CHECK_ARG_VARIABLES = $(eval $(value CHECK_ARG_VARIABLES_RUN))
define CHECK_ARG_VARIABLES_RUN
  arg-variables := $(patsubst %=,%,$(filter %=,$(subst =,= ,$(filter-out -%,$(MAKEFLAGS)))))
  ifneq (,$(allowed-variables))
    remaining-variables := $(filter-out $(allowed-variables),$(arg-variables))
    ifneq (,$(remaining-variables))
      ifeq (1,$(STRICT_MAKE_VARIABLE_CHECK))
        $(error Possibly unknown variables: $(remaining-variables))
      else
        $(info make: Possibly unknown variables: $(remaining-variables))
      endif
    endif
  endif
endef 

# These variables need to be set here so they appear in $(.VARIABLES)
old-env-variables ?=
old-makefiles ?=

# check-env-start/check-env-check should be used liks this:
# $(eval $(value check-env-start))
#   include foo.mk
# $(eval $(value check-env-check))

# Cache the current list of variables and included Makefiles
define check-env-start
  old-env-variables := $(.VARIABLES)
  old-makefiles := $(MAKEFILE_LIST)
endef

# Check for unknown variables in the variables defined since the last call to check-env-start
define check-env-check
  MAKE_VARIABLE_CHECK ?= 1
  ifeq (1,$(MAKE_VARIABLE_CHECK))
    new-env-variables := $(filter-out $(old-env-variables),$(.VARIABLES))

    ifneq (,$(allowed-variables))
      remaining-variables := $(filter-out $(allowed-variables) WAY,$(new-env-variables))
      checked-makefiles := $(filter-out $(old-makefiles) $(TOP)/mk/gen/allowed-variables.mk,$(MAKEFILE_LIST))

      ifneq (,$(remaining-variables))
        ifeq (1,$(STRICT_MAKE_VARIABLE_CHECK))
          $(error Possibly unknown variables defined in $(checked-makefiles): $(remaining-variables))
        else
          $(info make: Possibly unknown variables defined in $(checked-makefiles): $(remaining-variables))
        endif
      endif
    endif
  endif
endef
