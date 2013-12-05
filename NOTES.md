# Release 1.11.1 (Breakfast at Tiffany's)

Released on 2013-12-02

Bug fix update.

* Drivers no longer ignore the timeFormat flag (#1719)
* RethinkDB now correctly sets the `ResponseType` field in responses to `STOP` queries (#1715)
* Fixed a bug that caused RethinkDB to crash with a failed guarantee (#1691)

--

# Release 1.11.0 (Breakfast at Tiffany's)

Released on 2013-11-25

This release introduces a new query profiler, an overhauled streaming infrastructure,
and many enhancements that improve ease and robustness of operation.

## New Features ##

* Server
  * Removed anaphoric macros (#806)
  * Changed the array size limit to 100,000 (#971)
  * The server now reads blobs off of disk simultaneously (#1296)
  * Improved the batch replace logic (#1468)
  * Added IPv6 support (#1469)
  * Reduced random disk seeks during write transactions (#1470)
  * Merged writebacks on the serializer level (#1471)
  * Streaming improvements: smarter batch sizes are sent to clients (#1543)
  * Reduced the impact of index creation on realtime performance (#1556)
  * Optimized insert performance (#1559)
  * Added support for sending data back as JSON, improving driver performance (#1571)
  * Backtraces now span coroutine boundaries (#1602)
* Command line
  * Added a progress bar to `rethinkdb import` and `rethinkdb export` (#1415)
* ReQL
  * Added query profiling, enabled by passing `profile=True` to `run` (#175)
  * Added a sync command that flushes soft writes made on a table (#1046)
  * Made it possible to wait for no-reply writes to complete (#1388)
    * Added a `wait` command
    * Added an optional argument to `close` and `reconnect`
  * Added `index_status` and `index_wait` commands (#1562)
* Python driver
  * Added documentation strings (#808)
  * Added streaming and performance improvements (#1371)
  * Changed the installation procedure for using the C++ protobuf implementation (#1394)
  * Improved the streaming logic (#1364)
* JavaScript driver
  * Changed the installation procedure for using the C++ protobuf implementation (#1172)
  * Improved the streaming logic (#1364)
* Packaging
  * Made the version more explicit in the OS X package (#1413)

## Fixed Bugs ##

* Server
  * No longer access `perfmon_collection_t` after destruction (#1497)
  * Fixed a bug that caused nodes to become unresponsize and added a coroutine profiler (#1516)
  * Made database files compatible between 32-bit and 64-bit versions (#1535)
  * No longer use four times more cache space (#1538)
  * Fix handling of errors in continue queries (#1619)
  * Fixed heartbeat timeout when deleting many tables at once (#1624)
  * Improved signal handling (#1630)
  * Reduced the load caused by using the Web UI on a large cluster (#1660)
* Command line
  * Made `rethinkdb export` more resilient to low-memory conditions (#1546)
  * Fixed a race condition in `rethinkdb import` (#1597)
* ReQL
  * Non-indexed `order_by` queries now return arrays (#1566)
  * Type system now includes selections on both arrays and streams (#1566)
  * Fixed wrong `inserted` result (#1547)
  * Fixed crash caused by using unusual strings in `r.js` (#1638)
  * Redefined batched stream semantics (includes a specific fix for the JavaScipt driver as well) (#1544)
  * `r.js` now works with time values (#1513)
* Python driver
  * Handle interrupted system calls (#1362)
  * Improved the error message when inserting dates with no timezone (#1509)
  * Made `RqlTzInfo` copyable (#1588)
* JavaScript driver
  * Fixed argument handling (#1555, #1577)
  * Fixed `ArrayResult` (#1578, #1584)
  * Fixed pretty-printing of `limit` (#1617)
* Web UI
  * Made it obvious when errors are JavaScript errors or database errors (#1293)
  * Improved the message displayed when there are more than 40 documents (#1307)
  * Fixed date formatting (#1596)
  * Fixed typo (#1668)
* Build
  * Fixed item counts in make (#1443)
  * Bump the fetched version of `gperftools` (#1594)
  * Fixed how `termcap` is handled by `./configure` (#1622)
  * Generate a better version number when compiling from a shallow clone (#1636)

--

# Release 1.10.1 (Von Ryan's Express)

Released on 2013-10-23

Bug fix update.

* Server
  * `r.js` no longer fails when interrupted too early (#1553)
  * Data for some `get_all` queries is no longer duplicated (#1541)
  * Fixed crash `Guarantee failed: [!token_pair->sindex_write_token.has()]` (#1380)
  * Fixed crash caused by rapid resharding (#728)
* ReQL
  * `pluck` in combination with `limit` now returns a stream when possible (#1502)
* Python driver
  * Fixed `undefined variable` error in `tzname()` (#1512)
* Ruby driver
  * Fixed error message when calling unbound function (#1336)
* Packaging
  * Added support for Ubuntu 13.10 (Saucy) (#1554)

# Release 1.10.0 (Von Ryan's Express)

Released on 2013-09-26

## New Features ##

* Added multi-indexes (#933)
* Added selective history deletion to the Data Explorer (#1297)
* Made `filter` support the nested object syntax (#1325)
* Added `r.not_` to the Python driver (#1329)
* The backup scripts are now installed by the Python driver (#1355)
* Implemented variable-width object length encoding and other improvements to serialization (#1424)
* Lowered the I/O pool threads' stack size (#1425)
* Improved datum integer serialization (#1426)
* Added a thin wrapper struct `threadnum_t` instead of using raw ints for thread numbers (#1445)
* Re-enabled full perfmon capability (#1474)

## Fixed Bugs ##

* Fixed the output of `rethinkdb admin help` (#936, #1447)
* `make` no longer runs `./configure` (#979)
* Made the Python code mostly Python 3 compatible (still a work in progress) (#1050)
* The Data Explorer now correctly handles unexpected token errors (#1334)
* Improved the performance of JSON parsing (#1403)
* `batched_rget_stream_t` no longer uses an out-of-date interruptor (#1411)
* The missing `protobuf_implementation` variable was added to the JavaScript driver (#1416)
* Improved the error and help messages in the backup scripts (#1417)
* `r.json` is no longer incorrectly marked as non-deterministic (#1430)
* Fixed the import of `rethinkdb_pbcpp` in the Python driver (#1438)
* Made some stores go on threads other than just 0 through 3 (#1446)
* Fixed the error message when using `groupBy` with illegal pattern (#1460)
* `default` is no longer unprintable in Python (#1476)

--

# Release 1.9.0 (Kagemusha)

Released on 2013-09-09

Bug fix update.

* Server
  * Fixed a potential crash caused by a coroutine switch in exception handlers (#153)
  * No longer duplicate documents in the secondary indexes (#752)
  * Removed unnecessary conversions between `datum_t` and `cJSON` (#1041)
  * No longer load documents from disk for count queries (#1295)
  * Changed extproc code to use `datum_t` instead of `cJSON_t` (#1326)
* ReQL
  * Use `Buffer` instead of `ArrayBuffer` in the JavaScript driver (#1244)
  * `orderBy` on an index no longer destroys the preceding `between` (#1333)
  * Fixed error in the python driver caused by multiple threads using lambdas (#1377)
  * Fixed Unicode handling in the Python driver (#1378)
* Web UI
  * No longer truncate server names (#1313)
* CLI
  * `rethinkdb import` now works with Python 2.6 (#1349)

--

# Release 1.8.1 (High Noon)

Released on 2013-08-21

Bug fix update.

* The shard suggester now works with times (#1335)
* The Python backup scripts no longer rely on `pip show` (#1331)
* `rethinkdb help` no longer uses a pager (#1315, #1308)
* The web UI now correctly positions `SVGRectElement` (#1314)
* Fixed a bug that caused a crash when using `filter` with a non-deterministic value (#1299)

--

# Release 1.8.0 (High Noon)

Released on 2013-08-14

This release introduces date and time support, a new syntax for querying nested objects and 8x improvement in disk usage.

## New Features ##

* ReQL
  * `order_by` now accepts a function as an argument and can efficiently sort by index (#159, #1120, #1258)
  * `slice` and `between` are now half-open by default (#869)
    * The behaviour can be changed by setting the new optional `right_bound` argument to `closed`
      or by setting `left_bound` to `open`
  * `contains` can now be passed a predicate (#870)
  * `merge` is now deep by default (#872)
    * Introduced `literal` to merge flat values
  * `coerce_to` can now convert strings to numbers (#877)
  * Added support for times (#977)
    * `+`, `-`, `<`, `<=`, `>`, `>=`, `==` and `!=`: arithmetic and comparison
    * `during`: match a time with an interval
    * `in_timezone`: change the timezone offset
    * `date`, `time_of_day`, `timezone`, `year`, `month`, `day`, `weekday`, `hour`, `minute` and `second`: accessors
    * `time`, `epoch_time` and `iso8601`: constructors
    * `monday` to `sunday` and `january` to `december`: constants
    * `now`: current time
    * `to_iso8601`, `to_epoch_time`: conversion
  * Add the nested document syntax to functions other than `pluck` (#1094)
    * `without`, `group_by`, `with_fields` and `has_fields`
  * Remove Google Closure from the JavaScript driver (#1194)
    * It now depends on the `protobufjs` and `node-protobuf` libraries
* Server
  * Added a `--canonical-address HOST[:PORT]` command line option for connecting RethinkDB nodes across different networks (#486)
    * Two instances behind proxies can now be configured to connect to each other
  * Optimize space efficiency by allowing smaller block sizes (#939)
  * Added a `--no-direct-io` startup flag that turns off direct IO (#1051)
  * Rewrote the `extproc` code, making `r.js` interuptible and fixing many crashes (#1097, #1106)
  * Added support for V8 >= 3.19 (#1195)
  * Clear blobs when they are unused (#1286)
* Web UI
  * Use relative paths (#1053)
* Build
  * Add support for Emacs' `flymake-mode` (#1161)

## Fixed Bugs ##

* ReQL
  * Check the type of the callback passed to `next` and `each` in the JavaScript driver (#656)
  * Fixed how some backtraces are printed in the JavaScript driver (#973)
  * Coerce the port argument to a number in the Python driver (#1017)
  * Functions that are polymorphic on objects and sequences now only recurse one level deep (#1045)
    * Affects `pluck`, `has_fields`, `with_fields`, etc
  * In the JavaScript driver, no longer fail when requiring the module twice (#1047)
  * `r.union` now returns a stream when given two streams (#1081)
  * `r.db` can now be chained with `do` in the JavaScript driver (#1082)
  * Improve the error message when combining `for_each` and `return_vals` (#1104)
  * Fixed a bug causing the JavaScript driver to overflow the stack when given an object with circular references (#1133)
  * Don't leak internal functions in the JavaScript driver (#1164)
  * Fix the qurey printing in the Python driver (#1178)
  * Correctly depend on node >= 0.10 in the JavaScript driver (#1197)
* Server
  * Improved the error message when there is a version mismatch in data files (#521)
  * The `--no-http-admin` option now disables the check for the web assets folder (#1092)
  * No longer compile JavaScript expressions once per row (#1105)
  * Fixed a crash in the 32-bit version caused by not using 64-bit file offsets (#1129)
  * Fixed a crash caused by malformed json documents (#1132)
  * Fixed a crash caused by moving `func_t` betweek threads (#1157)
  * Improved the scheduling the coroutines that sometimes caused heartbeat timeouts (#1169)
  * Fixed `conflict_resolving_diskmgr_t` to suport files over 1TB (#1170)
  * Fixed a crash caused by disconnecting too fast (#1182)
  * Fixed the error message when `js_runner_t::call` times out (#1218)
  * Fixed a crash caused by serializing unintitialised values (#1219)
  * Fixed a bug that caused an assertion failure in `protob_server_t` (#1220)
  * Fixed a bug causing too many file descriptors to be open (#1225)
  * Fixed memory leaks reported by valgrind (#1233)
  * Fixed a crash triggered by the BTreeSindex test (#1237)
  * Fixed some problems with import performance, interruption, parsing, and error reporting (#1252)
  * RethinkDB proxy no longer crashes when interacting with the Web UI (#1276)
* Tests
  * Fixed the connectivity tests for OS X (#613)
  * Fix the python connection tests (#1110)
* Web UI
  * Provide a less scary error message when a request times out (#1074)
  * Provide proper suggestions in the presence of comments in the Data Explorer (#1214)
  * The `More data` link in the data explorer now works consistently (#1222)
* Documentation
  * Improved the documentaiton for `update` and `pluck` (#1141)
* Build
  * Fixed bugs that caused make to not rebuild certain files or build files it should not (#1162, #1215, #1216, #1229, #1230, #1272)

--

# Release 1.7.3 (Nights of Cabiria)

Released on 2013-07-19

Bug fix update.

* RethinkDB no longer fails to create the data directory when using `--daemon` (#1191)

--

# Release 1.7.2 (Nights of Cabiria)

Released on 2013-07-18

Bug fix update.

* Fixed `wire_func_t` serialization that caused inserts to fail (#1155)
* Fixed a bug in the JavaScript driver that caused asynchronous connections to fail (#1150)
* Removed `nice_crash` and `nice_guarantee` to improve error messages and logging (#1144)
* `rethinkdb import` now warns when unexpected files are found (#1143)
* `rethinkdb import` now correctly imports nested objects (#1142)
* Fixed the connection timeout in the JavaScript driver (#1140)
* Fixed `r.without` (#1139)
* Add a warning to `rethinkdb dump` about indexes and cluster config (#1137)
* Fixed the `debian/rules` makefile to properly build the rethinkdb-dbg package (#1130)
* Allow multiple instances with different port offsets in the init script (#1126)
* Fixed `make install` to not use `/dev/stdin` (#1125)
* Added missing files to the OS X uninstall script (#1123)
* Fixed the documentation for insert with the returnVals flag in JavaScript (#1122)
* No longer cache `index.html` in the web UI (#1117)
* The init script now waits for rethinkdb to stop before restarting (#1115)
* `rethinkdb` porcelain now removes the new directory if it fails (#1070)
* Added cooperative locking to the rethinkdb data directory to detect conflicts earlier (#1109)
* Improved the comments in the sample configuration file (#1078)
* Config file parsing now allows options that apply to other modes of rethinkdb (#1077)
* The init script now creates folders with the correct permissions (#1069)
* Client drivers now time out if the connection handshake takes too long (#1033)

--

# Release 1.7.1 (Nights of Cabiria)

Released on 2013-07-04

Bug fix update.

* Fixed a bug causing `rethinkdb import` to crash on single files
* Added options to `rethinkdb import` for custom CSV separators and no headers (#1112)

--

# Release 1.7.0 (Nights of Cabiria)

Released on 2013-07-03

This release introduces hot backup, atomic set and get operations, significant insert performance improvements, nested document syntax, and native binaries for CentOS / RHEL.

## New Features ##

* ReQL
  * Added `r.json` for parsing JSON strings server-side (#887)
  * Added syntax to `pluck` to access nested documents (#889)
  * `get_all` now takes a variable number of arguments (#915)
  * Added atomic set and get operations (#976)
    * `update`, `insert`, `delete` and `replace` now take an optional `return_vals` argument that returns the values that have been atomically modified
  * Renamed `getattr` to `get_field` and make it polymorphic on arrays (#993)
  * Drivers now use faster protobuf libraries when possible (#1027, #1026, #1025)
  * Drivers now use `r.json` to improve the performance of inserts (#1085)
  * Improved the behaviour of `pluck` (#1095)
    * A field with a non-pluckable value is considered absent
* Web UI
  * It is now possible to resolve `auth_key` conflicts via the web UI (#1028)
  * The web UI now uses relative paths (#1053)
* Server
  * Flushes to disk less often to improve performance without affecting durability (#520)
* CLI
  * Import and export JSON and CSV files (#193)
    * Four new subcommands: `rethinkdb import`, `rethinkdb export`, `rethinkdb dump` and `rethinkdb restore`
  * `rethinkdb admin` no longer requires the `--join` option (#1052)
    * It now connects to localhost:29015 by default
* Documentation
  * Documented durability settings correctly (#1008)
  * Improved instructions for migration (#1013)
  * Documented the allowed character names for tables (#1039)
* Packaging
  * RPMs for CentOS (#268)

## Fixed Bugs ##

* ReQL
  * Fixed the behaviour of `between` with `null` and secondary indexes (#1001)
* Web UI
  * Moved to using a patched Bootstrap, Handlebars.js 1.0.0 and LESS 1.4.0 (#954)
  * Fixed bug causing nested arrays to be shown as `{...}` in the Data Explorer (#1038)
  * Inline comments are now parsed correctly in the Data Explorer (#1060)
  * Properly remove event listeners from the dashboard view (#1044)
* Server
  * Fixed a crash caused by shutting down the server during secondary index creation (#1056)
  * Fixed a potential btree corruption bug (#986)
* Tests
  * ReQL tests no longer leave zombie processes (#1055)

--

# Release 1.6.1 (Fargo)

Released on 2013-06-19

## Critical Security Fixes ##

* Fixed a buffer overflow in the networking code
* Fixed a possible timing attack on the API key

## Other Changes ##

* In python, `r.table_list()` no longer throws an error (#1005)
* Fixed compilation failures with clang 3.2 (#1006)
* Fixed compilation failures with gcc 4.4 (#1011)
* Fixed problems with resolving a conflicted auth_key (#1024)

--

# Release 1.6.0 (Fargo)

Released on 2013-06-13

This release introduces basic access control, regular expression matching, new array operations, random sampling, better error handling, and many bug fixes.

## New Features ##

* ReQL
  * Added access control with a single, common API key (#266)
  * Improved handshake to the client driver protocol (#978)
  * Added `sample` command for random sampling of sequences (#861, #182)
  * Secondary indexes can be queried with boolean values (#854)
  * Added `onFinished` callback to `each` in JavaScript to improve cursors (#443)
  * Added per-command durability settings (#890)
    * Changed `hard_durability=True` to `durability = 'soft' | 'hard'`
    * Added a `durability` option to `run`
  * Added `with_fields` command, and `pluck` no longer throws on missing attributes (#886)
  * Renamed `contains` to `has_fields` (#885)
    * `has_fields` runs on tables, objects, and arrays, and doesn't throw on missing attributes
    * `contains` now checks if an element is in an array
  * Added `default` command: replaces missing fields with a default value (#884)
  * Added new array operations (#868, #198, #341)
      * `prepend`: prepends an element to an array
      * `append`: appends an element to an array
      * `insert_at`: inserts an element at the specified index
      * `splice_at`: splices a list into another list at the specified index
      * `delete_at`: deletes the element at the specified index
      * `change_at`: changes the element at the specified index to the specified value
      * `+` operator: adds two arrays -- returns the ordered union
      * `*` operator: repeats an array `n` times
      * `difference`: removes all instances of specified elements from an array
      * `count`: returns the number of elements in an array
      * `indexes_of`: returns positions of elements that match the specified value in an array
      * `is_empty`: check if an array or table is empty
      * `set_insert`: adds an element to a set
      * `set_intersection`: finds the intersection of two sets
      * `set_union`: returns the union of two sets
      * `set_difference`: returns the difference of two sets
  * Added `match` command for regular expression matching (#867)
  * Added `keys` command that returns the fields of an object (#181)
* Web UI
  * Document fields are now sorted in alphabetical order in the table view (#832)
* CLI
  * Added `-v` flag as an alias for `--version` (#839)
  * Added `--io-threads` flag to allow limiting the amount of concurrent I/O operations (#928)
* Build system
  * Allow building the documentation with Python 3 (#826, #815)
  * Have `make` build the Ruby and Python drivers (#923)

## Fixed Bugs ##

* Server
  * Fixed a crash caused by resharding during secondary index creation (#852)
  * Fixed style problems: code hygiene (#805)
  * Server code cleanup (#920, #924)
  * Properly check permissions for writing the pid file (#916)
* ReQL
  * Use the correct function name in backtraces (#995)
  * Fixed callback issues in the JavaScript driver (#846)
  * In JavaScript, call the callback when the connection is closed (#758)
  * In JavaScript, GETATTR now checks the argument count (#992)
  * Tweaked the CoffeeScript source to work with older versions of coffee and node (#963)
  * Fixed a typo in the error handling of the JavaScript driver (#961)
  * Fixed performance regression for `pluck` and `without` (#947)
  * Make sure callbacks get cleared in the Javascript driver (#942)
  * Improved errors when `return` is omitted in Javascript (#914)
* Web UI
  * Correctly distinguish case sensitive table names (#822)
  * No longer display some results as `{ ... }` in the table view (#937)
  * Ensured tables are available before listing their indexes (#926)
  * Fixed the autocompletion for opening square brackets (#917)
* Tests
  * Modified the polyglot test framework to catch more possible errors (#724)
  * Polyglot tests can now run in debug mode (#614)
* Build
  * Look for `libprotobuf` in the correct path (#860, #853)
  * Always fetch and use the same version of Handlebars.js (#821, #819, #958)
  * Check for versions of LESS that are known to work (#956)
  * Removed bitrotted MOCK_CACHE_CHECK option (#804)

--

# Release 1.5.2 (Le Gras de Ouate)

Released on 2013-05-24

Bug fix update.

## Changes ##

* Fix #844: Compilation error when using `./configure --fetch protobuf` resolved
* Fix #840: Using `.run` in the data explorer gives a more helpful error message
* Fix #817: Fix a crash caused by adding a secondary index while under load
* Fix #831: Some invalid queries no longer cause a crash

--

# Release 1.5.1 (The Grad You Ate) #

Released on 2013-05-16

Bug fix update.

## Changes ###

* Fix a build error
* Fix #816: Table view in the data explorer is no longer broken

---

# Release 1.5.0 (The Graduate) #

Released on 2013-05-16

This release introduces secondary indexes, stability and performance enhancements and many bug fixes.

## New Features ##

* Server
  * #69: Bad JavaScript snippets can no longer hang the server
    * `r.js` now takes an optional `timeout` argument
  * #88: Added secondary and compound indexes
  * #165: Backtraces on OS X
  * #207: Added soft durability option
  * #311: Added daemon mode (via --daemon)
  * #423: Allow logging to a specific file (via --log-file)
  * #457: Significant performance improvement for batched inserts
  * #496: Links against libc++ on OS X
  * #672: Replaced environment checkpoints with shared pointers in the ReQL layer, reducing the memory footprint
  * #715: Adds support for noreply writes
* CLI
  * #566: In the admin CLI, `ls tables --long` now includes a `durability` column
* Web UI
  * #331: Improved data explorer indentation and brace pairing
  * #355: Checks for new versions of RethinkDB from the admin UI
  * #395: Auto-completes databases and tables names in the data explorer
  * #514: Rewrote and improved the documentation generator
  * #569: Added toggle for Data Explorer brace pairing
  * #572: Changed button styles in the Data Explorer
  * #707: Two step confirmation added to the delete database button
* ReQL
  * #469: Added an `info` command that provides information on any ReQL value (useful for retreiving the primary key)
  * #604: Allow iterating over result arrays as if they were cursors in the JavaScript client
  * #670: Added a soft durability option to `table_create`
* Testing
  * #480: Run [tests on Travis-CI](https://travis-ci.org/rethinkdb/rethinkdb)
  * #517: Improved the integration tests
  * #587: Validate API documentation examples
  * #669: Added color to `make test` output

## Fixed bugs ##

* Server
  * Fix #403: Serialization of `perfmon_result_t` is no longer 32-bit/64-bit dependent
  * Fix #505: Reduced memory consumption to avoid failure on read query "tcmalloc: allocation failed"
  * Fix #510: Make bind to `0.0.0.0` be equivalent to `all`
  * Fix #639: Fixed bug where 100% cpu utilization was reported on OSX
  * Fix #645: Fixed server crash when updating replicas / acks
  * Fix #665: Fixed queries across shards that would trigger `Assertion failed: [!br.point_replaces.empty()] in src/rdb_protocol/protocol.cc`
  * Fix #750: Resolved intermittent RPCSemilatticeTest.MetadataExchange failure
  * Fix #757: Fixed a crashing bug in the environment checkpointing code
  * Fix #796: Crash when running batched writes to a sharded table in debug mode
* Documentation
  * Fix #387: Documented the command line option --web-static-directory (custom location for web assets)
  * Fix #503: Updated outdated useOutdated documentation
  * Fix #563: Fixed documentation for `typeof()`
  * Fix #610: Documented optional arguments for `table_create`: `datacenter` and `cache_size`
  * Fix #631: Fixed missing parameters in documentation for `reduce`
  * Fix #651: Updated comments in protobuf file to reflect current spec
  * Fix #679: Fixed incorrect examples in protobuf spec
  * Fix #691: Fixed faulty example for `zip` in API documentation
* Web UI
  * Fix #529: Suggestion arrow no longer goes out of bounds
  * Fix #583: Improved Data Explorer suggestions for `table`
  * Fix #584: Improved Data Explorer suggestion style and content
  * Fix #623: Improved margins on modals
  * Fix #629: Updated buttons and button styles on the Tables View (and throughout the UI)
  * Fix #662: Command key no longer deletes text selection in Data Explorer
  * Fix #725: Improved documentation in the Data Explorer
  * Fix #790: False values in a document no longer show up as blank in the tree view
* ReQL
  * Fix #46: Added top-level `tableCreate`, `tableDrop` and `tableList` to drivers
  * Fix #125: Grouped reductions now return a stream, not an array
  * Fix #370: `map` after `dbList` and `tableList` no longer errors
  * Fix #421: Return values from mutation operations now include all attributes, even if zero (`inserted`,`deleted`,`skipped`,`replaced`,`unchanged`,`errors`)
  * Fix #525: Improved error for `groupBy`
  * Fix #527: Numbers that can be represented as integers are returned as native integers
  * Fix #603: Check the number of arguments correctly in the JavaScript client
  * Fix #632: Keys of objects are no longer magically converted to strings in the drivers
  * Fix #640: Clients handle system interrupts better
  * Fix #642: Ruby client no longer has a bad error message when queries are run on a closed connection
  * Fix #650: Python driver successfully sends `STOP` query when the connection is closed
  * Fix #663: Undefined values are handled correctly in JavaScript objects
  * Fix #678: `limit` no longer automatically converts streams to arrays
  * Fix #689: Python driver no longer hardcodes the default database
  * Fix #704: `typeOf` is no longer broken in JavaScript driver
  * Fix #730: Ruby driver no longer accepts spurious methods
  * Fix #733: `groupBy` can now handle both a `MAKE_OBJ` protobuf and a literal `R_OBJECT` datum protobuf
  * Fix #767: Server now detects `NaN` in all cases
  * Fix #777: Ruby driver no longer occasionally truncates messages
  * Fix #779: Server now rejects strings that contain a null byte
  * Fix #789: Style improvements in Ruby driver
  * Fix #799: Python driver handles `use_outdated` when specified as a global option argument
* Testing
  * Fix #653: `make test` now works on OS X
* Build
  * Fix #475: Added a workaround to avoid `make -j2` segfaulting when building on older versions of `make`
  * Fix #541: `make clean` is more aggressive
  * Fix #630: Removed cruft from the RethinkDB source tree
  * Fix #635: Building fails quickly with troublesome versions of `coffee-script`
  * Fix #694: Improved warnings when building with `clang` and `ccache`
  * Fix #717: Fixed `./configure --fetch-protobuf`
* Migration
  * Fix #782: Improved speed in migration script

## Other ##

  * #508: Removed the JavaScript mock server
  * [All closed issues between 1.4.5 and 1.5.0](https://github.com/rethinkdb/rethinkdb/issues?milestone=8&page=1&state=closed)

---

# Release 1.4.5 (Some Like It Hot) #

Released on 2013-05-03.

## Changes ##

Bug fix update:

* Increase the TCP accept backlog (#369).

---

# Release 1.4.4 (Some Like It Hot) #

Released on 2013-04-19.

## Changes ##

Bug fix update:

* Improved the documentation
  * Made the output of `rethinkdb help` and `--help` consistent (#643)
  * Clarify details about the client protocol (#649)
* Cap the size of data returned from rgets not just the number of values (#597)
  * The limit changed from 4000 rows to 1MiB
* Partial bug fix: Rethinkdb server crashing (#621)
* Fixed bug: Can't insert objects that use 'self' as a key with Python driver (#619)
* Fixed bug: [Web UI] Statistics graphs are not plotted correctly in background tabs (#373)
* Fixed bug: [Web UI] Large JSON Causes Data Explorer to Hang (#536)
* Fixed bug: Import command doesn't import last row if doesn't have end-of-line (#637)
* Fixed bug: [Web UI] Cubism doesn't unbind some listeners (#622)
* Fixed crash: Made global optargs actually propagate to the shards properly (#683)

---

# Release 1.4.3 (Some Like It Hot) #

Released on 2013-04-10.

## Changes ##

Bug fix update:

* Improve the networking code in the Python driver
* Fix a crash triggered by a type error when using concatMap (#568)
* Fix a crash when running `rethinkdb proxy --help` (#565)
* Fix a bug in the Python driver that caused it to occasionally return `None` (#564)

---

# Release 1.4.2 (Some Like It Hot) #

Released on 2013-03-30.

## Changes ##

Bug fix update:

* Replace `~` with `About` in the web UI (#485)
* Add framing documentation to the protobuf spec (#500)
* Fix crashes triggered by .orderBy().skip() and .reduce(r.js()) (#522, #545)
* Replace MB with GB in an error message (#526)
* Remove some semicolons from the protobuf spec (#530)
* Fix the `rethinkdb import` command (#535)
* Improve handling of very large queries in the data explorer (#536)
* Fix variable shadowing in the javascript driver (#546)

---

# Release 1.4.1 (Some Like It Hot) #

Released on 2013-03-22.

## Changes ##

Bug fix update:

* Python driver fix for TCP streams (#495)
* Web UI fix that reduces the number of AJAX requests (#481)
* JS driver: added useOutdated to r.table() (#502)
* RDB protocol performance fix in release mode.
* Performance fix when using filter with object shortcut syntax.
* Do not abort when the `runuser` or `rungroup` options are present (#512)

---

# Release 1.4 (Some Like It Hot) #

Relased on 2013-03-18.

## Changes ##

* Improved ReQL wire protocol and client drivers
* New build system
* Data explorer query history

---

# Release 1.3.2 (Metropolis) #

Released on 2013-01-15.

## Changes ##

* Fixed security bug in http server.

---

# Release 1.3.1 (Metropolis) #

Released on 2012-12-20.

## Changes ##

* Fixed OS X crash on ReQL exceptions.

---

# Release 1.3.0 (Metropolis) #

Released on 2012-12-20.

## Changes ##

* Native OS X support.
* 32-bit support.
* Support for legacy systems (e.g. Ubuntu 10.04)

---

# Release 1.2.8 (Rashomon) #

Released on 2012-12-15.

## Changes ##

* Updating data explorer suggestions to account for recent `r.row`
  changes.

---

# Release 1.2.7 (Rashomon) #

Released on 2012-12-14.

## Changes ##

* Lots and lots of bug fixes

---

# Release 1.2.6 (Rashomon) #

Released on 2012-11-13.

## Changes ##

* Fixed the version string
* Fixed 'crashing after crashed'
* Fixed json docs

---

# Release 1.2.5 (Rashomon) #

Released on 2012-11-11.

## Changes ##

* Checking for a null ifaattrs

---

# Release 1.2.4 (Rashomon) #

Released on 2012-11-11.

## Changes ##

* Local interface lookup is now more robust.

---

# Release 1.2.3 (Rashomon) #

Released on 2012-11-09.

## Changes ##

* Fixes a bug in the query engine that causes large range queries to
  return incorrect results.

---

# Release 1.2.0 (Rashomon) #

Released on 2012-11-09.

## Changes ##

This is the first release of the product. It includes:

* JSON data model and immediate consistency support
* Distributed joins, subqueries, aggregation, atomic updates
* Hadoop-style map/reduce
* Friendly web and command-line administration tools
* Takes care of machine failures and network interrupts
* Multi-datacenter replication and failover
* Sharding and replication to multiple nodes
* Queries are automatically parallelized and distributed
* Lock-free operation via MVCC concurrency

## Limitations ##

There are a number of technical limitations that aren't baked into the
architecture, but were not resolved in this release due to time
pressure. They will be resolved in subsequent releases.

* Write requests have minimal batching in memory and are flushed to
  disk on every write. This significantly limits write performance,
  expecially on rotational disks.
* Range commands currently don't use an index.
* The clustering system has a bottleneck in cluster metadata
  propagation. Cluster management slows down significantly when more
  than sixteen shards are used.

