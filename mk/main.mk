# Copyright 2010-2013 RethinkDB, all rights reserved.

# This is the main file that controls the rethinkdb build process.
# It is run by the top-level Makefile
#
# The actual build rules are located in mk/*.mk and **/build.mk

# The default goal
.PHONY: default-goal
default-goal: real-default-goal

# Build the webui, drivers and executable
.PHONY: all
all: $(TOP)/src/all $(TOP)/admin/all $(TOP)/drivers/all

.PHONY: clean
clean: $(TOP)/admin/clean $(TOP)/src/clean

# $/ is a shorthand for $(TOP)/, without the leading ./
/ := $(patsubst ./%,%,$(TOP)/)

# check-env.mk provides check-env-start and check-env-check
include $(TOP)/mk/check-env.mk

# Include custom.mk, the way, and generate and include the config file
include $(TOP)/mk/configure.mk

# Makefile related definitions
include $(TOP)/mk/lib.mk

# Don't parse the rest of the rules if the configure step is not complete
ifneq (success,$(CONFIGURE_STATUS))
  # Don't build anything 
  real-default-goal:
else # if CONFIGURE_STATUS = success

# The default goal
real-default-goal: $(DEFAULT_GOAL)

# Paths, build rules, packaging, drivers, ...
include $(TOP)/mk/paths.mk

# Download and build internal tools like v8 and gperf
include $(TOP)/mk/support.mk

# make install
include $(TOP)/mk/install.mk

# Clients drivers
include $(TOP)/drivers/build.mk

# Build the web assets
include $(TOP)/mk/webui.mk

# Building the rethinkdb executable
include $(TOP)/src/build.mk

# Packaging for deb, osx, ...
include $(TOP)/mk/packaging.mk

# Rules for tools like valgrind and code coverage report
include $(TOP)/mk/tools.mk

# Generate local Makefiles
include $(TOP)/mk/local.mk

endif # CONFIGURE_STATUS = success