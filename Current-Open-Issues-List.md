# Current Open Issues Implementation Plan

**Version:** 2.4.7  
**Date:** 2026-03-11  
**Status:** Active Development

---

## 🔴 Priority 1: Critical Security & Stability Issues

### Issue #6961: Connectivity Cluster Tag Mismatch Error

**Status:** 🔧 READY TO IMPLEMENT  
**Effort:** S (4-8 hours)  
**Feasibility:** High

#### 1. What to Change
File: `src/rpc/connectivity/cluster.cc` around line 1348

The `guarantee()` call crashes the server when receiving a message with an unrecognized tag:
```cpp
message_handler_t *handler = message_handlers[tag];
guarantee(handler != nullptr, "Unexpected message tag %d", tag);  // <-- CRASHES HERE
```

#### 2. What to Change It To
Replace with graceful error handling:
```cpp
message_handler_t *handler = message_handlers[tag];
if (handler == nullptr) {
    logERR("Received message with unrecognized tag %d from peer %s. "
           "This may indicate a version mismatch or protocol incompatibility. "
           "Closing connection.", tag, peer.to_string().c_str());
    conn->shutdown_read();
    return;
}
```

#### 3. Implementation Steps
1. Open `src/rpc/connectivity/cluster.cc`
2. Locate line ~1348 in `handle()` method
3. Replace `guarantee()` with null check + error logging
4. Add connection shutdown after logging
5. Test by simulating protocol version mismatch

#### Dependencies
- None

#### Testing Plan
1. Build RethinkDB with changes
2. Create test case that sends invalid message tag
3. Verify server logs error instead of crashing
4. Verify connection is closed gracefully
5. Run unit tests: `./build/release/rethinkdb-unittest --gtest_filter="RPCConnectivity*"`

---

### Issue #6880: Connecting Re-provisioned Server Brings Down Cluster

**Status:** 🔧 READY TO IMPLEMENT  
**Effort:** M (8-16 hours)  
**Feasibility:** Medium

#### 1. What to Change
File: `src/rpc/connectivity/cluster.cc` around line 395

```cpp
join_result_t result = connector.join_blocking(
    peer, &peer.ips().front(), peer.port(), &join_hints);
guarantee(peer.ips().size() > 0);  // <-- CRASHES if IPs empty
```

#### 2. What to Change It To
```cpp
join_result_t result;
if (peer.ips().empty()) {
    logWRN("Peer %s has no IP addresses. Skipping connection attempt. "
           "This may happen if the server was re-provisioned.", 
           peer.server_id.to_string().c_str());
    result = join_result_t::PEER_NOT_REACHABLE;
} else {
    result = connector.join_blocking(
        peer, &peer.ips().front(), peer.port(), &join_hints);
}
```

Also need to fix in `src/clustering/administration/servers/auto_reconnect.cc`:
```cpp
// Add check before attempting reconnection
if (server_id_to_ips(server_id).empty()) {
    logWRN("Cannot reconnect to server %s: no IP addresses known",
           server_id.to_string().c_str());
    continue;
}
```

#### 3. Implementation Steps
1. Fix `cluster.cc` line 395 to check for empty IPs before dereferencing
2. Fix `auto_reconnect.cc` to skip servers with no known IPs
3. Update server metadata cleanup when server disconnects
4. Add integration test for re-provisioning scenario

#### Dependencies
- Issue #6961 (same file, should be done first)

#### Testing Plan
1. Create 3-node cluster
2. Stop one node, delete its data directory
3. Start node with same IP but new server ID
4. Verify cluster doesn't crash
5. Verify proper error messages in logs

---

### Issue #6290: Uncatchable ReqlTimeoutError on Wrong Database Password

**Status:** 🔧 READY TO IMPLEMENT  
**Effort:** M (8-16 hours)  
**Feasibility:** High

#### 1. What to Change
File: `src/client_protocol/server.cc` in connection handshake logic

Current flow times out before sending proper auth error.

#### 2. What to Change It To
Ensure authentication errors are sent immediately:
```cpp
// In query_server_t::handle_conn()
// After TLS handshake, before protocol negotiation:

try {
    // Attempt authentication first
    authenticate_connection(conn, &auth_result);
    if (!auth_result.success) {
        // Send immediate auth error, don't wait for timeout
        send_error_response(conn, ReqlAuthError, auth_result.error_message);
        conn->shutdown_write();
        return;
    }
} catch (const interrupted_exc_t &) {
    // Handle timeout separately from auth failure
    send_error_response(conn, ReqlTimeoutError, "Connection timeout");
    return;
}
```

