package com.rethinkdb.ast;

import com.rethinkdb.RethinkDB;
import com.rethinkdb.gen.exc.ReqlDriverError;
import com.rethinkdb.gen.proto.TermType;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.net.Connection;
import com.rethinkdb.net.ConnectionPool;
import org.json.simple.JSONArray;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

/** Base class for all reql queries.
 */
public class ReqlAst {

    protected final TermType termType;
    protected final Arguments args;
    protected final OptArgs optargs;

    protected ReqlAst(TermType termType, Arguments args, OptArgs optargs) {
        if(termType == null){
            throw new ReqlDriverError("termType can't be null!");
        }
        this.termType = termType;
        this.args = args != null ? args : new Arguments();
        this.optargs = optargs != null ? optargs : new OptArgs();
    }

    protected Object build() {
        // Create a JSON object from the Ast
        JSONArray list = new JSONArray();
        list.add(termType.value);
        if (args.size() > 0) {
            list.add(args.stream()
                    .map(ReqlAst::build)
                    .collect(Collectors.toCollection(JSONArray::new)));
        }else {
            list.add(new JSONArray());
        }
        if (optargs.size() > 0) {
            list.add(buildOptarg(optargs));
        }
        return list;
    }

    public static Map<String, Object> buildOptarg(OptArgs opts){
        Map<String, Object> result = new HashMap<>( opts.size() );
        opts.forEach( (name, arg) -> result.put( name, arg.build() ) );
        return result;
    }

    /**
     * Runs this query via connection {@code conn} with default options and returns an atom result
     * or a sequence result as a cursor. The atom result either has a primitive type (e.g., {@code Integer})
     * or represents a JSON object as a {@code Map<String, Object>}. The cursor is a {@code com.rethinkdb.net.Cursor}
     * which may be iterated to get a sequence of atom results
     * @param conn The connection to run this query
     * @param <T> The type of result
     * @return The result of this query
     */
    public <T> T run(Connection conn) {
        return conn.run(this, new OptArgs(), Optional.empty());
    }

	  /**
	   * Runs this query via connection {@code conn} provided by connection pool, with default options and returns an atom result
	   * or a sequence result as a cursor. The atom result either has a primitive type (e.g., {@code Integer})
	   * or represents a JSON object as a {@code Map<String, Object>}. The cursor is a {@code com.rethinkdb.net.Cursor}
	   * which may be iterated to get a sequence of atom results
	   * @param pool The connection pool to provide the connection
	   * @param <T> The type of result
	   * @return The result of this query
	   */
	  public <T> T run(ConnectionPool pool) {
		    Connection conn = pool.leaseConnection();
		    T res = this.run(conn);
		    pool.returnConnection(conn);
		    return res;
	  }

	  /**
	   * Runs this query via connection {@code conn} provided by the internal global connection pool,
	   * with default options and returns an atom result
	   * or a sequence result as a cursor. The atom result either has a primitive type (e.g., {@code Integer})
	   * or represents a JSON object as a {@code Map<String, Object>}. The cursor is a {@code com.rethinkdb.net.Cursor}
	   * which may be iterated to get a sequence of atom results.
	   * <p/>Global connection pool must be defined prior to using this metod.
	   * @param <T> The type of result
	   * @return The result of this query
	   */
	  public <T> T run() {
	  	  ConnectionPool globalPool = RethinkDB.getGlobalConnectionPool();
		    if (globalPool == null) {
			    throw new RuntimeException("Global connection pool must be defined via RethinkDB.setGlobalConnectionPool() " +
			  		  "prior to calling RethinkDB.run()");
  		  }
		    return run(globalPool);
	  }

    /**
     * Runs this query via connection {@code conn} with options {@code runOpts} and returns an atom result
     * or a sequence result as a cursor. The atom result either has a primitive type (e.g., {@code Integer})
     * or represents a JSON object as a {@code Map<String, Object>}. The cursor is a {@code com.rethinkdb.net.Cursor}
     * which may be iterated to get a sequence of atom results
     * @param conn The connection to run this query
     * @param runOpts The options to run this query with
     * @param <T> The type of result
     * @return The result of this query
     */
    public <T> T run(Connection conn, OptArgs runOpts) {
        return conn.run(this, runOpts, Optional.empty());
    }

