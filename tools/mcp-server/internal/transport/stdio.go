// Package transport - stdio.go implements newline-delimited JSON-RPC over
// stdin/stdout as required by the MCP specification for local tool integration.
package transport

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"sync"
)

// StdioTransport reads JSON messages from stdin and writes to stdout.
// Each message is a single line terminated by '\n' (NDJSON).
//
// Multiple goroutines may call Send concurrently; the implementation
// serialises writes with a mutex to prevent message interleaving.
type StdioTransport struct {
	reader  *bufio.Reader
	writer  io.Writer
	writeMu sync.Mutex
	encoder *json.Encoder
}

// NewStdioTransport returns a Transport backed by the process's standard
// streams. Pass custom readers/writers for unit testing.
func NewStdioTransport() *StdioTransport {
	return NewStdioTransportWith(os.Stdin, os.Stdout)
}

// NewStdioTransportWith creates a StdioTransport using the provided streams.
func NewStdioTransportWith(r io.Reader, w io.Writer) *StdioTransport {
	enc := json.NewEncoder(w)
	enc.SetEscapeHTML(false) // Keep JSON readable; MCP clients handle special chars.
	return &StdioTransport{
		reader:  bufio.NewReader(r),
		writer:  w,
		encoder: enc,
	}
}

// Receive reads the next newline-terminated JSON message from stdin.
// It blocks until a complete line is available or the context is cancelled.
func (t *StdioTransport) Receive(ctx context.Context) ([]byte, error) {
	type result struct {
		line []byte
		err  error
	}

	ch := make(chan result, 1)
	go func() {
		line, err := t.reader.ReadBytes('\n')
		ch <- result{line, err}
	}()

	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	case r := <-ch:
		if r.err != nil && len(r.line) == 0 {
			return nil, fmt.Errorf("stdio read: %w", r.err)
		}
		return r.line, nil
	}
}

// Send marshals v to JSON and writes it as a single newline-terminated line.
func (t *StdioTransport) Send(v interface{}) error {
	t.writeMu.Lock()
	defer t.writeMu.Unlock()
	if err := t.encoder.Encode(v); err != nil {
		return fmt.Errorf("stdio send: %w", err)
	}
	return nil
}

// Close is a no-op for stdio; the process owns the file descriptors.
func (t *StdioTransport) Close() error { return nil }
