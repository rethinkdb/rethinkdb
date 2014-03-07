#!/bin/sh

#  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
#
#  Distributed under the Boost Software License, Version 1.0. (See
#  accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
#

msgfmt --endianness=big he/LC_MESSAGES/simple.po -o he/LC_MESSAGES/simple.mo
msgfmt he/LC_MESSAGES/default.po -o he/LC_MESSAGES/default.mo
msgfmt he/LC_MESSAGES/fall.po -o he/LC_MESSAGES/fall.mo
msgfmt he_IL/LC_MESSAGES/full.po -o he_IL/LC_MESSAGES/full.mo