	  /**
	   * Runs this query via connection {@code conn} provided by connection pool,
	   * with options {@code runOpts} and returns an atom result
	   * or a sequence result as a cursor. The atom result either has a primitive type (e.g., {@code Integer})
	   * or represents a JSON object as a {@code Map<String, Object>}. The cursor is a {@code com.rethinkdb.net.Cursor}
	   * which may be iterated to get a sequence of atom results
	   * @param pool The connection pool to provide connection to run this query
	   * @param runOpts The options to run this query with
	   * @param <T> The type of result
	   * @return The result of this query
	   */
	  public <T> T run(ConnectionPool pool, OptArgs runOpts) {
		    Connection conn = pool.leaseConnection();
		    T res = this.run(conn, runOpts);
		    pool.returnConnection(conn);
		    return res;
	  }

    /**
     * Runs this query via connection {@code conn} provided by global connection pool,
     * with options {@code runOpts} and returns an atom result
     * or a sequence result as a cursor. The atom result either has a primitive type (e.g., {@code Integer})
     * or represents a JSON object as a {@code Map<String, Object>}. The cursor is a {@code com.rethinkdb.net.Cursor}
     * which may be iterated to get a sequence of atom results
     * @param runOpts The options to run this query with
     * @param <T> The type of result
     * @return The result of this query
     */
    public <T> T run(OptArgs runOpts) {
    	  ConnectionPool globalPool = RethinkDB.getGlobalConnectionPool();
    	  if (globalPool == null) {
    	  	throw new RuntimeException("Global connection pool must be defined via RethinkDB.setGlobalConnectionPool() " +
    	  			"prior to calling RethinkDB.run()");
    	  }
    	  return run(globalPool, runOpts);
    }

  	/**
     * Runs this query via connection {@code conn} with default options and returns an atom result
     * or a sequence result as a cursor. The atom result representing a JSON object is converted
     * to an object of type {@code Class<P>} specified with {@code pojoClass}. The cursor
     * is a {@code com.rethinkdb.net.Cursor} which may be iterated to get a sequence of atom results
     * of type {@code Class<P>}
     * @param conn The connection to run this query
     * @param pojoClass The class of POJO to convert to
     * @param <T> The type of result
     * @param <P> The type of POJO to convert to
     * @return The result of this query (either a {@code P or a Cursor<P>}
     */
    public <T, P> T run(Connection conn, Class<P> pojoClass) {
        return conn.run(this, new OptArgs(), Optional.of(pojoClass));
    }

    /**
     * Runs this query via connection {@code conn} provided by connection pool,
     * with options {@code runOpts} and returns an atom result
     * or a sequence result as a cursor. The atom result representing a JSON object is converted
     * to an object of type {@code Class<P>} specified with {@code pojoClass}. The cursor
     * is a {@code com.rethinkdb.net.Cursor} which may be iterated to get a sequence of atom results
     * of type {@code Class<P>}
     * @param pool The connection pool to provide connection to run this query
     * @param pojoClass The class of POJO to convert to
     * @param <T> The type of result
     * @param <P> The type of POJO to convert to
     * @return The result of this query (either a {@code P or a Cursor<P>}
     */
    public <T, P> T run(ConnectionPool pool, Class<P> pojoClass) {
    	  Connection conn = pool.leaseConnection();
    	  T res = this.run(conn, pojoClass);
    	  pool.returnConnection(conn);
    	  return res;
    }