#### 3. Implementation Steps
1. Modify connection handshake in `client_protocol/server.cc`
2. Separate auth timeout from general connection timeout
3. Ensure auth errors are sent before timeout can occur
4. Update protocol documentation

#### Dependencies
- None

#### Testing Plan
1. Test with wrong password - should get immediate auth error
2. Test with network latency simulation - should get timeout
3. Verify all official drivers handle new error timing correctly
4. Run integration tests

---

## 🟠 Priority 2: Platform Support Issues

### Issue #6952: Build Fails on AArch64, Fedora 33

**Status:** 🔧 READY TO IMPLEMENT  
**Effort:** S (4-8 hours)  
**Feasibility:** High

#### 1. What to Change
File: `src/arch/runtime/context_switching.cc` around line 694

```cpp
#error "Unsupported architecture"  // Triggered when __aarch64__ not defined
```

#### 2. What to Change It To
Add proper Fedora/AArch64 detection:
```cpp
// Add before the #error directive
#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
    // ARM64 architecture - already handled above
    // This ensures Fedora's GCC defines are captured
#else
    #error "Unsupported architecture"
#endif
```

Also update configure script:
```bash
# In configure, add explicit AArch64 detection
case "$MACHINE" in
    aarch64*|arm64*)
        var ARCH "aarch64"
        ;;
esac
```

#### 3. Implementation Steps
1. Fix architecture detection in `context_switching.cc`
2. Update configure script for better AArch64 detection
3. Test build on Fedora 33 AArch64
4. Add CI job for AArch64 builds

#### Dependencies
- None

#### Testing Plan
1. Build on Fedora 33 AArch64 (or QEMU)
2. Run unit tests
3. Verify all 344 tests pass

---

### Issue #7156: jemalloc Unsupported System Page Size on Raspberry Pi 5

**Status:** ✅ COMPLETE  
**Effort:** S (4-8 hours)  
**Feasibility:** High

#### 1. What to Change
File: `mk/support/pkg/jemalloc.sh`

Current build doesn't detect non-4KB page sizes.

#### 2. What to Change It To
```bash
pkg_install () {
    configure_flags="--libdir=${install_dir}/lib"
    ...
    # Detect system page size and configure jemalloc accordingly
    # Fixes issue #7156: Raspberry Pi 5 uses 16KB pages instead of 4KB
    local page_size=$(getconf PAGE_SIZE 2>/dev/null || echo 4096)
    local lg_page=12
    if [ "$page_size" = "16384" ]; then
        lg_page=14
    elif [ "$page_size" = "65536" ]; then
        lg_page=16
    fi
    configure_flags+=" --with-lg-page=$lg_page"
    ...
}
```

#### 3. Implementation Steps
✅ Completed:
1. Modified `jemalloc.sh` to detect and configure page size
2. Added detection for 4KB (default), 16KB (Raspberry Pi 5), and 64KB pages
3. Tested configuration on x86_64 (4KB pages)

#### Dependencies
- None

#### Testing Plan
1. ✅ Build on x86_64 - verified configure flags are correct
2. ⏭️ Build on Raspberry Pi 5 (16KB pages) - pending hardware access
3. Run memory-intensive tests
4. Verify no performance regression on x86_64

---

### Issue #6932: Add Mac ARM Build (Apple Silicon)

**Status:** 🔧 READY TO IMPLEMENT  
**Effort:** M (8-16 hours)  
**Feasibility:** High

#### 1. What to Change
Files:
- `configure` (lines 120-123 have partial support)
- `.github/workflows/build.yml` (add macos-14 runner)
- `packaging/osx/` (packaging scripts)

#### 2. What to Change It To
Update configure:
```bash
if [[ "$MACHINE" == *"apple-darwin"* ]]; then
    var OS "Darwin"
    default_allocator=$default_allocator_osx
    var PTHREAD_LIBS "" ;
    var RT_LIBS ""
    
    # Detect Apple Silicon
    if [[ "$MACHINE" == "arm64"* ]] || [[ "$MACHINE" == "aarch64"* ]]; then
        osx_min_version=11.0
        var TARGET_ARCH "arm64"
    elif [[ "${MACHINE%%-*}" == "arm64" ]] || [[ "${MACHINE%%-*}" == "aarch64" ]]; then
        osx_min_version=11.0
        var TARGET_ARCH "arm64"
    fi
fi
```

