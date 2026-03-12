# RethinkDB Development Todo List

**Generated:** 2026-03-11  
**Version:** 2.4.7  
**Status:** Active Development

---

## 🔴 Critical Priority (Fix Immediately)

### Security Issues

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #6290 | Uncatchable ReqlTimeoutError on wrong database password | M (8-16h) | High | TBD |
| #6329 | Randomized URL Suffix for Administrative GUI | S (4-8h) | High | TBD |

### Cluster Stability (Crash Fixes)

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #6880 | Connecting re-provisioned server brings down cluster | M (8-16h) | Medium | TBD |
| #6961 | Connectivity cluster tag mismatch error | S (4-8h) | High | TBD |
| #6856 | Damaged cluster: Guarantee failed [token.has()] | L (16-40h) | Low | TBD |

---

## 🟠 High Priority (Next Sprint)

### Build System

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #6952 | Build fails on AArch64, Fedora 33 | S (4-8h) | High | TBD |
| #6932 | Add Mac ARM build (Apple Silicon) | M (8-16h) | High | TBD |
| #6936 | Windows build fails (V8) | M (8-16h) | Medium | TBD |
| #6531 | VS2017 support | XS (1-4h) | High | TBD |
| #6337 | Support building against musl libc | M (8-16h) | Medium | TBD |

### Platform Support

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #7156 | jemalloc: Unsupported system page size on Raspberry Pi 5 | S (4-8h) | High | TBD |
| #7124 | to_string on uninitialized ip_address_t on Raspberry | XS (1-4h) | High | TBD |
| #7171 | Windows prebuilt binaries outdated | M (8-16h) | High | TBD |
| #6699 | Compiling for aarch64 | S (4-8h) | High | TBD |

### Cluster & Network

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #7158 | Failed to remove node from cluster | S (4-8h) | High | TBD |
| #7131 | Cluster connect/reconnect timeout | M (8-16h) | Medium | TBD |
| #7002 | Many connections in TIME_WAIT | S (4-8h) | High | TBD |
| #6849 | Proxy loses state from cluster | M (8-16h) | Medium | TBD |

---

## 🟡 Medium Priority (Backlog)

### Performance & Memory

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #7123 | Evaluate Profile-Guided Optimization (PGO) | L (16-40h) | Medium | TBD |
| #6316 | getAll slower than get on changefeeds | M (8-16h) | Medium | TBD |
| #6151 | Performance issues vs MySQL | L (16-40h) | Low | TBD |
| #6055 | Backfill process too slow | M (8-16h) | Medium | TBD |

### Security & WebUI

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #5947 | Add authentication to WebUI | L (16-40h) | Medium | TBD |
| #3159 | Encryption and authentication for WebUI | M (8-16h) | High | TBD |

### Build System Improvements

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #6867 | Reproducible builds | M (8-16h) | High | TBD |
| #6001 | Rework linker dependency system | L (16-40h) | Medium | TBD |
| #6873 | ArchLinux build issues | S (4-8h) | High | TBD |

### Platform Support

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #6636 | FreeBSD compilation | M (8-16h) | Medium | TBD |
| #6878 | Jetson Nano support | M (8-16h) | Medium | TBD |
| #6721 | ARM32 (Cubietruck) crashes | L (16-40h) | Medium | TBD |

---

## 🟢 Low Priority (Future)

### Feature Requests

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #7142 | Support protobuf 25 | M (8-16h) | Medium | TBD |
| #4210 | Business logic security rules | XL (40+) | Low | TBD |
| #7129 | Change hard-coded cluster size (64→65K) | L (16-40h) | Low | TBD |

### Documentation

| Issue | Title | Effort | Feasibility | Assigned |
|-------|-------|--------|-------------|----------|
| #7154 | Update Website | S (4-8h) | High | TBD |
| #6995 | Document ReqlTimeoutError handling | XS (1-4h) | High | TBD |

---

## ✅ Completed (v2.4.7)

| Issue | Title | Status |
|-------|-------|--------|
| N/A | Pluggable JavaScript engine architecture | ✅ Complete |
| N/A | V8 JavaScript engine support | ✅ Complete |
| N/A | QuickJS-NG, Duktape, Hermes support | ✅ Complete |
| N/A | RISC-V architecture support | ✅ Complete |
| N/A | Container memory limit detection | ✅ Complete |
| N/A | Security fixes for timing attacks | ✅ Complete |
| N/A | Updated dependencies (zlib, openssl, curl, re2) | ✅ Complete |
| #7120 | Add Buffers from /proc/meminfo | ✅ Complete |
| #7129 | Increase max_shards to 256 | ✅ Complete |

---

## 📊 Statistics

| Category | Total | XS | S | M | L | XL |
|----------|-------|----|---|---|---|----|
| Critical | 5 | 0 | 2 | 2 | 1 | 0 |
| High | 13 | 1 | 6 | 5 | 1 | 0 |
| Medium | 11 | 0 | 1 | 6 | 4 | 0 |
| Low | 5 | 1 | 1 | 1 | 1 | 1 |
| **Total** | **34** | **2** | **10** | **14** | **7** | **1** |

**Estimated Total Effort:** 340-680 hours (approximately 9-17 developer weeks)

---

## 🎯 Recommended Sprint Plan

### Sprint 1 (Critical Stability)
- #6880 - Fix cluster crash on re-provisioned server
- #6961 - Fix tag mismatch crash
- #6290 - Fix uncatchable timeout error

### Sprint 2 (Platform Support)
- #6952 - AArch64/Fedora build
- #6932 - Mac ARM build
- #7156 - Raspberry Pi 5 page size
- #7124 - Raspberry Pi ip_address fix

### Sprint 3 (Build System)
- #6531 - VS2017 support
- #6936 - Windows V8 build (or deprecate)
- #6337 - Musl libc support
- #6867 - Reproducible builds

### Sprint 4 (Cluster Reliability)
- #7158 - Fix node removal
- #7131 - Reconnect timeout
- #7002 - TIME_WAIT connections
- #6849 - Proxy state loss

---

## 📝 Notes

- **Effort Legend:**
  - XS: 1-4 hours (quick fix)
  - S: 4-8 hours (1 day)
  - M: 8-16 hours (2-3 days)
  - L: 16-40 hours (1 week)
  - XL: 40+ hours (multiple weeks)

- **Feasibility:**
  - High: Clear path forward, existing infrastructure
  - Medium: Some unknowns, may need research
  - Low: Complex architectural changes or external blockers

- **Dependencies:**
  - Many cluster issues depend on #6880 (cluster stability)
  - Platform issues can be worked on in parallel
  - Build system improvements benefit all platforms

---

*Last Updated: 2026-03-11*
