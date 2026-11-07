# RethinkDB MCP Server — Architecture

## Overview

The RethinkDB MCP server is a **companion Go process** that exposes RethinkDB
operations as [Model Context Protocol](https://spec.modelcontextprotocol.io/)
(MCP) tools. AI agents and applications such as Claude Desktop connect to it
and can query, modify, and stream real-time changes from a RethinkDB cluster.

```
┌─────────────────────────────────────────────────────────────────┐
│                     AI Agent / Claude Desktop                    │
└────────────────────────┬────────────────────────────────────────┘
                         │ JSON-RPC 2.0 (MCP)
              ┌──────────▼──────────┐
              │   Transport Layer    │
              │  stdio │ HTTP+SSE   │
              └──────────┬──────────┘
                         │
              ┌──────────▼──────────┐
              │  Protocol Handler   │  (per session)
              │  (JSON-RPC router)  │
              └──────────┬──────────┘
                         │
          ┌──────────────┼──────────────┐
          │              │              │
  ┌───────▼──────┐ ┌────▼────┐ ┌──────▼────────┐
  │  Auth Bridge │ │  Tool   │ │    Tool       │
  │  (SCRAM-SHA) │ │Registry │ │ Implementations│
  └───────┬──────┘ └────┬────┘ └──────┬────────┘
          │              │              │
          └──────────────▼──────────────┘
                         │ gorethink driver
              ┌──────────▼──────────┐
              │    RethinkDB Node   │
              │  (port 28015 wire   │
              │   protocol)         │
              └─────────────────────┘
```

---

## Component Reference

### `internal/config` — Configuration

Loads `[mcp]` and `[rethinkdb]` sections from a TOML file, applies CLI flag
overrides, and validates the result. Zero external dependencies beyond the
BurntSushi TOML parser.

### `internal/transport` — Transport Abstraction

**Interface** (`Transport`): `Receive(ctx) ([]byte, error)` and `Send(v) error`.

**StdioTransport**: reads newline-delimited JSON from stdin, writes to stdout.
A mutex serialises concurrent `Send` calls from the main goroutine and from
streaming notification goroutines.

**HTTPTransport** + **HTTPServer**: each POST `/mcp` request creates an
`HTTPTransport` that bridges the HTTP channel to the session handler. Clients
that send `Accept: text/event-stream` get an SSE response stream; otherwise
a single JSON response is returned.

### `internal/auth` — Authentication Bridge

`Bridge.AuthenticateWithCreds(ctx, user, pass)` opens a gorethink session using
SCRAM-SHA-256 — the exact same mechanism RethinkDB uses for all native client
connections. A lightweight ping query (`r.Now()`) confirms authentication before
returning the session.

**Permission enforcement is automatic**: every query runs as the authenticated
database user. RethinkDB's own permission layer rejects operations the user is
not authorised to perform — the MCP server never bypasses or duplicates this
logic.

### `internal/tools` — Tool Registry & Implementations

**Registry**: a simple `map[name]entry` with insertion-order preservation for
deterministic `tools/list` responses.

**ExecContext**: passed to every tool call, containing:
- `Context context.Context` — cancelled on client disconnect or timeout
- `Session *auth.Session` — authenticated gorethink session
- `Notify NotifyFunc` — sends `notifications/stream` events for changefeeds

**Tool categories**:

| File | Tools |
|------|-------|
| `database.go` | `list_databases`, `list_tables`, `describe_table` |
| `documents.go` | `query_reql`, `insert_document`, `update_document`, `delete_document` |
| `streaming.go` | `subscribe_table_changes`, `subscribe_query_changes`, `unsubscribe` |

### `internal/protocol` — MCP Protocol Handler

`Handler.Run(ctx)` is the main event loop for a session. It:
1. Reads JSON-RPC messages from the transport
2. Routes them by `method` name
3. Enforces the initialize → tools/list → tools/call ordering
4. Converts `tools.ToolError` to the correct JSON-RPC error code

---

## Authentication Flow

```
Client                         MCP Server              RethinkDB
  │                                │                       │
  │──── initialize(_auth:{..}) ───▶│                       │
  │                                │── Connect(SCRAM) ────▶│
  │                                │◀─ Auth OK ────────────│
  │◀─── InitializeResult ──────────│                       │
  │                                │                       │
  │──── tools/call(query_reql) ───▶│                       │
  │                                │── r.DB(..).Table(..) ▶│
  │                                │   (as authenticated   │
  │                                │    user; permissions  │
  │                                │    enforced by DB)    │
  │◀─── CallToolResult ────────────│                       │
```

**stdio (Claude Desktop)**: credentials come from the config file. The session
persists for the lifetime of the stdio connection.

**HTTP**: each POST `/mcp` is independent; clients must send `initialize` with
credentials on every new HTTP session (or use the config-file defaults).

---

## Streaming Architecture (Changefeeds)

RethinkDB's native changefeed capability is the core differentiator exposed
through the streaming tools.

```
tools/call(subscribe_table_changes)
         │
         ├─ Open changefeed cursor (r.DB(..).Table(..).Changes())
         ├─ Derive cancellable sub-context
         ├─ Register (subID → cancel) in subscriptionManager
         ├─ Launch background goroutine
         └─ Return {"subscriptionId": "...", "status": "subscribed"}

Background goroutine:
  loop:
    cursor.Next(&change)
    ctx.Notify("notifications/stream", {subscriptionId, type, data})
  on cancel / cursor close:
    cursor.Close()
    ctx.Notify("notifications/stream", {subscriptionId, type: "end"})
```

**Lifecycle events**:
- `change` — a document was inserted, updated, or deleted
- `state` — a RethinkDB state object (e.g. `{"state":"ready"}`)
- `error` — the cursor encountered an error; the subscription ends
- `end` — the subscription was cancelled or the cursor closed normally

**Backpressure**: cursor.Next blocks until a change is available; the
goroutine respects ctx cancellation between reads.

**Resource cleanup**: the cancel function is stored in a per-session
`subscriptionManager`. When the session context is cancelled (client
disconnect), all open cursors close automatically.

---

## Adding a New Tool

1. Choose or create a file in `internal/tools/`.
2. Write a `ToolDefinition` function and a `ToolFunc` implementation.
3. Add a `RegisterXxxTools(reg)` function and call it from `main.go`'s
   `buildRegistry()`.

Example skeleton:

```go
func myToolDef() tools.ToolDefinition {
    return tools.ToolDefinition{
        Name:        "my_tool",
        Description: "Does something useful.",
        InputSchema: inputSchema([]string{"required_field"}, map[string]interface{}{
            "required_field": prop("string", "Description of this field."),
        }),
    }
}

func myToolFn(ctx tools.ExecContext, raw json.RawMessage) (*tools.CallToolResult, error) {
    var args struct{ RequiredField string `json:"required_field"` }
    if err := json.Unmarshal(raw, &args); err != nil {
        return nil, tools.InputError("invalid arguments: %v", err)
    }
    // ... use ctx.Session.RDB() to query RethinkDB ...
    return tools.JSONResult(result)
}
```

---

## Adding a New Transport

Implement the `transport.Transport` interface:

```go
type Transport interface {
    Receive(ctx context.Context) ([]byte, error)
    Send(v interface{}) error
    Close() error
}
```

Wire it into `main.go`'s transport switch and add corresponding config/CLI
support. The protocol handler is transport-agnostic.

---

## Design Decisions

### Why a companion process, not embedded in the C++ binary?

RethinkDB's core is written in C++. Embedding a Go runtime in a C++ process
via CGo is technically possible but introduces significant complexity:
build-system integration, shared memory management, and debugging difficulty.
A companion process communicates over the well-defined native driver protocol
(already stable and TLS-capable) and can be deployed, upgraded, and debugged
independently.

### Why stdio as the primary transport?

Claude Desktop and most MCP clients use stdio by default. Stdio is inherently
single-client, which aligns with the simple security model: one session, one
set of credentials, no request routing ambiguity. HTTP+SSE is provided for
multi-client deployments.

### Why a structured query DSL instead of raw ReQL?

Raw ReQL is a programming-language API, not a string-based query language.
Accepting arbitrary strings would require implementing a ReQL parser. The
structured `query_reql` tool covers the vast majority of AI agent use cases
(filter, order, paginate, project) without parsing complexity or the
injection-attack surface that comes with string-based queries.

### Why is permission enforcement implicit?

Every query runs as the authenticated RethinkDB user via the gorethink session.
RethinkDB's own permission system rejects unauthorised operations at the
database layer, returning an error that the MCP server forwards to the client.
This avoids duplicating permission logic and ensures the MCP server can never
accidentally grant more access than the database itself permits.

### Why changefeeds for streaming instead of polling?

RethinkDB changefeeds are push-based, distributed, and real-time — they are
the database's native streaming primitive. Polling would be slower, more
resource-intensive, and would miss rapid sequences of changes. Changefeeds
are a key RethinkDB differentiator and the natural fit for MCP streaming.
