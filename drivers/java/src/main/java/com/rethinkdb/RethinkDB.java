package com.rethinkdb;

import com.rethinkdb.ast.gen.TopLevel;

/**
 * The starting point for all interaction with RethinkDB. This singleton corresponds to r
 * in the documentation and is used to open a connection or generate a query. i.e:
 * <pre>
 *     {@code RethinkDB.r.connect("hostname", 28015); }
 * </pre>
 * Or
 * <pre>
 *     {@code RethinkDB.r.dbCreate("test") }
 * </pre>
 */
public class RethinkDB extends TopLevel {

    /**
     * The Singleton to use to begin interacting with RethinkDB Driver
     */
    public static final RethinkDB r = new RethinkDB();

    private RethinkDB() {
        super(null, null, null, null);
    }

    /**
     * Connect with default hostname and default port and default timeout
     *
     * @return connection
     */
    public RethinkDBConnection connect() {
        return new RethinkDBConnection();
    }

    /**
     * connect with given hostname and default port and default timeout
     *
     * @param hostname hostname
     * @return connection
     */
    public RethinkDBConnection connect(String hostname) {
        return new RethinkDBConnection(hostname);
    }

    /**
     * connect with given hostname and port and default timeout
     *
     * @param hostname hostname
     * @param port     port
     * @return connection
     */
    public RethinkDBConnection connect(String hostname, int port) {
        return new RethinkDBConnection(hostname, port);
    }

    /**
     * connect with given hostname, port and authentication key and default timeout
     *
     * @param hostname hostname
     * @param port     port
     * @param authKey  authentication key
     * @return connection
     */
    public RethinkDBConnection connect(String hostname, int port, String authKey) {
        return new RethinkDBConnection(hostname, port, authKey);
    }

    /**
     * connect with given hostname, port, authentication key and timeout
     *
     * @param hostname hostname
     * @param port     port
     * @param authKey  authentication key
     * @param timeout  the maximum time to wait attempting to connect
     * @return connection
     */
    public RethinkDBConnection connect(String hostname, int port, String authKey, int timeout) {
        return new RethinkDBConnection(hostname, port, authKey, timeout);
    }
}
