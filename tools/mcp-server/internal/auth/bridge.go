// Package auth provides the authentication bridge between the MCP server and
// RethinkDB's native auth system.
//
// RethinkDB uses SCRAM-SHA-256 for authentication over the binary driver
// protocol. This package wraps the Go driver's session lifecycle so that:
//
//  1. Each MCP session gets its own authenticated database session.
//  2. Per-user permissions are enforced at the database layer — the MCP server
//     never bypasses RethinkDB's permission model.
//  3. Credentials supplied by the MCP client take precedence over the server's
//     configured defaults, enabling multi-user HTTP deployments.
package auth

import (
	"context"
	"fmt"
	"log/slog"
	"time"

	r "gopkg.in/rethinkdb/rethinkdb-go.v6"
)

// BridgeConfig holds connection parameters for the default (config-file)
// database user.
type BridgeConfig struct {
	Host           string
	Port           int
	DefaultUser    string
	DefaultPass    string
	TLS            bool
	MaxConnections int
	ConnectTimeout time.Duration
}

// Bridge creates authenticated RethinkDB sessions for MCP clients.
type Bridge struct {
	cfg    BridgeConfig
	logger *slog.Logger
}

// NewBridge creates a Bridge with the given configuration.
func NewBridge(cfg BridgeConfig, logger *slog.Logger) *Bridge {
	return &Bridge{cfg: cfg, logger: logger}
}

// ---------------------------------------------------------------------------
// Session
// ---------------------------------------------------------------------------

// Session wraps a gorethink session and exposes the authenticated username.
// All tool operations receive a *Session so they can run queries under the
// correct user context.
type Session struct {
	inner    *r.Session
	username string
}

// RDB returns the underlying gorethink session for running queries.
func (s *Session) RDB() *r.Session { return s.inner }

// Username returns the database username for this session.
func (s *Session) Username() string { return s.username }

// Close releases the database connection.
func (s *Session) Close() error {
	if s.inner != nil {
		return s.inner.Close()
	}
	return nil
}

// ---------------------------------------------------------------------------
// Authenticate
// ---------------------------------------------------------------------------

// AuthenticateWithCreds opens a session using the supplied username/password.
// Pass empty strings to fall back to the configured defaults.
func (b *Bridge) AuthenticateWithCreds(ctx context.Context, username, password string) (*Session, error) {
	user, pass := b.cfg.DefaultUser, b.cfg.DefaultPass
	if username != "" {
		user = username
		pass = password
	}
	return b.openSession(ctx, user, pass)
}

// openSession establishes and authenticates a gorethink session.
func (b *Bridge) openSession(ctx context.Context, username, password string) (*Session, error) {
	addr := fmt.Sprintf("%s:%d", b.cfg.Host, b.cfg.Port)

	timeout := b.cfg.ConnectTimeout
	if timeout == 0 {
		timeout = 10 * time.Second
	}

	opts := r.ConnectOpts{
		Address:    addr,
		Username:   username,
		Password:   password,
		MaxOpen:    b.cfg.MaxConnections,
		NumRetries: 0, // fail fast; let the caller retry
		Timeout:    timeout,
	}

	// Use the context deadline if it is tighter than our default.
	if dl, ok := ctx.Deadline(); ok {
		remaining := time.Until(dl)
		if remaining < timeout {
			opts.Timeout = remaining
		}
	}

	b.logger.Debug("opening RethinkDB session",
		"addr", addr, "user", username)

	session, err := r.Connect(opts)
	if err != nil {
		return nil, fmt.Errorf("RethinkDB connect (%s@%s): %w", username, addr, err)
	}

	// Verify authentication with a lightweight ping query.
	if err := b.ping(ctx, session); err != nil {
		_ = session.Close()
		return nil, fmt.Errorf("RethinkDB auth verify (%s@%s): %w", username, addr, err)
	}

	b.logger.Info("RethinkDB session opened",
		"addr", addr, "user", username)

	return &Session{inner: session, username: username}, nil
}

// ping runs r.Now() to confirm the session is live and authenticated.
func (b *Bridge) ping(ctx context.Context, session *r.Session) error {
	cursor, err := r.Now().Run(session)
	if err != nil {
		return err
	}
	return cursor.Close()
}

