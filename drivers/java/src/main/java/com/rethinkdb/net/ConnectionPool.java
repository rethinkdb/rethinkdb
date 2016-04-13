package com.rethinkdb.net;

import com.rethinkdb.RethinkDB;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public interface ConnectionPool {

	/**
	 * Provides a Connausted.ection by taking it from a connection pool or creates a new Connection of pool is exh
	 * @return
	 */
	Connection leaseConnection();

	/**
	 * To be called by application code to return Connection to the connection pool.
	 * At this point the connection pool is free to provide this Connection to any caller of leaseConnection().
	 * App code must not use the Connection after it called this method.
	 * @param conn
	 */
	void returnConnection(Connection conn);

	/**
	 * Pool size.
	 * @return
	 */
	int size();

	/**
	 * Close all connections and clear the pool
	 */
	void close();

	public default List<Connection.Builder> queryCluster(Connection.Builder initialConnectionBuilder) {

		List<Connection.Builder> discoveredNodes = new ArrayList<>();
		Connection conn = initialConnectionBuilder.connect();
		Cursor serverConfig = RethinkDB.r.db("rethinkdb").table("server_status").run(conn);

		for (Object obj : serverConfig.toList()) {
			Map map = (Map) obj;

			System.out.println("_______________\nname:" + map.get("name") + " id:" + map.get("id"));
			Map network = (Map) map.get("network");
			String hostname = (String) network.get("hostname");
			long reqlPort = (long) network.get("reql_port");
			List addresses = (List) network.get("canonical_addresses");
			List<InetAddress> foundAddresses = new ArrayList<>(10);
			for (Object addressObj : addresses) {
				Map address = (Map) addressObj;
				String host = (String) address.get("host");
				long port = (long) address.get("port");
				try {
					InetAddress inetAddress = InetAddress.getByName(host);
					foundAddresses.add(inetAddress);
					String type = inetAddress instanceof Inet4Address ? "IPv4" : (inetAddress instanceof Inet6Address ? "IPv6" : "unknown");
					System.out.println(host + ":" + port + " type:" + type
							+ " loopback:" + inetAddress.isLoopbackAddress()
							+ " anyLocal:" + inetAddress.isAnyLocalAddress()
							+ " siteLocal:" + inetAddress.isSiteLocalAddress()
							+ " linkLocal:" + inetAddress.isLinkLocalAddress());
					//+ " isReachable:" + inetAddress.isReachable(2000));

				} catch (UnknownHostException e) {
					// quietly drop this IP address
				}
			}

			// find most appropriate address in this order:
			// 1. public IPv4
			// 2. public IPv6
			List<InetAddress> publicIPv4 = filterInetAddresses(foundAddresses, true, true);
			List<InetAddress> publicIPv6 = filterInetAddresses(foundAddresses, false, true);
			if (!publicIPv4.isEmpty()) {

				for (InetAddress inetAddress : publicIPv4) {
					Connection.Builder copy = cloneConnection(initialConnectionBuilder, inetAddress.getHostAddress(), (int) reqlPort);
					if (copy != null) {
						discoveredNodes.add(copy);
					}
				}
			} else if (!publicIPv6.isEmpty()) {

				for (InetAddress inetAddress : publicIPv4) {
					Connection.Builder copy = cloneConnection(initialConnectionBuilder, inetAddress.getHostAddress(), (int) reqlPort);
					if (copy != null) {
						discoveredNodes.add(copy);
					}
				}
			}
		}

		return discoveredNodes;
	}

	public default Connection.Builder cloneConnection(Connection.Builder original, String hostAddress, int port) {
		Connection.Builder copy = null;
		try {
			copy = original.clone();
		} catch (CloneNotSupportedException e) {
			e.printStackTrace();  // should not happen - Connection.Builder implements Cloneable
			return null;
		}
		copy.hostname(hostAddress);
		copy.port(port);
		return copy;
	}


	/**
	 * Filters a list InetAddress addresses according to given criteria
	 *
	 * @param addresses       An input list of InetAddress addresses.
	 * @param ipType          true if IPv4, false if IPv6
	 * @param publicOrPrivate true if only public addresses, false if only private or loopback
	 * @return A list of public IPv4 addresses matching the criteria.
	 */
	default List<InetAddress> filterInetAddresses(List<InetAddress> addresses, boolean ipType, boolean publicOrPrivate) {
		List<InetAddress> found = new ArrayList<>();
		for (InetAddress address : addresses) {
			boolean isPublic = !address.isLoopbackAddress() && !address.isLinkLocalAddress() && !address.isSiteLocalAddress();
			boolean properInetClass = ipType ? address instanceof Inet4Address : address instanceof Inet6Address;
			if (properInetClass && (publicOrPrivate && isPublic || !publicOrPrivate && !isPublic)) {
				found.add(address);
			}
		}
		return found;
	}

	default String printList(List<InetAddress> list) {
		StringBuilder out = new StringBuilder();
		for (InetAddress ia : list) {
			out.append(ia.getHostAddress()).append(" ,");
		}
		return out.toString();
	}

}
