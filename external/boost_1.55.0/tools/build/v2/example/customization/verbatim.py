# Copyright 2010 Vladimir Prus 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

# This file is only used with Python port of Boost.Build

#  This file shows some of the primary customization mechanisms in Boost.Build V2
#  and should serve as a basic for your own customization.
#  Each part has a comment describing its purpose, and you can pick the parts
#  which are relevant to your case, remove everything else, and then change names
#  and actions to taste.

#  Declare a new target type. This allows Boost.Build to do something sensible
#  when targets with the .verbatim extension are found in sources.
import b2.build.type as type
type.register("VERBATIM", ["verbatim"])

#  Declare a dependency scanner for the new target type. The
#  'inline-file.py' script does not handle includes, so this is
#  only for illustraction.
import b2.build.scanner as scanner;
#  First, define a new class, derived from 'common-scanner',
#  that class has all the interesting logic, and we only need
#  to override the 'pattern' method which return regular
#  expression to use when scanning.
class VerbatimScanner(scanner.CommonScanner):

    def pattern(self):
        return "//###include[ ]*\"([^\"]*)\""

scanner.register(VerbatimScanner, ["include"])
type.set_scanner("VERBATIM", VerbatimScanner)

import b2.build.generators as generators

generators.register_standard("verbatim.inline-file",
                             ["VERBATIM"], ["CPP"])

from b2.manager import get_manager

get_manager().engine().register_action("verbatim.inline-file",
"""
./inline_file.py $(<) $(>)
""")



