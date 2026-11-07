// Package tools - documents.go implements CRUD operations:
//   - query_reql   (structured query execution)
//   - insert_document
//   - update_document
//   - delete_document
package tools

import (
	"encoding/json"
	"fmt"

	r "gopkg.in/rethinkdb/rethinkdb-go.v6"
)

// RegisterDocumentTools adds CRUD tools to the registry.
func RegisterDocumentTools(reg *Registry) {
	reg.Register(queryReQLDef(), queryReQLFn)
	reg.Register(insertDocumentDef(), insertDocumentFn)
	reg.Register(updateDocumentDef(), updateDocumentFn)
	reg.Register(deleteDocumentDef(), deleteDocumentFn)
}

// ---------------------------------------------------------------------------
// query_reql — structured read query
// ---------------------------------------------------------------------------

// QueryArgs describes a structured query against a RethinkDB table.
//
// Query pipeline order: Table → Filter → OrderBy → Skip → Limit → Pluck/Without
type QueryArgs struct {
	Database string `json:"database"`
	Table    string `json:"table"`

	// Filter is an equality match object ANDing all conditions together.
	Filter map[string]interface{} `json:"filter,omitempty"`

	// GetByID fetches a single document by primary key.
	GetByID interface{} `json:"get_by_id,omitempty"`

	// OrderBy field name. Prefix with "-" for descending, e.g. "-created_at".
	OrderBy string `json:"order_by,omitempty"`

	// Limit caps the number of returned documents (default 100, max 10 000).
	Limit int `json:"limit,omitempty"`

	// Skip offsets the result set (pagination).
	Skip int `json:"skip,omitempty"`

	// Pluck lists fields to return; empty means return all fields.
	Pluck []string `json:"pluck,omitempty"`

	// Without lists fields to exclude from results.
	Without []string `json:"without,omitempty"`

	// Count returns the count of matching documents instead of the documents.
	Count bool `json:"count,omitempty"`
}

const (
	defaultLimit = 100
	maxLimit     = 10_000
)

func queryReQLDef() ToolDefinition {
	return ToolDefinition{
		Name: "query_reql",
		Description: `Execute a structured read query against a RethinkDB table.
Supports filtering, ordering, pagination, and field projection.
All operations run under the authenticated user's permissions.`,
		InputSchema: inputSchema(
			[]string{"database", "table"},
			map[string]interface{}{
				"database":  prop("string", "Database name."),
				"table":     prop("string", "Table name."),
				"filter":    map[string]interface{}{"type": "object", "description": "Equality filter map (all conditions ANDed)."},
				"get_by_id": map[string]interface{}{"description": "Return a single document by primary key value."},
				"order_by":  prop("string", fmt.Sprintf("Field to sort by. Prefix with '-' for descending order.")),
				"limit":     map[string]interface{}{"type": "integer", "description": fmt.Sprintf("Max results (default %d, max %d).", defaultLimit, maxLimit)},
				"skip":      map[string]interface{}{"type": "integer", "description": "Number of documents to skip (for pagination)."},
				"pluck":     map[string]interface{}{"type": "array", "items": map[string]interface{}{"type": "string"}, "description": "Fields to include (default: all)."},
				"without":   map[string]interface{}{"type": "array", "items": map[string]interface{}{"type": "string"}, "description": "Fields to exclude."},
				"count":     map[string]interface{}{"type": "boolean", "description": "Return document count instead of documents."},
			},
		),
	}
}

