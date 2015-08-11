package com.rethinkdb;

import com.rethinkdb.ast.gen.TopLevel;
import com.rethinkdb.net.Connection;
import com.rethinkdb.net.ConnectionInstance;

public class RethinkDB extends TopLevel {

    /**
     * The Singleton to use to begin interacting with RethinkDB Driver
     */
    public static final RethinkDB r = new RethinkDB();

    private RethinkDB() {
        super(null, null, null, null);
    }

    public Connection.Builder<ConnectionInstance> connection() {
        return Connection.build();
    }
}
