// Package tools - streaming.go implements RethinkDB changefeed-backed
// streaming tools:
//
//   - subscribe_table_changes  — streams all changes to a table
//   - subscribe_query_changes  — streams changes matching a filter
//   - unsubscribe              — cancels an active subscription
//
// # Architecture
//
// Each subscription creates a dedicated goroutine that:
//  1. Opens a RethinkDB changefeed cursor using the authenticated session
//  2. Calls ctx.Notify for every change event
//  3. Terminates when the session context is cancelled (client disconnect),
//     unsubscribe is called, or the cursor closes
//
// # Notification format
//
// The transport layer receives (method, params) pairs and serialises them
// as JSON-RPC notifications:
//
//	{
//	  "jsonrpc": "2.0",
//	  "method":  "notifications/stream",
//	  "params": {
//	    "subscriptionId": "<uuid>",
//	    "type":           "change" | "state" | "error" | "end",
//	    "data":           { ... }
//	  }
//	}
package tools

import (
	"context"
	"crypto/rand"
	"encoding/json"
	"fmt"
	"sync"

	r "gopkg.in/rethinkdb/rethinkdb-go.v6"
)

// RegisterStreamingTools adds changefeed subscription tools to the registry.
func RegisterStreamingTools(reg *Registry) {
	reg.Register(subscribeTableChangesDef(), subscribeTableChangesFn)
	reg.Register(subscribeQueryChangesDef(), subscribeQueryChangesFn)
	reg.Register(unsubscribeDef(), unsubscribeFn)
}

// streamParams is the payload of a notifications/stream notification.
// Defined here (not in protocol) to avoid a circular import.
type streamParams struct {
	SubscriptionID string      `json:"subscriptionId"`
	Type           string      `json:"type"` // "change" | "state" | "error" | "end"
	Data           interface{} `json:"data,omitempty"`
}

// ---------------------------------------------------------------------------
// Subscription manager
// ---------------------------------------------------------------------------

type subscriptionManager struct {
	mu   sync.Mutex
	subs map[string]context.CancelFunc
}

// globalSubs maps session pointer (as string key) to its subscriptionManager.
var globalSubs sync.Map

func getOrCreateManager(key string) *subscriptionManager {
	v, _ := globalSubs.LoadOrStore(key, &subscriptionManager{
		subs: make(map[string]context.CancelFunc),
	})
	return v.(*subscriptionManager)
}

// ---------------------------------------------------------------------------
// subscribe_table_changes
// ---------------------------------------------------------------------------

type subscribeTableArgs struct {
	Database       string      `json:"database"`
	Table          string      `json:"table"`
	IncludeInitial bool        `json:"include_initial,omitempty"`
	IncludeStates  bool        `json:"include_states,omitempty"`
	Squash         interface{} `json:"squash,omitempty"`
}

func subscribeTableChangesDef() ToolDefinition {
	return ToolDefinition{
		Name: "subscribe_table_changes",
		Description: `Subscribe to real-time changes on a RethinkDB table using a native changefeed.
Returns a subscriptionId immediately. Subsequent changes arrive as
"notifications/stream" MCP notifications until unsubscribe is called.
This leverages RethinkDB's unique distributed changefeed capability.`,
		InputSchema: inputSchema(
			[]string{"database", "table"},
			map[string]interface{}{
				"database":        prop("string", "Database name."),
				"table":           prop("string", "Table to watch for changes."),
				"include_initial": map[string]interface{}{"type": "boolean", "description": "Emit current documents before streaming changes (default false)."},
				"include_states":  map[string]interface{}{"type": "boolean", "description": "Include RethinkDB state notifications (default false)."},
				"squash":          map[string]interface{}{"description": "Coalesce changes: true (per-row) or a numeric interval in seconds."},
			},
		),
	}
}

func subscribeTableChangesFn(ctx ExecContext, raw json.RawMessage) (*CallToolResult, error) {
	var args subscribeTableArgs
	if err := json.Unmarshal(raw, &args); err != nil {
		return nil, InputError("invalid arguments: %v", err)
	}
	if args.Database == "" || args.Table == "" {
		return nil, InputError("database and table must not be empty")
	}

	opts := r.ChangesOpts{
		IncludeInitial: args.IncludeInitial,
		IncludeStates:  args.IncludeStates,
	}
	if args.Squash != nil {
		opts.Squash = args.Squash
	}

	query := r.DB(args.Database).Table(args.Table).Changes(opts)
	label := fmt.Sprintf("%s.%s", args.Database, args.Table)
	return startSubscription(ctx, query, label)
}

// ---------------------------------------------------------------------------
// subscribe_query_changes
// ---------------------------------------------------------------------------

type subscribeQueryArgs struct {
	Database       string                 `json:"database"`
	Table          string                 `json:"table"`
	Filter         map[string]interface{} `json:"filter,omitempty"`
	IncludeInitial bool                   `json:"include_initial,omitempty"`
	IncludeStates  bool                   `json:"include_states,omitempty"`
	Squash         interface{}            `json:"squash,omitempty"`
}

func subscribeQueryChangesDef() ToolDefinition {
	return ToolDefinition{
		Name: "subscribe_query_changes",
		Description: `Subscribe to real-time changes on a filtered subset of a RethinkDB table.
Only documents matching the filter will produce notifications.
Returns a subscriptionId; "notifications/stream" events arrive until
unsubscribe is called or the connection closes.`,
		InputSchema: inputSchema(
			[]string{"database", "table"},
			map[string]interface{}{
				"database":        prop("string", "Database name."),
				"table":           prop("string", "Table to watch."),
				"filter":          map[string]interface{}{"type": "object", "description": "Equality filter; only matching document changes are streamed."},
				"include_initial": map[string]interface{}{"type": "boolean", "description": "Emit matching documents before streaming changes."},
				"include_states":  map[string]interface{}{"type": "boolean", "description": "Include RethinkDB state notifications."},
				"squash":          map[string]interface{}{"description": "Coalesce changes: true or numeric seconds."},
			},
		),
	}
}