func queryReQLFn(ctx ExecContext, raw json.RawMessage) (*CallToolResult, error) {
	var args QueryArgs
	if err := json.Unmarshal(raw, &args); err != nil {
		return nil, InputError("invalid arguments: %v", err)
	}
	if args.Database == "" || args.Table == "" {
		return nil, InputError("database and table must not be empty")
	}

	// Single-document lookup by primary key.
	if args.GetByID != nil {
		cursor, err := r.DB(args.Database).Table(args.Table).Get(args.GetByID).Run(ctx.Session.RDB())
		if err != nil {
			return nil, wrapDBErr(err, "get by id in %q.%q", args.Database, args.Table)
		}
		defer cursor.Close()
		var doc interface{}
		if err := cursor.One(&doc); err != nil {
			if err.Error() == "document not found" {
				return nil, NotFoundError("document with id %v not found in %s.%s",
					args.GetByID, args.Database, args.Table)
			}
			return nil, DBError(err)
		}
		return JSONResult(doc)
	}

	query := buildReadQuery(args)
	cursor, err := query.Run(ctx.Session.RDB())
	if err != nil {
		return nil, wrapDBErr(err, "query %q.%q", args.Database, args.Table)
	}
	defer cursor.Close()

	if args.Count {
		var count int
		if err := cursor.One(&count); err != nil {
			return nil, DBError(err)
		}
		return JSONResult(map[string]interface{}{"count": count})
	}

	var docs []interface{}
	if err := cursor.All(&docs); err != nil {
		return nil, DBError(err)
	}
	if docs == nil {
		docs = []interface{}{}
	}

	return JSONResult(map[string]interface{}{
		"database": args.Database,
		"table":    args.Table,
		"count":    len(docs),
		"results":  docs,
	})
}

// buildReadQuery constructs a gorethink term chain from QueryArgs.
func buildReadQuery(args QueryArgs) r.Term {
	q := r.DB(args.Database).Table(args.Table)

	if len(args.Filter) > 0 {
		q = q.Filter(args.Filter)
	}

	if args.Count {
		return q.Count()
	}

	if args.OrderBy != "" {
		if len(args.OrderBy) > 1 && args.OrderBy[0] == '-' {
			q = q.OrderBy(r.Desc(args.OrderBy[1:]))
		} else {
			q = q.OrderBy(args.OrderBy)
		}
	}

	if args.Skip > 0 {
		q = q.Skip(args.Skip)
	}

	limit := args.Limit
	if limit <= 0 {
		limit = defaultLimit
	}
	if limit > maxLimit {
		limit = maxLimit
	}
	q = q.Limit(limit)

	if len(args.Pluck) > 0 {
		fields := make([]interface{}, len(args.Pluck))
		for i, f := range args.Pluck {
			fields[i] = f
		}
		q = q.Pluck(fields...)
	} else if len(args.Without) > 0 {
		fields := make([]interface{}, len(args.Without))
		for i, f := range args.Without {
			fields[i] = f
		}
		q = q.Without(fields...)
	}

	return q
}

// ---------------------------------------------------------------------------
// insert_document
// ---------------------------------------------------------------------------

type insertArgs struct {
	Database string      `json:"database"`
	Table    string      `json:"table"`
	Document interface{} `json:"document"`
	// Conflict controls behaviour when the primary key already exists.
	// Allowed values: "error" (default), "replace", "update".
	Conflict string `json:"conflict,omitempty"`
}

func insertDocumentDef() ToolDefinition {
	return ToolDefinition{
		Name:        "insert_document",
		Description: "Insert a single document into a RethinkDB table. Returns the generated primary key if one was not supplied.",
		InputSchema: inputSchema(
			[]string{"database", "table", "document"},
			map[string]interface{}{
				"database": prop("string", "Database name."),
				"table":    prop("string", "Table name."),
				"document": map[string]interface{}{"type": "object", "description": "Document to insert. Omit 'id' to have RethinkDB generate a UUID."},
				"conflict": propEnum("What to do when the primary key already exists.", []string{"error", "replace", "update"}),
			},
		),
	}
}

func insertDocumentFn(ctx ExecContext, raw json.RawMessage) (*CallToolResult, error) {
	var args insertArgs
	if err := json.Unmarshal(raw, &args); err != nil {
		return nil, InputError("invalid arguments: %v", err)
	}
	if args.Database == "" || args.Table == "" {
		return nil, InputError("database and table must not be empty")
	}
	if args.Document == nil {
		return nil, InputError("document must not be null")
	}

	conflict := args.Conflict
	if conflict == "" {
		conflict = "error"
	}

	cursor, err := r.DB(args.Database).Table(args.Table).
		Insert(args.Document, r.InsertOpts{Conflict: conflict, ReturnChanges: true}).
		Run(ctx.Session.RDB())
	if err != nil {
		return nil, wrapDBErr(err, "insert into %q.%q", args.Database, args.Table)
	}
	defer cursor.Close()

	var result interface{}
	if err := cursor.One(&result); err != nil {
		return nil, DBError(err)
	}

	return JSONResult(result)
}

// ---------------------------------------------------------------------------
// update_document
// ---------------------------------------------------------------------------

