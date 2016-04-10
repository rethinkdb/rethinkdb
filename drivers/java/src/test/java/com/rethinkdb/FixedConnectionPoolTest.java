package com.rethinkdb;

import com.rethinkdb.net.Connection;
import com.rethinkdb.net.FixedConnectionPool;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

public class FixedConnectionPoolTest {

	private FixedConnectionPool pool;

	@Before
	public void before() {

		Connection.Builder connBuilder1 = Connection.build().hostname("localhost").port(28015);
		Connection.Builder connBuilder2 = Connection.build().hostname("localhost").port(28016);
		Connection.Builder connBuilder3 = Connection.build().hostname("localhost").port(28017);
		Connection.Builder connBuilder4 = Connection.build().hostname("localhost").port(28018); // this one should not exist

		pool = new FixedConnectionPool(connBuilder1, connBuilder2, connBuilder3, connBuilder4);
		RethinkDB.r.db("test").tableCreate("pooltest").run(pool);
	}

	@After
	public void after() {
		RethinkDB.r.db("test").tableDrop("pooltest").run(pool);
		pool.close();
	}

	@Test(expected = RuntimeException.class)
	public void noGlobalPoolTest() {

		RethinkDB.r.db("test").table("pooltest").run();  // throws RuntimeException
	}

	@Test
	public void globalPoolTest() {

		RethinkDB.setGlobalConnectionPool(pool);

		RethinkDB.r.db("test").table("pooltest").run();  // uses global conneciton pool
	}

	@Test
	public void oneLeaseTest() {

		// connection is leased and immediately returned, so only one connection is created
		Connection conn1 = pool.leaseConnection();
		pool.returnConnection(conn1);
		Connection conn2 = pool.leaseConnection();
		pool.returnConnection(conn2);
		Connection conn3 = pool.leaseConnection();
		pool.returnConnection(conn3);

		// it's all the same connection
		Assert.assertEquals(conn1, conn2);
		Assert.assertEquals(conn2, conn3);

		// one connection was created
		Assert.assertEquals(1, pool.size());
	}

	@Test
	public void multiLeaseTest() {

		// three connections are leased and then  returned, so three connections are created
		Connection conn1 = pool.leaseConnection();
		Connection conn2 = pool.leaseConnection();
		Connection conn3 = pool.leaseConnection();
		Connection conn4 = pool.leaseConnection();
		pool.returnConnection(conn1);
		pool.returnConnection(conn2);
		pool.returnConnection(conn3);
		pool.returnConnection(conn4);

		Assert.assertNotEquals(conn1, conn2);
		Assert.assertNotEquals(conn2, conn3);
		Assert.assertNotEquals(conn3, conn4);

		// four connections were created
		Assert.assertEquals(4, pool.size());
	}
}
