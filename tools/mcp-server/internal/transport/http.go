// Package transport - http.go implements the HTTP+SSE transport for the MCP
// server. Each HTTP client connection gets its own session handler.
//
// Endpoints:
//   POST /mcp        - accepts a JSON-RPC request body, returns a JSON-RPC
//                      response. For streaming notifications the response
//                      begins an SSE stream; otherwise it is a plain JSON
//                      response.
//   GET  /mcp/health - returns 200 OK (liveness probe).
package transport

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"log/slog"
	"net"
	"net/http"
	"sync"
)

// HTTPTransport implements Transport for a single HTTP request/response pair.
// It is created per-request by the HTTPServer and passed to the MCP handler.
type HTTPTransport struct {
	incoming chan []byte
	outgoing chan interface{}
	done     chan struct{}
	closeOnce sync.Once
}

func newHTTPTransport() *HTTPTransport {
	return &HTTPTransport{
		incoming: make(chan []byte, 1),
		outgoing: make(chan interface{}, 32),
		done:     make(chan struct{}),
	}
}

func (t *HTTPTransport) Receive(ctx context.Context) ([]byte, error) {
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	case <-t.done:
		return nil, io.EOF
	case raw := <-t.incoming:
		return raw, nil
	}
}

func (t *HTTPTransport) Send(v interface{}) error {
	select {
	case t.outgoing <- v:
		return nil
	case <-t.done:
		return fmt.Errorf("transport closed")
	}
}

func (t *HTTPTransport) Close() error {
	t.closeOnce.Do(func() { close(t.done) })
	return nil
}

// ---------------------------------------------------------------------------
// HTTP server
// ---------------------------------------------------------------------------

// SessionFactory is a function that creates and runs an MCP protocol handler
// for a given transport. The HTTPServer calls it for every new connection.
type SessionFactory func(ctx context.Context, t Transport)

// HTTPServer listens on a TCP address and serves the MCP HTTP transport.
type HTTPServer struct {
	addr    string
	factory SessionFactory
	logger  *slog.Logger
	server  *http.Server
}

// NewHTTPServer creates an HTTPServer bound to addr.
func NewHTTPServer(addr string, factory SessionFactory, logger *slog.Logger) *HTTPServer {
	hs := &HTTPServer{
		addr:    addr,
		factory: factory,
		logger:  logger,
	}
	mux := http.NewServeMux()
	mux.HandleFunc("/mcp", hs.handleMCP)
	mux.HandleFunc("/mcp/health", handleHealth)
	hs.server = &http.Server{
		Addr:    addr,
		Handler: mux,
	}
	return hs
}

// ListenAndServe starts the HTTP server. It blocks until the context is
// cancelled, then performs a graceful shutdown.
func (hs *HTTPServer) ListenAndServe(ctx context.Context) error {
	ln, err := net.Listen("tcp", hs.addr)
	if err != nil {
		return fmt.Errorf("http listen %s: %w", hs.addr, err)
	}

	hs.logger.Info("MCP HTTP transport listening", "addr", hs.addr)

	errCh := make(chan error, 1)
	go func() { errCh <- hs.server.Serve(ln) }()

	select {
	case <-ctx.Done():
		return hs.server.Shutdown(context.Background())
	case err := <-errCh:
		return err
	}
}

// handleMCP processes a single POST /mcp request.
//
// The HTTP request body contains one JSON-RPC message. The response is either:
//   - A plain JSON-RPC response (Content-Type: application/json) for
//     non-streaming tool calls.
//   - A text/event-stream (SSE) response for streaming subscriptions.
//
// Authentication note: the MCP initialize request must be the first message
// in each HTTP session. Because HTTP is connectionless, clients are expected
// to include credentials in every initialize call or use session tokens
// (future work).
func (hs *HTTPServer) handleMCP(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}

	body, err := io.ReadAll(io.LimitReader(r.Body, 4<<20)) // 4 MiB max
	if err != nil {
		http.Error(w, "read body: "+err.Error(), http.StatusBadRequest)
		return
	}

	t := newHTTPTransport()

	// Deliver the request body as the first (and likely only) inbound message.
	t.incoming <- body

	ctx, cancel := context.WithCancel(r.Context())
	defer cancel()

	// Detect whether the client can accept SSE (streaming).
	wantsSSE := r.Header.Get("Accept") == "text/event-stream"

	if wantsSSE {
		hs.serveSSE(ctx, w, t)
	} else {
		hs.serveJSON(ctx, w, t)
	}
}

// serveJSON runs the session factory and writes a single JSON response.
func (hs *HTTPServer) serveJSON(ctx context.Context, w http.ResponseWriter, t *HTTPTransport) {
	ctx, cancel := context.WithCancel(ctx)
	go func() {
		hs.factory(ctx, t)
		cancel()
	}()

	// Wait for the first outgoing message.
	var resp interface{}
	select {
	case resp = <-t.outgoing:
	case <-ctx.Done():
		http.Error(w, "handler closed prematurely", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	if err := json.NewEncoder(w).Encode(resp); err != nil {
		hs.logger.Warn("http json write failed", "err", err)
	}
	t.Close()
}

// serveSSE streams JSON-RPC messages as Server-Sent Events.
func (hs *HTTPServer) serveSSE(ctx context.Context, w http.ResponseWriter, t *HTTPTransport) {
	flusher, ok := w.(http.Flusher)
	if !ok {
		http.Error(w, "streaming not supported", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")
	w.Header().Set("X-Accel-Buffering", "no")

	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	go hs.factory(ctx, t)

	bw := bufio.NewWriter(w)
	for {
		select {
		case <-ctx.Done():
			return
		case msg, ok := <-t.outgoing:
			if !ok {
				return
			}
			data, err := json.Marshal(msg)
			if err != nil {
				continue
			}
			fmt.Fprintf(bw, "data: %s\n\n", data)
			if err := bw.Flush(); err != nil {
				return
			}
			flusher.Flush()
		}
	}
}

func handleHealth(w http.ResponseWriter, _ *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	fmt.Fprint(w, `{"status":"ok"}`)
}