    /**
     * Runs this query via connection {@code conn} provided by global connection pool,
     * with options {@code runOpts} and returns an atom result
     * or a sequence result as a cursor. The atom result representing a JSON object is converted
     * to an object of type {@code Class<P>} specified with {@code pojoClass}. The cursor
     * is a {@code com.rethinkdb.net.Cursor} which may be iterated to get a sequence of atom results
     * of type {@code Class<P>}
     * @param pojoClass The class of POJO to convert to
     * @param <T> The type of result
     * @param <P> The type of POJO to convert to
     * @return The result of this query (either a {@code P or a Cursor<P>}
     */
    public <T, P> T run(Class<P> pojoClass) {
    	  ConnectionPool globalPool = RethinkDB.getGlobalConnectionPool();
    	  if (globalPool == null) {
    		  throw new RuntimeException("Global connection pool must be defined via RethinkDB.setGlobalConnectionPool() " +
    			  	"prior to calling RethinkDB.run()");
    	  }
    	  return run(globalPool, pojoClass);
    }

    /**
     * Runs this query via connection {@code conn} with options {@code runOpts} and returns an atom result
     * or a sequence result as a cursor. The atom result representing a JSON object is converted
     * to an object of type {@code Class<P>} specified with {@code pojoClass}. The cursor
     * is a {@code com.rethinkdb.net.Cursor} which may be iterated to get a sequence of atom results
     * of type {@code Class<P>}
     * @param conn The connection to run this query
     * @param runOpts The options to run this query with
     * @param pojoClass The class of POJO to convert to
     * @param <T> The type of result
     * @param <P> The type of POJO to convert to
     * @return The result of this query (either a {@code P or a Cursor<P>}
     */
    public <T, P> T run(Connection conn, OptArgs runOpts, Class<P> pojoClass) {
        return conn.run(this, runOpts, Optional.of(pojoClass));
    }

    /**
     * Runs this query via connection {@code conn} provided by connection pool,
     * with options {@code runOpts} and returns an atom result
     * or a sequence result as a cursor. The atom result representing a JSON object is converted
     * to an object of type {@code Class<P>} specified with {@code pojoClass}. The cursor
     * is a {@code com.rethinkdb.net.Cursor} which may be iterated to get a sequence of atom results
     * of type {@code Class<P>}
     * @param pool The connection pool to provide connection to run this query
     * @param runOpts The options to run this query with
     * @param pojoClass The class of POJO to convert to
     * @param <T> The type of result
     * @param <P> The type of POJO to convert to
     * @return The result of this query (either a {@code P or a Cursor<P>}
     */
    public <T, P> T run(ConnectionPool pool, OptArgs runOpts, Class<P> pojoClass) {
	      Connection conn = pool.leaseConnection();
	      T res = this.run(conn, runOpts, pojoClass);
	      pool.returnConnection(conn);
	      return res;
    }

  	/**
	   * Runs this query via connection {@code conn} provided by global connection pool,
	   * with options {@code runOpts} and returns an atom result
	   * or a sequence result as a cursor. The atom result representing a JSON object is converted
	   * to an object of type {@code Class<P>} specified with {@code pojoClass}. The cursor
	   * is a {@code com.rethinkdb.net.Cursor} which may be iterated to get a sequence of atom results
	   * of type {@code Class<P>}
	   * @param runOpts The options to run this query with
	   * @param pojoClass The class of POJO to convert to
	   * @param <T> The type of result
	   * @param <P> The type of POJO to convert to
	   * @return The result of this query (either a {@code P or a Cursor<P>}
	   */
	  public <T, P> T run(OptArgs runOpts, Class<P> pojoClass) {
	  	  ConnectionPool globalPool = RethinkDB.getGlobalConnectionPool();
		    if (globalPool == null) {
			      throw new RuntimeException("Global connection pool must be defined via RethinkDB.setGlobalConnectionPool() " +
		  		  	"prior to calling RethinkDB.run()");
	  	  }
		    return run(globalPool, runOpts, pojoClass);
	  }

    public void runNoReply(Connection conn){
        conn.runNoReply(this, new OptArgs());
    }

    public void runNoReply(Connection conn, OptArgs globalOpts){
        conn.runNoReply(this, globalOpts);
    }

    @Override
    public String toString() {
        return "ReqlAst{" +
                "termType=" + termType +
                ", args=" + args +
                ", optargs=" + optargs +
                '}';
    }
}
