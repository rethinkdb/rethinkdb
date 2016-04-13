package com.rethinkdb;

import com.rethinkdb.gen.model.TopLevel;
import com.rethinkdb.net.Connection;
import com.rethinkdb.net.ConnectionPool;

public class RethinkDB extends TopLevel {

    /**
     * The Singleton to use to begin interacting with RethinkDB Driver
     */
    public static final RethinkDB r = new RethinkDB();

    public Connection.Builder connection() {
        return Connection.build();
    }

	  private static ConnectionPool globalConnectionPool;

	public static ConnectionPool getGlobalConnectionPool() {
		return globalConnectionPool;
	}

	public static void setGlobalConnectionPool(ConnectionPool globalConnectionPool) {
		RethinkDB.globalConnectionPool = globalConnectionPool;
	}
}
