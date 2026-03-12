# RethinkDB Open Issues Todo List

**Generated:** 2026-03-11  
**Total Open Issues:** 1339

## Summary by Category

| Category | Count | Priority |
|----------|-------|----------|
| Build & Compilation | 42 | High |
| Cluster & Network | 113 | High |
| Security | 5 | Critical |
| Platform-Specific | 62 | Medium |
| Performance & Memory | 52 | Medium |
| JavaScript Engine | 22 | Medium |
| Feature Requests | 185 | Low |
| Documentation | 27 | Low |
| Other | 831 | Triage |

---

## 🔴 Critical Priority (Security & Crashes)

### Security Issues

- [ ] **#6329** - Security Feature Request: Randomized URL Suffix for Administrative GUI
  - https://github.com/rethinkdb/rethinkdb/issues/6329
- [ ] **#6290** - Uncatchable ReqlTimeoutError on wrong database password
  - https://github.com/rethinkdb/rethinkdb/issues/6290
- [ ] **#5947** - add authentication to WebUI
  - https://github.com/rethinkdb/rethinkdb/issues/5947
- [ ] **#4210** - Enable business logic enforcement with security rules
  - https://github.com/rethinkdb/rethinkdb/issues/4210
- [ ] **#3159** - Encryption and authentication for the web UI
  - https://github.com/rethinkdb/rethinkdb/issues/3159

---

## 🟠 High Priority (Build & Core Functionality)

### JavaScript Engine Issues

- [ ] **#7008** - QuickJS-on-Windows support
  - https://github.com/rethinkdb/rethinkdb/issues/7008
- [ ] **#5872** - Javascript driver doesn't properly accept optargs for prefix union
  - https://github.com/rethinkdb/rethinkdb/issues/5872
- [ ] **#5738** - "Cursor is closed" in Javascript since upgrading to Rethink 2.3.
  - https://github.com/rethinkdb/rethinkdb/issues/5738
- [ ] **#5589** - Add `close` event to JavaScript event emitter interface
  - https://github.com/rethinkdb/rethinkdb/issues/5589
- [ ] **#5568** - Make `toString` on JavaScript cursors ignore arguments 
  - https://github.com/rethinkdb/rethinkdb/issues/5568
- [ ] **#5476** - Web ui - an inline comment in the data explorer causes JavaScript error
  - https://github.com/rethinkdb/rethinkdb/issues/5476
- [ ] **#5473** - complete the JavaScript cursor tests for `r.expr`
  - https://github.com/rethinkdb/rethinkdb/issues/5473
- [ ] **#5453** - JavaScript `run` command returns a promise if conn is null/undefined even with a function
  - https://github.com/rethinkdb/rethinkdb/issues/5453
- [ ] **#5430** - make cursor.toString produce something nice in JavaScript driver
  - https://github.com/rethinkdb/rethinkdb/issues/5430
- [ ] **#5276** - Bundled ICU in bundled V8 detects the wrong compiler
  - https://github.com/rethinkdb/rethinkdb/issues/5276
- [ ] **#5137** - Decouple ICU from V8
  - https://github.com/rethinkdb/rethinkdb/issues/5137
- [ ] **#4026** - return connection along with value in JavaScript promises
  - https://github.com/rethinkdb/rethinkdb/issues/4026
- [ ] **#3972** - Feature request: would you consider releasing a javascript implementation of reql ?
  - https://github.com/rethinkdb/rethinkdb/issues/3972
- [ ] **#3786** - JavaScript driver nits/fixes
  - https://github.com/rethinkdb/rethinkdb/issues/3786
- [ ] **#3675** - Change the format of group queries in python and ruby to match javascript
  - https://github.com/rethinkdb/rethinkdb/issues/3675
- [ ] **#3569** - ReQL Test JavaScript "driver" does not actually run atexit cleanup
  - https://github.com/rethinkdb/rethinkdb/issues/3569
- [ ] **#3297** - Move HTTP connections out of the JavaScript driver and into the web ui
  - https://github.com/rethinkdb/rethinkdb/issues/3297
- [ ] **#3263** - JavaScript driver doesn't work across modules, if those modules import local driver instances
  - https://github.com/rethinkdb/rethinkdb/issues/3263
- [ ] **#3027** - Consider having our own object that inherits from Javascript's `Date` to preserve timezone information
  - https://github.com/rethinkdb/rethinkdb/issues/3027
