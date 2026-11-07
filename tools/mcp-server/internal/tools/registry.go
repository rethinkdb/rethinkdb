// Package tools provides the MCP tool registry and execution infrastructure.
//
// Tools are registered once at startup and looked up by name at runtime.
// Each tool implementation receives an ExecContext that carries:
//   - The authenticated database session (with the user's permissions baked in)
//   - A cancellable context (honours MCP client disconnects / timeouts)
//   - A notification callback for streaming tools
//
// Dependency note: this package does NOT import the protocol package to avoid
// a circular dependency. Shared MCP output types (ToolDefinition, CallToolResult)
// are defined here and imported by the protocol package.
package tools

import (
	"context"
	"encoding/json"
	"fmt"

	"github.com/rethinkdb/rethinkdb/tools/mcp-server/internal/auth"
)

// ---------------------------------------------------------------------------
// MCP tool result types (owned by this package; imported by protocol)
// ---------------------------------------------------------------------------

// ToolDefinition describes a single callable tool exposed via MCP.
type ToolDefinition struct {
	Name        string      `json:"name"`
	Description string      `json:"description"`
	InputSchema interface{} `json:"inputSchema"`
}

// ContentItem is a single piece of content in a tool result.
type ContentItem struct {
	Type string `json:"type"` // "text" | "image" | "resource"
	Text string `json:"text,omitempty"`
}

// CallToolResult is the value returned by a tool and forwarded to the client.
type CallToolResult struct {
	Content []ContentItem `json:"content"`
	IsError bool          `json:"isError,omitempty"`
}

// TextContent is a convenience constructor for a text content item.
func TextContent(text string) ContentItem { return ContentItem{Type: "text", Text: text} }

// ---------------------------------------------------------------------------
// Error types
// ---------------------------------------------------------------------------

// ErrKind categorises tool errors so the protocol layer can map them to
// appropriate JSON-RPC error codes.
type ErrKind int

const (
	ErrKindGeneric    ErrKind = iota
	ErrKindPermission         // permission denied
	ErrKindNotFound           // resource not found
	ErrKindTimeout            // deadline exceeded
	ErrKindDB                 // database-level error
	ErrKindInput              // invalid input from the caller
)

// ToolError is a structured error returned by tool implementations.
type ToolError struct {
	Kind    ErrKind
	Message string
}

func (e *ToolError) Error() string { return e.Message }

func toolErr(kind ErrKind, format string, args ...interface{}) *ToolError {
	return &ToolError{Kind: kind, Message: fmt.Sprintf(format, args...)}
}

// DBError wraps a database-level error.
func DBError(err error) *ToolError { return toolErr(ErrKindDB, "database error: %v", err) }

// InputError signals that the caller provided invalid arguments.
func InputError(format string, args ...interface{}) *ToolError {
	return toolErr(ErrKindInput, format, args...)
}

// NotFoundError signals that the requested resource does not exist.
func NotFoundError(format string, args ...interface{}) *ToolError {
	return toolErr(ErrKindNotFound, format, args...)
}

// ---------------------------------------------------------------------------
// ExecContext
// ---------------------------------------------------------------------------

// NotifyFunc is the callback a streaming tool uses to push events to the
// MCP client. method is the JSON-RPC notification method name;
// params will be JSON-serialized by the transport layer.
type NotifyFunc func(method string, params interface{})

// ExecContext bundles every resource a tool needs during execution.
type ExecContext struct {
	// Context is cancelled when the MCP client disconnects or a timeout fires.
	Context context.Context
	// Session is the authenticated RethinkDB session for this MCP client.
	Session *auth.Session
	// Notify sends an MCP notification to the client (used by streaming tools).
	Notify NotifyFunc
}

// ---------------------------------------------------------------------------
// Tool function type
// ---------------------------------------------------------------------------

// ToolFunc is the signature every tool implementation must satisfy.
type ToolFunc func(ctx ExecContext, args json.RawMessage) (*CallToolResult, error)

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------

type entry struct {
	def ToolDefinition
	fn  ToolFunc
}

// Registry maps tool names to their definitions and implementations.
type Registry struct {
	entries map[string]entry
	order   []string // insertion order for deterministic tools/list responses
}

// NewRegistry creates an empty Registry.
func NewRegistry() *Registry {
	return &Registry{entries: make(map[string]entry)}
}

// Register adds a tool. Panics if the name is already taken.
func (reg *Registry) Register(def ToolDefinition, fn ToolFunc) {
	if _, exists := reg.entries[def.Name]; exists {
		panic(fmt.Sprintf("tool %q already registered", def.Name))
	}
	reg.entries[def.Name] = entry{def: def, fn: fn}
	reg.order = append(reg.order, def.Name)
}

// Definitions returns all tool definitions in registration order.
func (reg *Registry) Definitions() []ToolDefinition {
	defs := make([]ToolDefinition, 0, len(reg.order))
	for _, name := range reg.order {
		defs = append(defs, reg.entries[name].def)
	}
	return defs
}

// Call executes the named tool with the given raw JSON arguments.
func (reg *Registry) Call(ctx ExecContext, name string, args json.RawMessage) (*CallToolResult, error) {
	e, ok := reg.entries[name]
	if !ok {
		return nil, toolErr(ErrKindNotFound, "unknown tool %q", name)
	}
	return e.fn(ctx, args)
}

// ---------------------------------------------------------------------------
// Helpers for building tool results
// ---------------------------------------------------------------------------

// TextResult wraps a plain text string in a CallToolResult.
func TextResult(text string) *CallToolResult {
	return &CallToolResult{Content: []ContentItem{TextContent(text)}}
}

// JSONResult marshals v to pretty-printed JSON and wraps it.
func JSONResult(v interface{}) (*CallToolResult, error) {
	data, err := json.MarshalIndent(v, "", "  ")
	if err != nil {
		return nil, fmt.Errorf("marshal result: %w", err)
	}
	return TextResult(string(data)), nil
}

// ErrorResult returns a CallToolResult signalling a tool-level error.
func ErrorResult(msg string) *CallToolResult {
	return &CallToolResult{Content: []ContentItem{TextContent(msg)}, IsError: true}
}

// ---------------------------------------------------------------------------
// JSON Schema helpers used by tool definitions
// ---------------------------------------------------------------------------

func inputSchema(required []string, properties map[string]interface{}) map[string]interface{} {
	s := map[string]interface{}{
		"type":       "object",
		"properties": properties,
	}
	if len(required) > 0 {
		s["required"] = required
	}
	return s
}

func prop(typ, desc string) map[string]interface{} {
	return map[string]interface{}{"type": typ, "description": desc}
}

func propEnum(desc string, values []string) map[string]interface{} {
	return map[string]interface{}{
		"type":        "string",
		"description": desc,
		"enum":        values,
	}
}
