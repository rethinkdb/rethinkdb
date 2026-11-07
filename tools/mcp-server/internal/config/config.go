// Package config handles configuration loading and validation for the
// RethinkDB MCP server. Supports TOML files and CLI flag overrides.
package config

import (
	"errors"
	"fmt"
	"os"
	"time"

	"github.com/BurntSushi/toml"
)

// Transport specifies the MCP transport mechanism.
type Transport string

const (
	TransportStdio Transport = "stdio"
	TransportHTTP  Transport = "http"
)

// MCPConfig holds Model Context Protocol server settings.
type MCPConfig struct {
	Enabled   bool      `toml:"enabled"`
	Transport Transport `toml:"transport"`
	// Address is only used when Transport == TransportHTTP.
	Address string `toml:"address"`
	// Timeout for individual tool executions.
	Timeout time.Duration `toml:"timeout"`
}

// RethinkDBConfig holds connection parameters for the backing database.
type RethinkDBConfig struct {
	Host     string `toml:"host"`
	Port     int    `toml:"port"`
	Username string `toml:"username"`
	Password string `toml:"password"`
	// TLSEnabled enables TLS for the driver connection.
	TLSEnabled bool `toml:"tls"`
	// MaxConnections is the size of the connection pool (HTTP transport).
	MaxConnections int `toml:"max_connections"`
}

// Config is the top-level configuration structure.
type Config struct {
	MCP       MCPConfig       `toml:"mcp"`
	RethinkDB RethinkDBConfig `toml:"rethinkdb"`
}

// defaults returns a Config with reasonable defaults.
func defaults() Config {
	return Config{
		MCP: MCPConfig{
			Enabled:   true,
			Transport: TransportStdio,
			Address:   "127.0.0.1:8081",
			Timeout:   30 * time.Second,
		},
		RethinkDB: RethinkDBConfig{
			Host:           "127.0.0.1",
			Port:           28015,
			Username:       "admin",
			Password:       "",
			TLSEnabled:     false,
			MaxConnections: 10,
		},
	}
}

// Load reads a TOML config file. Fields not present in the file retain
// their default values. Pass an empty path to use defaults only.
func Load(path string) (*Config, error) {
	cfg := defaults()
	if path == "" {
		return &cfg, nil
	}

	data, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("reading config file %q: %w", path, err)
	}

	if _, err := toml.Decode(string(data), &cfg); err != nil {
		return nil, fmt.Errorf("parsing config file %q: %w", path, err)
	}

	return &cfg, nil
}

// Validate checks that all required fields are present and consistent.
func (c *Config) Validate() error {
	if !c.MCP.Enabled {
		return nil // nothing else to check
	}

	switch c.MCP.Transport {
	case TransportStdio, TransportHTTP:
	default:
		return fmt.Errorf("unknown transport %q; must be %q or %q",
			c.MCP.Transport, TransportStdio, TransportHTTP)
	}

	if c.MCP.Transport == TransportHTTP && c.MCP.Address == "" {
		return errors.New("mcp.address must be set when transport is http")
	}

	if c.RethinkDB.Host == "" {
		return errors.New("rethinkdb.host must not be empty")
	}

	if c.RethinkDB.Port <= 0 || c.RethinkDB.Port > 65535 {
		return fmt.Errorf("rethinkdb.port %d is out of valid range 1-65535", c.RethinkDB.Port)
	}

	if c.MCP.Timeout <= 0 {
		return errors.New("mcp.timeout must be positive")
	}

	return nil
}

// RethinkDBAddr returns a "host:port" address string.
func (c *RethinkDBConfig) RethinkDBAddr() string {
	return fmt.Sprintf("%s:%d", c.Host, c.Port)
}
