// Command rethinkdb-mcp is the Model Context Protocol server for RethinkDB.
//
// It runs as a companion process alongside a RethinkDB instance, connecting
// via the native driver protocol and exposing RethinkDB operations as MCP
// tools for AI agents such as Claude Desktop.
//
// Usage:
//
//	# stdio transport (Claude Desktop)
//	rethinkdb-mcp --config /etc/rethinkdb/mcp.toml
//	rethinkdb-mcp --transport stdio --db-host 127.0.0.1 --db-port 28015
//
//	# HTTP transport
//	rethinkdb-mcp --transport http --address 127.0.0.1:8081 \
//	              --db-host 127.0.0.1 --db-port 28015 \
//	              --db-user admin --db-password secret
package main

import (
	"context"
	"flag"
	"fmt"
	"log/slog"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/rethinkdb/rethinkdb/tools/mcp-server/internal/auth"
	"github.com/rethinkdb/rethinkdb/tools/mcp-server/internal/config"
	"github.com/rethinkdb/rethinkdb/tools/mcp-server/internal/protocol"
	"github.com/rethinkdb/rethinkdb/tools/mcp-server/internal/tools"
	"github.com/rethinkdb/rethinkdb/tools/mcp-server/internal/transport"
)

func main() {
	os.Exit(run())
}

func run() int {
	// ---------------------------------------------------------------------------
	// Flag definitions — all flags override config-file values.
	// ---------------------------------------------------------------------------
	var (
		configPath  = flag.String("config", "", "Path to TOML configuration file.")
		logLevel    = flag.String("log-level", "info", "Log level: debug|info|warn|error.")
		logFormat   = flag.String("log-format", "text", "Log format: text|json.")

		// MCP flags
		mcpTransport = flag.String("transport", "", "MCP transport: stdio|http. Overrides config.")
		mcpAddress   = flag.String("address", "", "Listen address for HTTP transport (host:port).")
		mcpTimeout   = flag.Duration("timeout", 0, "Tool execution timeout. Overrides config.")

		// RethinkDB connection flags
		dbHost     = flag.String("db-host", "", "RethinkDB host. Overrides config.")
		dbPort     = flag.Int("db-port", 0, "RethinkDB driver port. Overrides config.")
		dbUser     = flag.String("db-user", "", "RethinkDB username. Overrides config.")
		dbPassword = flag.String("db-password", "", "RethinkDB password. Overrides config.")
		dbTLS      = flag.Bool("db-tls", false, "Enable TLS for the driver connection.")

		showVersion = flag.Bool("version", false, "Print version and exit.")
	)
	flag.Parse()

	if *showVersion {
		fmt.Println("rethinkdb-mcp 1.0.0")
		return 0
	}

	// ---------------------------------------------------------------------------
	// Logging
	// ---------------------------------------------------------------------------
	logger := buildLogger(*logLevel, *logFormat)

	// ---------------------------------------------------------------------------
	// Configuration
	// ---------------------------------------------------------------------------
	cfg, err := config.Load(*configPath)
	if err != nil {
		logger.Error("failed to load configuration", "err", err)
		return 1
	}

	// Apply CLI overrides.
	if *mcpTransport != "" {
		cfg.MCP.Transport = config.Transport(*mcpTransport)
	}
	if *mcpAddress != "" {
		cfg.MCP.Address = *mcpAddress
	}
	if *mcpTimeout > 0 {
		cfg.MCP.Timeout = *mcpTimeout
	}
	if *dbHost != "" {
		cfg.RethinkDB.Host = *dbHost
	}
	if *dbPort > 0 {
		cfg.RethinkDB.Port = *dbPort
	}
	if *dbUser != "" {
		cfg.RethinkDB.Username = *dbUser
	}
	if *dbPassword != "" {
		cfg.RethinkDB.Password = *dbPassword
	}
	if *dbTLS {
		cfg.RethinkDB.TLSEnabled = true
	}

	if err := cfg.Validate(); err != nil {
		logger.Error("invalid configuration", "err", err)
		return 1
	}

	if !cfg.MCP.Enabled {
		logger.Info("MCP server is disabled in configuration; exiting")
		return 0
	}

	// ---------------------------------------------------------------------------
	// Build shared components
	// ---------------------------------------------------------------------------
	bridge := auth.NewBridge(auth.BridgeConfig{
		Host:           cfg.RethinkDB.Host,
		Port:           cfg.RethinkDB.Port,
		DefaultUser:    cfg.RethinkDB.Username,
		DefaultPass:    cfg.RethinkDB.Password,
		TLS:            cfg.RethinkDB.TLSEnabled,
		MaxConnections: cfg.RethinkDB.MaxConnections,
		ConnectTimeout: 10 * time.Second,
	}, logger)

	registry := buildRegistry()

	// ---------------------------------------------------------------------------
	// Run
	// ---------------------------------------------------------------------------
	ctx, stop := signal.NotifyContext(context.Background(),
		os.Interrupt, syscall.SIGTERM)
	defer stop()

	logger.Info("starting RethinkDB MCP server",
		"transport", cfg.MCP.Transport,
		"db", cfg.RethinkDB.RethinkDBAddr(),
		"db_user", cfg.RethinkDB.Username,
	)

	switch cfg.MCP.Transport {
	case config.TransportStdio:
		if err := runStdio(ctx, cfg, bridge, registry, logger); err != nil {
			logger.Error("stdio server exited with error", "err", err)
			return 1
		}

	case config.TransportHTTP:
		if err := runHTTP(ctx, cfg, bridge, registry, logger); err != nil {
			logger.Error("HTTP server exited with error", "err", err)
			return 1
		}

	default:
		logger.Error("unknown transport", "transport", cfg.MCP.Transport)
		return 1
	}

	logger.Info("MCP server stopped")
	return 0
}

