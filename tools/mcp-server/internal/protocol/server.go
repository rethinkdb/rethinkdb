// Package protocol - server.go provides the MCP request router and session
// lifecycle manager for a single client connection.
package protocol

import (
	"context"
	"encoding/json"
	"fmt"
	"log/slog"

	"github.com/rethinkdb/rethinkdb/tools/mcp-server/internal/auth"
	"github.com/rethinkdb/rethinkdb/tools/mcp-server/internal/tools"
	"github.com/rethinkdb/rethinkdb/tools/mcp-server/internal/transport"
)

const serverName = "rethinkdb-mcp"
const serverVersion = "1.0.0"

// Handler dispatches MCP JSON-RPC messages for a single client session.
type Handler struct {
	transport   transport.Transport
	registry    *tools.Registry
	authBridge  *auth.Bridge
	logger      *slog.Logger
	initialized bool
	session     *auth.Session
}

// NewHandler creates a Handler for the given transport connection.
func NewHandler(
	t transport.Transport,
	registry *tools.Registry,
	bridge *auth.Bridge,
	logger *slog.Logger,
) *Handler {
	return &Handler{
		transport:  t,
		registry:   registry,
		authBridge: bridge,
		logger:     logger,
	}
}

// Run reads messages from the transport until the context is cancelled or
// the transport closes. This is the main event loop for a session.
func (h *Handler) Run(ctx context.Context) error {
	for {
		raw, err := h.transport.Receive(ctx)
		if err != nil {
			return fmt.Errorf("transport receive: %w", err)
		}

		var req Request
		if err := json.Unmarshal(raw, &req); err != nil {
			h.sendError(nil, ErrCodeParse, "parse error", err.Error())
			continue
		}

		if req.JSONRPC != "2.0" {
			h.sendError(req.ID, ErrCodeInvalidRequest, `jsonrpc must be "2.0"`, nil)
			continue
		}

		h.logger.Debug("received message", "method", req.Method, "id", string(req.ID))
		h.dispatch(ctx, &req)
	}
}

// dispatch routes a parsed request to the appropriate handler method.
func (h *Handler) dispatch(ctx context.Context, req *Request) {
	switch req.Method {
	case "initialize":
		h.handleInitialize(ctx, req)

	case "notifications/initialized":
		// Client acknowledgement — no response required.
		h.logger.Info("client initialized")

	case "ping":
		if !req.IsNotification() {
			h.sendSuccess(req.ID, PingResult{})
		}

	case "tools/list":
		h.handleToolsList(req)

	case "tools/call":
		h.handleToolsCall(ctx, req)

	default:
		if !req.IsNotification() {
			h.sendError(req.ID, ErrCodeMethodNotFound,
				fmt.Sprintf("method %q not found", req.Method), nil)
		}
	}
}

// ---------------------------------------------------------------------------
// initialize
// ---------------------------------------------------------------------------

func (h *Handler) handleInitialize(ctx context.Context, req *Request) {
	if h.initialized {
		h.sendError(req.ID, ErrCodeInvalidRequest, "session already initialized", nil)
		return
	}

	var params InitializeParams
	if err := json.Unmarshal(req.Params, &params); err != nil {
		h.sendError(req.ID, ErrCodeInvalidParams, "invalid initialize params", err.Error())
		return
	}

	// Authenticate against RethinkDB using either the supplied credentials
	// or the server's configured defaults.
	var username, password string
	if params.Auth != nil {
		username = params.Auth.Username
		password = params.Auth.Password
	}

	session, err := h.authBridge.AuthenticateWithCreds(ctx, username, password)
	if err != nil {
		h.logger.Warn("authentication failed",
			"user", coalesce(username, "<config>"), "err", err)
		h.sendError(req.ID, ErrCodeAuthFailed, "authentication failed", err.Error())
		return
	}

	h.session = session
	h.initialized = true

	h.logger.Info("session initialized",
		"client", params.ClientInfo.Name,
		"client_version", params.ClientInfo.Version,
		"protocol_version", params.ProtocolVersion,
		"db_user", session.Username(),
	)

	h.sendSuccess(req.ID, InitializeResult{
		ProtocolVersion: ProtocolVersion,
		Capabilities:    ServerCapabilities{Tools: &ToolsCapability{ListChanged: false}},
		ServerInfo:      ServerInfo{Name: serverName, Version: serverVersion},
	})
}

// ---------------------------------------------------------------------------
// tools/list
// ---------------------------------------------------------------------------

func (h *Handler) handleToolsList(req *Request) {
	if !h.requireInitialized(req.ID) {
		return
	}
	h.sendSuccess(req.ID, ListToolsResult{Tools: h.registry.Definitions()})
}

// ---------------------------------------------------------------------------
// tools/call
// ---------------------------------------------------------------------------

func (h *Handler) handleToolsCall(ctx context.Context, req *Request) {
	if !h.requireInitialized(req.ID) {
		return
	}

	var params CallToolParams
	if err := json.Unmarshal(req.Params, &params); err != nil {
		h.sendError(req.ID, ErrCodeInvalidParams, "invalid tools/call params", err.Error())
		return
	}

	h.logger.Info("tool call", "tool", params.Name, "user", h.session.Username())

	execCtx := tools.ExecContext{
		Context: ctx,
		Session: h.session,
		// Notify wraps the transport.Send call, turning (method, params) into a
		// JSON-RPC Notification. The mutex in the transport prevents interleaving.
		Notify: func(method string, notifParams interface{}) {
			n := NewNotification(method, notifParams)
			if err := h.transport.Send(n); err != nil {
				h.logger.Warn("notification send failed", "err", err)
			}
		},
	}

	result, err := h.registry.Call(execCtx, params.Name, params.Arguments)
	if err != nil {
		h.logger.Warn("tool call failed",
			"tool", params.Name,
			"user", h.session.Username(),
			"err", err,
		)
		h.sendError(req.ID, toolErrCode(err), err.Error(), nil)
		return
	}

	h.sendSuccess(req.ID, result)
}

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

func (h *Handler) requireInitialized(id json.RawMessage) bool {
	if !h.initialized {
		h.sendError(id, ErrCodeInvalidRequest,
			"session not initialized; send initialize first", nil)
		return false
	}
	return true
}

func (h *Handler) sendSuccess(id json.RawMessage, result interface{}) {
	resp := NewSuccessResponse(id, result)
	if err := h.transport.Send(resp); err != nil {
		h.logger.Warn("send response failed", "err", err)
	}
}

func (h *Handler) sendError(id json.RawMessage, code int, msg string, data interface{}) {
	resp := NewErrorResponse(id, code, msg, data)
	if err := h.transport.Send(resp); err != nil {
		h.logger.Warn("send error response failed", "err", err)
	}
}

// toolErrCode maps a tools.ToolError kind to an MCP error code.
func toolErrCode(err error) int {
	te, ok := err.(*tools.ToolError)
	if !ok {
		return ErrCodeInternal
	}
	switch te.Kind {
	case tools.ErrKindPermission:
		return ErrCodePermissionDenied
	case tools.ErrKindNotFound:
		return ErrCodeNotFound
	case tools.ErrKindTimeout:
		return ErrCodeTimeout
	case tools.ErrKindDB:
		return ErrCodeDBError
	case tools.ErrKindInput:
		return ErrCodeInvalidParams
	default:
		return ErrCodeInternal
	}
}

func coalesce(s, fallback string) string {
	if s != "" {
		return s
	}
	return fallback
}
