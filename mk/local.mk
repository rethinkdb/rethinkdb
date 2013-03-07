##### Generate Makefiles in subdirectories

# Do not overwrite the top-level Makefile
$/Makefile:

$/%/Makefile:
	$P ECHO "> $@"
	( echo 'TOP := $(call relative-path,$(TOP),$(@D))' ; \
	  echo 'include $$(TOP)/Makefile' \
	) > $@

relative-path = $(subst $(space),/,$(patsubst %,..,$(subst /,$(space),$(subst $(abspath $1),,$(abspath $2)))))