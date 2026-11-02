# RethinkDB MCP Server

A production-grade [Model Context Protocol](https://spec.modelcontextprotocol.io/)
(MCP) server that exposes RethinkDB operations as AI-callable tools.

Connect AI agents and applications like **Claude Desktop** to your RethinkDB
cluster and let them query, modify, and subscribe to real-time data streams
through a standardised protocol — all with full RethinkDB permission enforcement.

---

## Quick Start

### Prerequisites

- Go 1.21+
- A running RethinkDB instance

### Build

```bash
cd rethinkdb/tools/mcp-server
go build -o rethinkdb-mcp ./cmd/rethinkdb-mcp
```

### Run (stdio — Claude Desktop)

```bash
./rethinkdb-mcp \
  --transport stdio \
  --db-host 127.0.0.1 \
  --db-port 28015 \
  --db-user admin \
  --db-password ""
```

### Run (HTTP — multi-client)

```bash
./rethinkdb-mcp \
  --transport http \
  --address 127.0.0.1:8081 \
  --db-host 127.0.0.1 \
  --db-port 28015
```

Health probe: `GET http://127.0.0.1:8081/mcp/health`

---

## Claude Desktop Integration

Add this to your `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "rethinkdb": {
      "command": "/path/to/rethinkdb-mcp",
      "args": [
        "--transport", "stdio",
        "--db-host", "127.0.0.1",
        "--db-port", "28015",
        "--db-user", "admin",
        "--db-password", ""
      ],
      "env": {}
    }
  }
}
```

Per-session credentials (for multi-user deployments) can be supplied in the
MCP `initialize` request via the `_auth` extension field:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "initialize",
  "params": {
    "protocolVersion": "2024-11-05",
    "capabilities": {},
    "clientInfo": {"name": "my-agent", "version": "1.0"},
    "_auth": {"username": "alice", "password": "s3cret"}
  }
}
```

---

## Available Tools

### Introspection

| Tool | Description |
|------|-------------|
| `list_databases` | List all databases the authenticated user can access |
| `list_tables` | List tables in a database |
| `describe_table` | Table metadata: primary key, indexes, config, status |

### CRUD

| Tool | Description |
|------|-------------|
| `query_reql` | Structured read query (filter, order, paginate, project) |
| `insert_document` | Insert a document (auto-generates UUID if no id given) |
| `update_document` | Merge fields into an existing document by primary key |
| `delete_document` | Delete a document by primary key |

### Streaming (Changefeeds)

| Tool | Description |
|------|-------------|
| `subscribe_table_changes` | Stream all changes to a table |
| `subscribe_query_changes` | Stream changes matching an equality filter |
| `unsubscribe` | Cancel an active subscription |

Streaming tools return a `subscriptionId` immediately. Changes arrive as
`notifications/stream` JSON-RPC notifications:

```json
{
  "jsonrpc": "2.0",
  "method": "notifications/stream",
  "params": {
    "subscriptionId": "a1b2c3d4-...",
    "type": "change",
    "data": {
      "new_val": {"id": "xyz", "status": "active"},
      "old_val": {"id": "xyz", "status": "pending"}
    }
  }
}
```

---

## Configuration

Copy `configs/example.toml` and pass it with `--config`:

```toml
[mcp]
enabled   = true
transport = "stdio"
address   = "127.0.0.1:8081"
timeout   = "30s"

[rethinkdb]
host            = "127.0.0.1"
port            = 28015
username        = "admin"
password        = ""
tls             = false
max_connections = 10
```

All fields can be overridden with CLI flags; run `rethinkdb-mcp --help`.

---

## Security

- **Authentication mirrors RethinkDB exactly**: credentials are passed to the
  database using SCRAM-SHA-256 (the same protocol used by all official RethinkDB
  drivers and the Web UI).
- **Permission enforcement is automatic**: every query runs as the authenticated
  user. RethinkDB's own permission layer rejects unauthorised operations.
- **No privilege escalation**: the MCP server cannot grant more access than the
  database itself permits.
- **Input validation**: all tool arguments are validated before any database
  call is made.
- **Query limits**: `query_reql` caps results at 10 000 documents by default to
  prevent resource exhaustion.
- **Log output** goes to **stderr** only, keeping stdout clean for JSON-RPC
  messages when using the stdio transport.

---

## Architecture

See [`docs/architecture.md`](docs/architecture.md) for a full description of
the component relationships, authentication flow, streaming design, and
extension points.

---

## Development

```bash
# Run tests
go test ./...

# Build and run against a local RethinkDB
go run ./cmd/rethinkdb-mcp --transport stdio --log-level debug
```

Logging uses Go's `log/slog`. Pass `--log-format json` for structured output
suitable for log aggregation systems.