func subscribeQueryChangesFn(ctx ExecContext, raw json.RawMessage) (*CallToolResult, error) {
	var args subscribeQueryArgs
	if err := json.Unmarshal(raw, &args); err != nil {
		return nil, InputError("invalid arguments: %v", err)
	}
	if args.Database == "" || args.Table == "" {
		return nil, InputError("database and table must not be empty")
	}

	opts := r.ChangesOpts{
		IncludeInitial: args.IncludeInitial,
		IncludeStates:  args.IncludeStates,
	}
	if args.Squash != nil {
		opts.Squash = args.Squash
	}

	tbl := r.DB(args.Database).Table(args.Table)
	var query r.Term
	if len(args.Filter) > 0 {
		query = tbl.Filter(args.Filter).Changes(opts)
	} else {
		query = tbl.Changes(opts)
	}

	label := fmt.Sprintf("%s.%s", args.Database, args.Table)
	if len(args.Filter) > 0 {
		label += "(filtered)"
	}
	return startSubscription(ctx, query, label)
}

// ---------------------------------------------------------------------------
// unsubscribe
// ---------------------------------------------------------------------------

type unsubscribeArgs struct {
	SubscriptionID string `json:"subscriptionId"`
}

func unsubscribeDef() ToolDefinition {
	return ToolDefinition{
		Name:        "unsubscribe",
		Description: "Cancel an active changefeed subscription created by subscribe_table_changes or subscribe_query_changes.",
		InputSchema: inputSchema(
			[]string{"subscriptionId"},
			map[string]interface{}{
				"subscriptionId": prop("string", "The subscription ID returned by the subscribe call."),
			},
		),
	}
}

func unsubscribeFn(ctx ExecContext, raw json.RawMessage) (*CallToolResult, error) {
	var args unsubscribeArgs
	if err := json.Unmarshal(raw, &args); err != nil {
		return nil, InputError("invalid arguments: %v", err)
	}
	if args.SubscriptionID == "" {
		return nil, InputError("subscriptionId must not be empty")
	}

	key := sessionKey(ctx)
	v, ok := globalSubs.Load(key)
	if !ok {
		return nil, NotFoundError("no active subscriptions in this session")
	}
	mgr := v.(*subscriptionManager)

	mgr.mu.Lock()
	cancel, found := mgr.subs[args.SubscriptionID]
	if found {
		delete(mgr.subs, args.SubscriptionID)
	}
	mgr.mu.Unlock()

	if !found {
		return nil, NotFoundError("subscription %q not found", args.SubscriptionID)
	}

	cancel()
	return JSONResult(map[string]interface{}{
		"subscriptionId": args.SubscriptionID,
		"status":         "cancelled",
	})
}

// ---------------------------------------------------------------------------
// internal — subscription lifecycle
// ---------------------------------------------------------------------------

// startSubscription opens a changefeed cursor, registers a cancellable
// goroutine, and returns the subscription ID immediately.
func startSubscription(ctx ExecContext, query r.Term, label string) (*CallToolResult, error) {
	subID := newSubscriptionID()

	// Open cursor before returning so the caller gets an immediate error if
	// the query is invalid (table missing, permission denied, etc.).
	cursor, err := query.Run(ctx.Session.RDB())
	if err != nil {
		return nil, wrapDBErr(err, "open changefeed for %s", label)
	}

	subCtx, cancel := context.WithCancel(ctx.Context)

	key := sessionKey(ctx)
	mgr := getOrCreateManager(key)
	mgr.mu.Lock()
	mgr.subs[subID] = cancel
	mgr.mu.Unlock()

	go func() {
		defer func() {
			cursor.Close()
			cancel()
			mgr.mu.Lock()
			delete(mgr.subs, subID)
			mgr.mu.Unlock()
			ctx.Notify("notifications/stream", streamParams{
				SubscriptionID: subID,
				Type:           "end",
			})
		}()

		for {
			select {
			case <-subCtx.Done():
				return
			default:
			}

			var change interface{}
			if !cursor.Next(&change) {
				if curErr := cursor.Err(); curErr != nil {
					ctx.Notify("notifications/stream", streamParams{
						SubscriptionID: subID,
						Type:           "error",
						Data:           map[string]string{"message": curErr.Error()},
					})
				}
				return
			}

			notifType := "change"
			if changeMap, ok := change.(map[string]interface{}); ok {
				if _, hasState := changeMap["state"]; hasState {
					notifType = "state"
				}
			}

			ctx.Notify("notifications/stream", streamParams{
				SubscriptionID: subID,
				Type:           notifType,
				Data:           change,
			})
		}
	}()

	return JSONResult(map[string]interface{}{
		"subscriptionId": subID,
		"label":          label,
		"status":         "subscribed",
		"note":           "Changes arrive as notifications/stream MCP notifications. Call unsubscribe to stop.",
	})
}

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

func sessionKey(ctx ExecContext) string {
	return fmt.Sprintf("%p", ctx.Session)
}

// newSubscriptionID generates a random UUID v4 string.
func newSubscriptionID() string {
	b := make([]byte, 16)
	if _, err := rand.Read(b); err != nil {
		panic("crypto/rand unavailable: " + err.Error())
	}
	b[6] = (b[6] & 0x0f) | 0x40 // version 4
	b[8] = (b[8] & 0x3f) | 0x80 // variant bits
	return fmt.Sprintf("%08x-%04x-%04x-%04x-%012x",
		b[0:4], b[4:6], b[6:8], b[8:10], b[10:16])
}
