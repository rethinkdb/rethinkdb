#
# Regular cron jobs for the rethinkdb package
#
0 4	* * *	root	[ -x /usr/bin/rethinkdb_maintenance ] && /usr/bin/rethinkdb_maintenance
