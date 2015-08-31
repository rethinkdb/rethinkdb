package com.rethinkdb;

import com.rethinkdb.gen.model.TopLevel;
import com.rethinkdb.net.Connection;
import com.rethinkdb.net.ConnectionInstance;

public class RethinkDB extends TopLevel {

    /**
     * The Singleton to use to begin interacting with RethinkDB Driver
     */
    public static final RethinkDB r = new RethinkDB();

    public Connection.Builder<ConnectionInstance> connection() {
        return Connection.build();
    }
}
