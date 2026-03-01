// Package tools - database.go implements introspection tools:
//   - list_databases
//   - list_tables
//   - describe_table
package tools

import (
	"encoding/json"
	"fmt"

	r "gopkg.in/rethinkdb/rethinkdb-go.v6"
)

// RegisterDatabaseTools adds database introspection tools to the registry.
func RegisterDatabaseTools(reg *Registry) {
	reg.Register(listDatabasesDef(), listDatabasesFn)
	reg.Register(listTablesDef(), listTablesFn)
	reg.Register(describeTableDef(), describeTableFn)
}

// ---------------------------------------------------------------------------
// list_databases
// ---------------------------------------------------------------------------

func listDatabasesDef() ToolDefinition {
	return ToolDefinition{
		Name:        "list_databases",
		Description: "List all databases available in the RethinkDB instance. Returns only databases the authenticated user has access to.",
		InputSchema: inputSchema(nil, map[string]interface{}{}),
	}
}

func listDatabasesFn(ctx ExecContext, _ json.RawMessage) (*CallToolResult, error) {
	cursor, err := r.DBList().Run(ctx.Session.RDB())
	if err != nil {
		return nil, DBError(err)
	}
	defer cursor.Close()

	var dbs []string
	if err := cursor.All(&dbs); err != nil {
		return nil, DBError(err)
	}

	return JSONResult(dbs)
}

// ---------------------------------------------------------------------------
// list_tables
// ---------------------------------------------------------------------------

type listTablesArgs struct {
	Database string `json:"database"`
}

func listTablesDef() ToolDefinition {
	return ToolDefinition{
		Name:        "list_tables",
		Description: "List all tables within a specific database. Returns only tables the authenticated user has read access to.",
		InputSchema: inputSchema(
			[]string{"database"},
			map[string]interface{}{
				"database": prop("string", "Name of the database to list tables from."),
			},
		),
	}
}

func listTablesFn(ctx ExecContext, raw json.RawMessage) (*CallToolResult, error) {
	var args listTablesArgs
	if err := json.Unmarshal(raw, &args); err != nil {
		return nil, InputError("invalid arguments: %v", err)
	}
	if args.Database == "" {
		return nil, InputError("database must not be empty")
	}

	cursor, err := r.DB(args.Database).TableList().Run(ctx.Session.RDB())
	if err != nil {
		return nil, wrapDBErr(err, "list tables in %q", args.Database)
	}
	defer cursor.Close()

	var tables []string
	if err := cursor.All(&tables); err != nil {
		return nil, DBError(err)
	}

	return JSONResult(map[string]interface{}{
		"database": args.Database,
		"tables":   tables,
	})
}

// ---------------------------------------------------------------------------
// describe_table
// ---------------------------------------------------------------------------

type describeTableArgs struct {
	Database string `json:"database"`
	Table    string `json:"table"`
}

// TableInfo aggregates metadata about a single RethinkDB table.
type TableInfo struct {
	Database   string        `json:"database"`
	Table      string        `json:"table"`
	PrimaryKey string        `json:"primaryKey"`
	Indexes    []interface{} `json:"indexes"`
	Config     interface{}   `json:"config"`
	Status     interface{}   `json:"status"`
}

func describeTableDef() ToolDefinition {
	return ToolDefinition{
		Name:        "describe_table",
		Description: "Return metadata for a table including primary key, secondary indexes, replication settings, and current status.",
		InputSchema: inputSchema(
			[]string{"database", "table"},
			map[string]interface{}{
				"database": prop("string", "Database that owns the table."),
				"table":    prop("string", "Table to describe."),
			},
		),
	}
}

func describeTableFn(ctx ExecContext, raw json.RawMessage) (*CallToolResult, error) {
	var args describeTableArgs
	if err := json.Unmarshal(raw, &args); err != nil {
		return nil, InputError("invalid arguments: %v", err)
	}
	if args.Database == "" || args.Table == "" {
		return nil, InputError("database and table must not be empty")
	}

	session := ctx.Session.RDB()

	// Fetch index list.
	idxCursor, err := r.DB(args.Database).Table(args.Table).IndexList().Run(session)
	if err != nil {
		return nil, wrapDBErr(err, "index list for %q.%q", args.Database, args.Table)
	}
	defer idxCursor.Close()
	var indexes []interface{}
	if err := idxCursor.All(&indexes); err != nil {
		return nil, DBError(err)
	}

	// Fetch table config (primary key, shards, replicas).
	cfgCursor, err := r.DB(args.Database).Table(args.Table).Config().Run(session)
	if err != nil {
		return nil, wrapDBErr(err, "config for %q.%q", args.Database, args.Table)
	}
	defer cfgCursor.Close()
	var cfg interface{}
	if err := cfgCursor.One(&cfg); err != nil {
		return nil, DBError(err)
	}

	// Fetch table status (ready_for_reads, ready_for_writes, etc.).
	stCursor, err := r.DB(args.Database).Table(args.Table).Status().Run(session)
	if err != nil {
		return nil, wrapDBErr(err, "status for %q.%q", args.Database, args.Table)
	}
	defer stCursor.Close()
	var status interface{}
	if err := stCursor.One(&status); err != nil {
		return nil, DBError(err)
	}

	// Extract primary key from config map.
	primaryKey := "id"
	if cfgMap, ok := cfg.(map[string]interface{}); ok {
		if pk, ok := cfgMap["primary_key"].(string); ok {
			primaryKey = pk
		}
	}

	info := TableInfo{
		Database:   args.Database,
		Table:      args.Table,
		PrimaryKey: primaryKey,
		Indexes:    indexes,
		Config:     cfg,
		Status:     status,
	}

	return JSONResult(info)
}

// ---------------------------------------------------------------------------
// shared helpers
// ---------------------------------------------------------------------------

func wrapDBErr(err error, format string, args ...interface{}) *ToolError {
	return toolErr(ErrKindDB, fmt.Sprintf(format, args...)+": "+err.Error())
}