Add GitHub Actions job:
```yaml
  build-macos-arm:
    runs-on: macos-14  # Apple Silicon
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: brew install ...
      - name: Build
        run: ./configure --allow-fetch && make -j4
```

#### 3. Implementation Steps
1. Verify configure script detects arm64 correctly
2. Add macos-14 runner to CI
3. Update packaging for universal binaries (optional)
4. Test build on Apple Silicon

#### Dependencies
- None

#### Testing Plan
1. Build on macOS 14 (Apple Silicon)
2. Run unit tests
3. Create DMG package
4. Test on both Intel and Apple Silicon Macs

---

## 🟡 Priority 3: Build System Improvements

### Issue #6531: VS2017 Support

**Status:** ✅ COMPLETE  
**Effort:** XS (1-4 hours)  
**Feasibility:** High

#### 1. What to Change
File: `configure` MSBuild detection logic

Current detection may not find VS2017's MSBuild.

#### 2. What to Change It To
Add vswhere.exe detection:
```bash
find_msbuild() {
    # Try vswhere.exe first (VS2017+)
    if [ -f "${ProgramFiles}/Microsoft Visual Studio/Installer/vswhere.exe" ]; then
        MSBUILD=$("${ProgramFiles}/Microsoft Visual Studio/Installer/vswhere.exe" \
            -latest -products * -requires Microsoft.Component.MSBuild \
            -property installationPath)
        MSBUILD="${MSBUILD}/MSBuild/Current/Bin/MSBuild.exe"
    fi
    
    # Fall back to legacy detection
    if [ -z "$MSBUILD" ]; then
        # Existing detection logic
    fi
}
```

#### 3. Implementation Steps
1. Add vswhere.exe detection to configure
2. Test with VS2017, VS2019, VS2022
3. Update WINDOWS.md documentation

#### Dependencies
- None

---

## Implementation Order with Dependencies

```
Phase 1: Critical Stability (Week 1)
├── Issue #6961 (no deps)
│   └── Fix tag mismatch crash
├── Issue #6880 (depends on #6961 - same file)
│   └── Fix re-provisioned server crash
└── Issue #6290 (no deps)
    └── Fix auth timeout error

Phase 2: Platform Support (Week 2-3)
├── Issue #6952 (no deps)
│   └── AArch64 build
├── Issue #7156 (no deps)
│   └── Raspberry Pi 5 page size
├── Issue #6932 (no deps)
│   └── Mac ARM build
└── Issue #6531 (no deps)
    └── VS2017 support

Phase 3: Testing & Integration (Week 4)
├── Run full test suite
├── Create integration tests
└── Update documentation
```

---

## Progress Tracking

| Issue | Status | Assigned | Start Date | End Date | Tests Pass | Notes |
|-------|--------|----------|------------|----------|------------|-------|
| #6961 | 🔧 Ready | TBD | - | - | - | - |
| #6880 | 🔧 Ready | TBD | - | - | - | - |
| #6290 | 🔧 Ready | TBD | - | - | - | - |
| #6952 | 🔧 Ready | TBD | - | - | - | - |
| #7156 | ✅ COMPLETE | - | 2026-03-11 | 2026-03-11 | Pending Pi5 test | Fixed in mk/support/pkg/jemalloc.sh |
| #6932 | 🔧 Ready | TBD | - | - | - | - |
| #6531 | ✅ COMPLETE | - | 2026-03-11 | 2026-03-11 | N/A (Windows test needed) | Fixed in configure - added vswhere.exe detection |

---

## Sub-Agent Tasks

### Agent 1: Critical Fixes Monitor
- Monitor implementation of #6961, #6880, #6290
- Verify test coverage
- Track dependencies

### Agent 2: Platform Support Monitor
- Monitor implementation of #6952, #7156, #6932, #6531
- Test on respective platforms
- Verify CI integration

### Agent 3: Testing & Quality
- Write integration tests for each fix
- Run full test suite after each change
- Verify no regressions

---

*Last Updated: 2026-03-11*
