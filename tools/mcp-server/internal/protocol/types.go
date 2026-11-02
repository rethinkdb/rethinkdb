// Package protocol implements the Model Context Protocol (MCP) over JSON-RPC 2.0.
//
// Specification reference: https://spec.modelcontextprotocol.io/specification/
// Protocol version: 2024-11-05
//
// Dependency note: types defined in the tools package (ToolDefinition,
// CallToolResult) are referenced here by importing tools to avoid duplication.
// The tools package does NOT import protocol, keeping the dependency graph acyclic.
package protocol

import "encoding/json"

// ProtocolVersion is the MCP specification version this server implements.
const ProtocolVersion = "2024-11-05"

// ---------------------------------------------------------------------------
// JSON-RPC 2.0 base types
// ---------------------------------------------------------------------------

// Request is an inbound JSON-RPC call that expects a response.
type Request struct {
	JSONRPC string          `json:"jsonrpc"`
	ID      json.RawMessage `json:"id,omitempty"` // string, number, or null
	Method  string          `json:"method"`
	Params  json.RawMessage `json:"params,omitempty"`
}

// IsNotification returns true when the request carries no ID (fire-and-forget).
func (r *Request) IsNotification() bool {
	return len(r.ID) == 0 || string(r.ID) == "null"
}

// Response is an outbound JSON-RPC reply to a Request.
type Response struct {
	JSONRPC string          `json:"jsonrpc"`
	ID      json.RawMessage `json:"id"`
	Result  interface{}     `json:"result,omitempty"`
	Error   *RPCError       `json:"error,omitempty"`
}

// Notification is an outbound JSON-RPC message with no ID.
type Notification struct {
	JSONRPC string      `json:"jsonrpc"`
	Method  string      `json:"method"`
	Params  interface{} `json:"params,omitempty"`
}

// RPCError represents a JSON-RPC error object.
type RPCError struct {
	Code    int         `json:"code"`
	Message string      `json:"message"`
	Data    interface{} `json:"data,omitempty"`
}

func (e *RPCError) Error() string { return e.Message }

// Standard JSON-RPC error codes.
const (
	ErrCodeParse          = -32700
	ErrCodeInvalidRequest = -32600
	ErrCodeMethodNotFound = -32601
	ErrCodeInvalidParams  = -32602
	ErrCodeInternal       = -32603

	// RethinkDB-specific server errors.
	ErrCodeAuthFailed       = -32001
	ErrCodeDBError          = -32002
	ErrCodePermissionDenied = -32003
	ErrCodeTimeout          = -32004
	ErrCodeNotFound         = -32005
)

// NewErrorResponse builds a Response that carries an error.
func NewErrorResponse(id json.RawMessage, code int, msg string, data interface{}) *Response {
	return &Response{
		JSONRPC: "2.0",
		ID:      id,
		Error:   &RPCError{Code: code, Message: msg, Data: data},
	}
}

// NewSuccessResponse builds a successful Response.
func NewSuccessResponse(id json.RawMessage, result interface{}) *Response {
	return &Response{JSONRPC: "2.0", ID: id, Result: result}
}

// NewNotification creates an outbound notification.
func NewNotification(method string, params interface{}) *Notification {
	return &Notification{JSONRPC: "2.0", Method: method, Params: params}
}

// ---------------------------------------------------------------------------
// MCP initialize / initialized
// ---------------------------------------------------------------------------

// ClientInfo describes the connecting client.
type ClientInfo struct {
	Name    string `json:"name"`
	Version string `json:"version"`
}

// ServerInfo describes this server.
type ServerInfo struct {
	Name    string `json:"name"`
	Version string `json:"version"`
}

// ClientCapabilities advertises what the client supports.
type ClientCapabilities struct {
	Tools     *ToolsCapability `json:"tools,omitempty"`
	Resources interface{}      `json:"resources,omitempty"`
}

// ServerCapabilities advertises what the server supports.
type ServerCapabilities struct {
	Tools *ToolsCapability `json:"tools,omitempty"`
}

// ToolsCapability signals tool support.
type ToolsCapability struct {
	ListChanged bool `json:"listChanged"`
}

// InitializeParams are the parameters of the initialize request.
type InitializeParams struct {
	ProtocolVersion string             `json:"protocolVersion"`
	Capabilities    ClientCapabilities `json:"capabilities"`
	ClientInfo      ClientInfo         `json:"clientInfo"`
	// Auth is a RethinkDB extension: credentials to authenticate against the
	// database. When omitted the server uses the config-file credentials.
	Auth *AuthCredentials `json:"_auth,omitempty"`
}

// AuthCredentials carries a RethinkDB username/password pair.
type AuthCredentials struct {
	Username string `json:"username"`
	Password string `json:"password"`
}

// InitializeResult is the server's response to initialize.
type InitializeResult struct {
	ProtocolVersion string             `json:"protocolVersion"`
	Capabilities    ServerCapabilities `json:"capabilities"`
	ServerInfo      ServerInfo         `json:"serverInfo"`
}

// ---------------------------------------------------------------------------
// MCP tools/list
// ---------------------------------------------------------------------------

// ListToolsResult is returned by tools/list.
// Uses tools.ToolDefinition, imported via the server.go file.
type ListToolsResult struct {
	// Tools is populated from the registry and serialised as-is.
	Tools interface{} `json:"tools"`
}

// ---------------------------------------------------------------------------
// MCP tools/call
// ---------------------------------------------------------------------------

// CallToolParams are the parameters of a tools/call request.
type CallToolParams struct {
	Name      string          `json:"name"`
	Arguments json.RawMessage `json:"arguments,omitempty"`
}

// ---------------------------------------------------------------------------
// MCP ping
// ---------------------------------------------------------------------------

// PingResult is returned by ping.
type PingResult struct{}
