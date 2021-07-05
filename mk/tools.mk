# Copyright 2010-2013 RethinkDB, all rights reserved.

##### Code information

coffeelint:
	$P COFFEELINT ""
	-coffeelint -f $(TOP)/scripts/coffeelint.json -r $(TOP)/admin/

style: coffeelint
	$P CHECK-STYLE ""
	$(TOP)/scripts/check_style.sh

showdefines:
	$P SHOW-DEFINES ""
	$(RT_CXX) $(RT_CXXFLAGS) -m32 -E -dM - < /dev/null