// ---------------------------------------------------------------------------
// Transport runners
// ---------------------------------------------------------------------------

func runStdio(
	ctx context.Context,
	cfg *config.Config,
	bridge *auth.Bridge,
	registry *tools.Registry,
	logger *slog.Logger,
) error {
	t := transport.NewStdioTransport()
	defer t.Close()

	handler := protocol.NewHandler(t, registry, bridge, logger)
	return handler.Run(ctx)
}

func runHTTP(
	ctx context.Context,
	cfg *config.Config,
	bridge *auth.Bridge,
	registry *tools.Registry,
	logger *slog.Logger,
) error {
	factory := func(ctx context.Context, t transport.Transport) {
		handler := protocol.NewHandler(t, registry, bridge, logger)
		if err := handler.Run(ctx); err != nil {
			logger.Debug("session ended", "err", err)
		}
	}

	srv := transport.NewHTTPServer(cfg.MCP.Address, factory, logger)
	return srv.ListenAndServe(ctx)
}

// ---------------------------------------------------------------------------
// Tool registration
// ---------------------------------------------------------------------------

func buildRegistry() *tools.Registry {
	reg := tools.NewRegistry()
	tools.RegisterDatabaseTools(reg)
	tools.RegisterDocumentTools(reg)
	tools.RegisterStreamingTools(reg)
	return reg
}

// ---------------------------------------------------------------------------
// Logging setup
// ---------------------------------------------------------------------------

func buildLogger(level, format string) *slog.Logger {
	var lvl slog.Level
	switch level {
	case "debug":
		lvl = slog.LevelDebug
	case "warn":
		lvl = slog.LevelWarn
	case "error":
		lvl = slog.LevelError
	default:
		lvl = slog.LevelInfo
	}

	opts := &slog.HandlerOptions{Level: lvl}
	var handler slog.Handler
	if format == "json" {
		handler = slog.NewJSONHandler(os.Stderr, opts)
	} else {
		handler = slog.NewTextHandler(os.Stderr, opts)
	}

	// For stdio transport, all log output must go to stderr so stdout remains
	// clean for JSON-RPC messages.
	return slog.New(handler)
}