- [ ] **#2842** - Figure out what to do about the JavaScript YAML tests translating underscores
  - https://github.com/rethinkdb/rethinkdb/issues/2842
- [ ] **#1928** - JavaScript driver cobbles the onConnect error message
  - https://github.com/rethinkdb/rethinkdb/issues/1928
- [ ] **#1587** - JavaScript driver - each should destroy the cursor if the callback return false
  - https://github.com/rethinkdb/rethinkdb/issues/1587

### Build & Compilation Issues (Top 15)

- [ ] **#6952** - build fails on AArch64, Fedora 33 
  - https://github.com/rethinkdb/rethinkdb/issues/6952
- [ ] **#6936** - 2.4.1 windows build fails
  - https://github.com/rethinkdb/rethinkdb/issues/6936
- [ ] **#6932** - Add Mac ARM build
  - https://github.com/rethinkdb/rethinkdb/issues/6932
- [ ] **#6873** - ArchLinux - Build issues and fixes
  - https://github.com/rethinkdb/rethinkdb/issues/6873
- [ ] **#6867** - Reproducible builds
  - https://github.com/rethinkdb/rethinkdb/issues/6867
- [ ] **#6531** - Request Update To Allow Builds With VS2017
  - https://github.com/rethinkdb/rethinkdb/issues/6531
- [ ] **#6473** - Cannot build rethinkdb on windows
  - https://github.com/rethinkdb/rethinkdb/issues/6473
- [ ] **#6337** - Support building against musl
  - https://github.com/rethinkdb/rethinkdb/issues/6337
- [ ] **#6201** - Package build fails on Raspberry Pi
  - https://github.com/rethinkdb/rethinkdb/issues/6201
- [ ] **#6001** - rework build system linker dependency system
  - https://github.com/rethinkdb/rethinkdb/issues/6001
- [ ] **#5998** - Build recently modified cc files first
  - https://github.com/rethinkdb/rethinkdb/issues/5998
- [ ] **#5893** - mysterious connection latency when building deb package of PR #5720
  - https://github.com/rethinkdb/rethinkdb/issues/5893
- [ ] **#5620** - decouple the driver building from `./configure`
  - https://github.com/rethinkdb/rethinkdb/issues/5620
- [ ] **#5431** - Add back a way to configure custom shard boundaries
  - https://github.com/rethinkdb/rethinkdb/issues/5431
- [ ] **#5149** - Compare-and-swap (CAS) command as a building block for synchronization primitives
  - https://github.com/rethinkdb/rethinkdb/issues/5149

### Cluster & Network Issues (Top 15)

- [ ] **#7158** - Failed to remove a node from the cluster
  - https://github.com/rethinkdb/rethinkdb/issues/7158
- [ ] **#7131** - cluster connect/reconnect timeout
  - https://github.com/rethinkdb/rethinkdb/issues/7131
- [ ] **#7129** - Reasonable to change hard-coded cluster size?
  - https://github.com/rethinkdb/rethinkdb/issues/7129
- [ ] **#7002** - Many connections in TIME_WAIT
  - https://github.com/rethinkdb/rethinkdb/issues/7002
- [ ] **#6995** - ReqlTimeoutError: Could not connect to [ip], operation timed out.
  - https://github.com/rethinkdb/rethinkdb/issues/6995
- [ ] **#6961** - Connectivity cluster tag mismatch error
  - https://github.com/rethinkdb/rethinkdb/issues/6961
- [ ] **#6880** - Connecting a re-provisioned server brings down the entire cluster
  - https://github.com/rethinkdb/rethinkdb/issues/6880
- [ ] **#6856** - Damaged Rethinkdb Cluster :: Guarantee failed: [token.has()] 
  - https://github.com/rethinkdb/rethinkdb/issues/6856
- [ ] **#6849** - Proxy loses state from cluster
  - https://github.com/rethinkdb/rethinkdb/issues/6849
- [ ] **#6656** - Rethinkdb `db.wait()` fails with timeout when tables are all ready
  - https://github.com/rethinkdb/rethinkdb/issues/6656
- [ ] **#6520** - Multi-node Cluster Issue 
  - https://github.com/rethinkdb/rethinkdb/issues/6520
- [ ] **#6442** - High idle-load on 4 node cluster
  - https://github.com/rethinkdb/rethinkdb/issues/6442
