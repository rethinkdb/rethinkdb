// Package transport defines the abstraction layer that decouples the MCP
// protocol handler from the underlying wire medium (stdio, HTTP/SSE, …).
package transport

import "context"

// Transport is the interface every wire medium must implement.
//
// A single call to Receive blocks until a complete JSON message is available
// or the context is cancelled. Send serialises the value and writes it to
// the transport's output channel.
//
// Implementations must be safe for concurrent calls to Send from multiple
// goroutines (e.g. tool goroutines sending streaming notifications while the
// main loop sends responses). Receive is called from a single goroutine only.
type Transport interface {
	// Receive blocks until a JSON-RPC message arrives, then returns the
	// raw bytes. EOF or context cancellation returns an error.
	Receive(ctx context.Context) ([]byte, error)

	// Send serialises v to JSON and writes it to the output stream.
	Send(v interface{}) error

	// Close releases all resources held by the transport.
	Close() error
}