type updateArgs struct {
	Database  string      `json:"database"`
	Table     string      `json:"table"`
	ID        interface{} `json:"id"`
	Update    interface{} `json:"update"`
	NonAtomic bool        `json:"non_atomic,omitempty"`
}

func updateDocumentDef() ToolDefinition {
	return ToolDefinition{
		Name:        "update_document",
		Description: "Update fields of an existing document identified by its primary key. Only the specified fields are changed; others remain intact.",
		InputSchema: inputSchema(
			[]string{"database", "table", "id", "update"},
			map[string]interface{}{
				"database":   prop("string", "Database name."),
				"table":      prop("string", "Table name."),
				"id":         map[string]interface{}{"description": "Primary key value of the document to update."},
				"update":     map[string]interface{}{"type": "object", "description": "Fields to merge into the document."},
				"non_atomic": map[string]interface{}{"type": "boolean", "description": "Set true only when the update references other documents."},
			},
		),
	}
}

func updateDocumentFn(ctx ExecContext, raw json.RawMessage) (*CallToolResult, error) {
	var args updateArgs
	if err := json.Unmarshal(raw, &args); err != nil {
		return nil, InputError("invalid arguments: %v", err)
	}
	if args.Database == "" || args.Table == "" {
		return nil, InputError("database and table must not be empty")
	}
	if args.ID == nil {
		return nil, InputError("id must not be null")
	}
	if args.Update == nil {
		return nil, InputError("update must not be null")
	}

	opts := r.UpdateOpts{ReturnChanges: true}
	if args.NonAtomic {
		opts.NonAtomic = true
	}

	cursor, err := r.DB(args.Database).Table(args.Table).
		Get(args.ID).Update(args.Update, opts).Run(ctx.Session.RDB())
	if err != nil {
		return nil, wrapDBErr(err, "update %q.%q id=%v", args.Database, args.Table, args.ID)
	}
	defer cursor.Close()

	var result interface{}
	if err := cursor.One(&result); err != nil {
		return nil, DBError(err)
	}

	if resMap, ok := result.(map[string]interface{}); ok {
		if skipped, _ := resMap["skipped"].(float64); skipped > 0 {
			return nil, NotFoundError("document with id %v not found in %s.%s",
				args.ID, args.Database, args.Table)
		}
	}

	return JSONResult(result)
}

// ---------------------------------------------------------------------------
// delete_document
// ---------------------------------------------------------------------------

type deleteArgs struct {
	Database string      `json:"database"`
	Table    string      `json:"table"`
	ID       interface{} `json:"id"`
}

func deleteDocumentDef() ToolDefinition {
	return ToolDefinition{
		Name:        "delete_document",
		Description: "Delete a single document by its primary key. Returns a summary including the number of deleted documents.",
		InputSchema: inputSchema(
			[]string{"database", "table", "id"},
			map[string]interface{}{
				"database": prop("string", "Database name."),
				"table":    prop("string", "Table name."),
				"id":       map[string]interface{}{"description": "Primary key value of the document to delete."},
			},
		),
	}
}

func deleteDocumentFn(ctx ExecContext, raw json.RawMessage) (*CallToolResult, error) {
	var args deleteArgs
	if err := json.Unmarshal(raw, &args); err != nil {
		return nil, InputError("invalid arguments: %v", err)
	}
	if args.Database == "" || args.Table == "" {
		return nil, InputError("database and table must not be empty")
	}
	if args.ID == nil {
		return nil, InputError("id must not be null")
	}

	cursor, err := r.DB(args.Database).Table(args.Table).
		Get(args.ID).Delete(r.DeleteOpts{ReturnChanges: true}).Run(ctx.Session.RDB())
	if err != nil {
		return nil, wrapDBErr(err, "delete %q.%q id=%v", args.Database, args.Table, args.ID)
	}
	defer cursor.Close()

	var result interface{}
	if err := cursor.One(&result); err != nil {
		return nil, DBError(err)
	}

	if resMap, ok := result.(map[string]interface{}); ok {
		if skipped, _ := resMap["skipped"].(float64); skipped > 0 {
			return nil, NotFoundError("document with id %v not found in %s.%s",
				args.ID, args.Database, args.Table)
		}
	}

	return JSONResult(result)
}