- [ ] **#6437** - Investigate possible connection issues in 2.4
  - https://github.com/rethinkdb/rethinkdb/issues/6437
- [ ] **#6345** - Node crash with "Guarantee failed: [refcount == 0]"
  - https://github.com/rethinkdb/rethinkdb/issues/6345
- [ ] **#6130** - cluster reconnect try time
  - https://github.com/rethinkdb/rethinkdb/issues/6130

---

## 🟡 Medium Priority (Platform & Performance)

### Platform-Specific Issues (Top 10)

- [ ] **#7171** - Windows prebuilt binaries is outdated
  - https://github.com/rethinkdb/rethinkdb/issues/7171
- [ ] **#7156** - Error <jemalloc>: Unsupported system page size with Raspberry Pi 5 !!!
  - https://github.com/rethinkdb/rethinkdb/issues/7156
- [ ] **#7124** - error: to_string called on an uninitialized ip_address_t, addr_type: 0 compiling rethinkdb on Raspberry
  - https://github.com/rethinkdb/rethinkdb/issues/7124
- [ ] **#7086** - Failed to determine absolute path for Metadata (Docker - Linux on Windows) for bind-mount volume
  - https://github.com/rethinkdb/rethinkdb/issues/7086
- [ ] **#6878** - Compiling from source on Jetson Nano
  - https://github.com/rethinkdb/rethinkdb/issues/6878
- [ ] **#6829** - "No rule to make target" on Windows
  - https://github.com/rethinkdb/rethinkdb/issues/6829
- [ ] **#6721** - RethinkDB 2.3.6 crashes on Cubietruck(ARM) /  ARMBian/ Debian GNU/Linux 9.7 (stretch)
  - https://github.com/rethinkdb/rethinkdb/issues/6721
- [ ] **#6699** - Compiling 2.3.x for aarch64
  - https://github.com/rethinkdb/rethinkdb/issues/6699
- [ ] **#6636** - FreeBSD compilation
  - https://github.com/rethinkdb/rethinkdb/issues/6636
- [ ] **#6584** - rethinkdb restore fails under windows.
  - https://github.com/rethinkdb/rethinkdb/issues/6584

### Performance & Memory Issues (Top 10)

- [ ] **#7123** - Evaluate Profile-Guided Optimization (PGO) on RethinkDB
  - https://github.com/rethinkdb/rethinkdb/issues/7123
- [ ] **#6316** - getAll an order of magnitude slower than get on changefeeds
  - https://github.com/rethinkdb/rethinkdb/issues/6316
- [ ] **#6151** - Performance issues comparing to MySQL
  - https://github.com/rethinkdb/rethinkdb/issues/6151
- [ ] **#6119** - Seeing massive performance degredation on a replace operation.
  - https://github.com/rethinkdb/rethinkdb/issues/6119
- [ ] **#6063** - .eqJoin().group() is slower than .map().group()
  - https://github.com/rethinkdb/rethinkdb/issues/6063
- [ ] **#6062** - Poor performance with .limit() after a large .skip()
  - https://github.com/rethinkdb/rethinkdb/issues/6062
- [ ] **#6055** - Backfill process too slow
  - https://github.com/rethinkdb/rethinkdb/issues/6055
- [ ] **#6012** - .skip().limit() performance is worse than .slice()
  - https://github.com/rethinkdb/rethinkdb/issues/6012
- [ ] **#5972** - Slow on querying system tables and creating/deleting tables
  - https://github.com/rethinkdb/rethinkdb/issues/5972
- [ ] **#5918** - investigate fsync performance on Apple's APFS
  - https://github.com/rethinkdb/rethinkdb/issues/5918

---

## 🟢 Low Priority (Features & Docs)

### Feature Requests (Top 10)

- [ ] **#7142** - Support protobuf 25
  - https://github.com/rethinkdb/rethinkdb/issues/7142
- [ ] **#7120** - Add "Buffers" from /proc/meminfo in parse_meminfo_file to determine available memory
  - https://github.com/rethinkdb/rethinkdb/issues/7120
- [ ] **#6951** - Is there some type of support to "virtual" computed column on rethinkdb
  - https://github.com/rethinkdb/rethinkdb/issues/6951
- [ ] **#6885** - addr2line: addr2line: 'rethinkdb': ... serialize
  - https://github.com/rethinkdb/rethinkdb/issues/6885
