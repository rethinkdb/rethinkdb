# RethinkDB Project Overview

RethinkDB is an open-source, distributed NoSQL database designed for building realtime web applications. It stores schemaless JSON documents and provides features like automatic failover, robust fault tolerance, and continuous push of updated query results to applications without polling.

## Build and Test Commands

### Building
- Install dependencies (Ubuntu/Debian): `sudo apt-get install build-essential protobuf-compiler python3 python-is-python3 libprotobuf-dev libcurl4-openssl-dev libncurses5-dev libjemalloc-dev wget m4 g++ libssl-dev`
- Configure: `./configure --allow-fetch`
- Build: `make -j4`
- Install: `sudo make install`

### Testing
- Unit tests: `make unit`
- All tests: `make test`
- Run specific tests: `test/run --verbose --jobs <n> -H <test_type>`
  - Unit tests: `-H unit`
  - Integration tests: `-H all '!unit' '!cpplint' '!long' '!disabled'`

## Code Style Guidelines

- Use braces for all control structures: `if (...) {`, `while (...) {`, etc.
- Include headers in order: parent .hpp, C system, C++ standard, Boost (with errors.hpp first), local headers.
- Avoid non-const lvalue references except in return values.
- Do not make fields whose type is a reference type.
- Use `DISABLE_COPYING` macro for non-copyable types.
- Run `../scripts/check_style.sh` to verify style compliance.
- Suppress bogus lints with `// NOLINT(specific/category)`.

## Testing Instructions

RethinkDB has unit tests (in `src/unittest/`) and integration tests. Integration tests use scenarios and workloads:
- Scenarios: Scripts that start RethinkDB clusters and run workloads (e.g., `test/scenarios/static_cluster.py`).
- Workloads: Commands that run queries against the database (memcached or RDB protocol).
- Run tests in a clean directory: `(rm -rf scratch; mkdir scratch; cd scratch; ../scenarios/<scenario> <args>)`

## Security Considerations

- Supports TLS encryption for driver, intracluster, and web UI connections.
- Includes user authentication and permissions system (since v2.3).
- Certificate verification for TLS connections.
- No known active security issues; follow standard database security practices.

## Project Architecture

### Core Components
- **Thread Pool and Event Loop**: Cooperative coroutines for IO operations (`arch/runtime/`).
- **Networking**: TCP connections between servers, mailboxes for communication (`rpc/`).
- **Query Execution**: Parses ReQL queries, executes on tables (`rdb_protocol/`).
- **Storage Engine**: B-tree storage on disk with page cache (`btree/`, `buffer_cache/`, `serializer/`).
- **Clustering**: Distributed operations, table management, Raft consensus (`clustering/`).

### Key Modules
- `arch/`: Runtime primitives, coroutines, IO.
- `concurrency/`: Concurrency utilities.
- `rpc/`: Networking and messaging.
- `rdb_protocol/`: Query language, terms, data types.
- `clustering/`: Administration, routing, table contracts.
- `btree/`: B-tree operations.
- `buffer_cache/`: Page caching.
- `serializer/`: Log-structured serialization.

### Server Startup
- `main()` in `main.cc` delegates to `clustering/administration/main/serve.cc`.
- `do_serve()` sets up all components in order.

### Query Flow
- Client queries parsed into `ql::term_t` tree.
- Executed via `read_t`/`write_t` objects routed through `table_query_client_t`.
- Reach `store_t` for B-tree operations.
- Results returned via the same path.

### Changefeeds
- Special handling for realtime updates, pushing data from shards to clients.

### Table Configuration
- Managed via Raft consensus (`raft_member_t`).
- `table_manager_t` and `contract_coordinator_t` handle metadata and execution.

## Development Conventions

- Use C++ with specific style guidelines (see STYLE.md).
- Build system uses GNU Make with autotools-like configure.
- Dependencies fetched automatically if `--allow-fetch` is used.
- Web UI assets pre-generated in `src/gen/web_assets.cc`.
- Drivers maintained in separate repositories (Python, Ruby, JavaScript, etc.).

## Deployment Processes

- Packages available for Ubuntu, Debian, CentOS, OS X, Windows (beta).
- Snap packages supported.
- AMI for AWS.
- Source distribution via `make dist`.

## Continuous Integration

- GitHub Actions workflow builds on Ubuntu, runs cpplint, unit tests, integration tests, polyglot tests for Python, JavaScript, Ruby.
- Nightly tests run full test suite.
- Preflight checks style before building.