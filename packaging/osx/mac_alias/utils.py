# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import datetime

ZERO = datetime.timedelta(0)
class UTC (datetime.tzinfo):
    def utcoffset(self, dt):
        return ZERO
    def dst(self, dt):
        return ZERO
    def tzname(self, dt):
        return 'UTC'

utc = UTC()
mac_epoch = datetime.datetime(1904,1,1,0,0,0,0,utc)
unix_epoch = datetime.datetime(1970,1,1,0,0,0,0,utc)