- [ ] **#6874** - Support for JetBrains Data Grip integration
  - https://github.com/rethinkdb/rethinkdb/issues/6874
- [ ] **#6836** - Move enterprise features
  - https://github.com/rethinkdb/rethinkdb/issues/6836
- [ ] **#6815** - rethinkdb crash when adding a new server
  - https://github.com/rethinkdb/rethinkdb/issues/6815
- [ ] **#6736** - Feature Request: Create a ReQL alternative based on JSON
  - https://github.com/rethinkdb/rethinkdb/issues/6736
- [ ] **#6715** - Support for pushing notifications based on geograhic position
  - https://github.com/rethinkdb/rethinkdb/issues/6715
- [ ] **#6546** - Can't start RethinkDb (Can't assign requested address)
  - https://github.com/rethinkdb/rethinkdb/issues/6546

### Documentation Issues (Top 10)

- [ ] **#7154** - Update Website 
  - https://github.com/rethinkdb/rethinkdb/issues/7154
- [ ] **#7148** - Something i forgot to had when having the default doc like so
  - https://github.com/rethinkdb/rethinkdb/issues/7148
- [ ] **#5857** - Allow retrieving the btree timestamp for a document
  - https://github.com/rethinkdb/rethinkdb/issues/5857
- [ ] **#5844** - RethinkDB in a docker container reports table is sharded in query profiler
  - https://github.com/rethinkdb/rethinkdb/issues/5844
- [ ] **#5791** - Web UI: disable syntax highlighting for massive JSON documents
  - https://github.com/rethinkdb/rethinkdb/issues/5791
- [ ] **#5144** - Proposal: Fast, cross-document transaction support and fast document counting via append-only databases (AODB)
  - https://github.com/rethinkdb/rethinkdb/issues/5144
- [ ] **#5143** - add doc.in(array) helper method
  - https://github.com/rethinkdb/rethinkdb/issues/5143
- [ ] **#4966** - Add support for document type inheritance
  - https://github.com/rethinkdb/rethinkdb/issues/4966
- [ ] **#4874** - Document the Polyglot test format
  - https://github.com/rethinkdb/rethinkdb/issues/4874
- [ ] **#4853** - ReQL proposal: document reference pseudotype
  - https://github.com/rethinkdb/rethinkdb/issues/4853

---

## ⚪ Other Issues (Sample of 831)

- [ ] **#7118** - Set a name to a proxy name
  - https://github.com/rethinkdb/rethinkdb/issues/7118
- [ ] **#7117** - Rethinkdb Proxy
  - https://github.com/rethinkdb/rethinkdb/issues/7117
- [ ] **#7102** - Codespell report for "RethinkDB" (on fossies.org)
  - https://github.com/rethinkdb/rethinkdb/issues/7102
- [ ] **#7089** - Error running on production, log
  - https://github.com/rethinkdb/rethinkdb/issues/7089
- [ ] **#7067** - Revisit ALL issues and PRs merged/closed for 2.5.0
  - https://github.com/rethinkdb/rethinkdb/issues/7067
- [ ] **#7005** - Guarantee failed: [get_safety_boundary() >= num_elements]
  - https://github.com/rethinkdb/rethinkdb/issues/7005
- [ ] **#7001** - Free data from ram memmory but not from disk
  - https://github.com/rethinkdb/rethinkdb/issues/7001
- [ ] **#6993** - RethinkDB v2.4.1 centos requires protobuf on aarch64
  - https://github.com/rethinkdb/rethinkdb/issues/6993
- [ ] **#6981** - Rethinkdb status
  - https://github.com/rethinkdb/rethinkdb/issues/6981
- [ ] **#6967** - Official Helm chart
  - https://github.com/rethinkdb/rethinkdb/issues/6967

---

## Completed Work (v2.4.7)

### ✅ Fixed Issues

- See NOTES.md for detailed list of fixes

### ✅ New Features Implemented

- [x] Pluggable JavaScript engine architecture
- [x] V8 JavaScript engine support (jitless mode)
- [x] QuickJS-NG, Duktape, Hermes engine support
- [x] RISC-V architecture support
- [x] Container memory limit detection (cgroups)
- [x] Security fixes for timing attacks
- [x] Updated dependencies (zlib, openssl, curl, re2)

---

## Notes

- This list contains 1339 open issues from the RethinkDB GitHub repository
- Priority levels are suggestions based on issue impact
- Check individual GitHub issues for latest status and discussion

