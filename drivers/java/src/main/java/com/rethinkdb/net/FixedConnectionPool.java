package com.rethinkdb.net;

import com.rethinkdb.gen.exc.ReqlDriverError;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.ConcurrentLinkedDeque;

/**
 * A {@link ConnectionPool} that manages {@link Connection}s to a set of predefined cluster nodes.
 * This {@link ConnectionPool} implementation does not auto-discover cluster nodes,
 * nor does it dynamically handle adding or removing cluster nodes.
 */
public class FixedConnectionPool implements ConnectionPool {

	private static final Logger log = LoggerFactory.getLogger(FixedConnectionPool.class);

	private final Connection.Builder[] initialConnectionBuilders;

	private ConcurrentLinkedDeque<Connection> connDequeue = new ConcurrentLinkedDeque<>();

	// for sync purposes this field should only be changed in method getNewConnectionIndex()
	private int lastCreatedConnIndex = -1;

	/**
	 * Constructs a {@link ConnectionPool} with connections to the provided set of nodes.
	 * {@link Connection.Builder} must be provided for every cluster node that this ConnectionPool manages Connection for.
	 *
	 * @param connectionBuilders An array of one Connection.Builder for each cluster node.
	 */
	public FixedConnectionPool(Connection.Builder... connectionBuilders) {
		this.initialConnectionBuilders = connectionBuilders;
	}

	@Override
	public Connection leaseConnection() {

		// get from the front of the queue
		Connection leasedConn = connDequeue.pollFirst();

		// out of connections?
		if (leasedConn == null) {
			int i = 0;
			while (i < initialConnectionBuilders.length && leasedConn == null) {
				try {
					int pickedIndex = pickNextConnectionIndex();
					log.debug("Creating new connection. Picked conn builder " + pickedIndex + "/" + initialConnectionBuilders.length);
					Connection.Builder connBuilder = initialConnectionBuilders[pickedIndex];
					leasedConn = connBuilder.connect();
				} catch (ReqlDriverError rde) {
					log.warn("ReqlDriverError: " + rde.getMessage() + (rde.getBacktrace().isPresent() ? " backtrace:" + rde.getBacktrace().get().toString() : ""));
				}
				i++;
			}
			if (leasedConn == null) {
				throw new RuntimeException("Error: could not create a connection with any of the provided connection builders.");
			}
		}

		// check if connection is not open anymore and then try leasing another connection
		// this purges closed connections and consequently shrinks the pool if unused (closed) connections start piling up
		if (!leasedConn.isOpen()) {
			leasedConn = leaseConnection();
		}

		return leasedConn;
	}

	@Override
	public void returnConnection(Connection conn) {
		if (conn != null) {
			// return to the back of the queue
			connDequeue.addLast(conn);
		}
	}

	@Override
	public int size() {
		return connDequeue.size();
	}

	@Override
	public synchronized void close() {
		for (Connection connection : connDequeue) {
			connection.close();
		}
		connDequeue.clear();
	}

	/**
	 * Loops through initialConnectionBuilders indexes in a round-robin fashion.
	 * @return
	 */
	private synchronized int pickNextConnectionIndex() {
		// increment index
		lastCreatedConnIndex++;
		// reset if overflows
		if (lastCreatedConnIndex >= initialConnectionBuilders.length) {
			lastCreatedConnIndex = 0;
		}
		return lastCreatedConnIndex;
	}
}
