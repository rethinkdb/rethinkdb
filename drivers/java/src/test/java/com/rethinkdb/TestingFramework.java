package com.rethinkdb;

import com.rethinkdb.net.Connection;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

/**
 * Very basic testing framework lying miserably in the java's default package.
 */
public class TestingFramework {

    private static final String DEFAULT_CONFIG_RESOURCE = "default-config.properties";
    private static final String OVERRIDE_FILE_NAME = "test-config-override.properties";

    // properties used to populate configuration
    private static final String PROP_HOSTNAME = "hostName";
    private static final String PROP_PORT = "port";
    private static final String PROP_AUTHKEY = "authKey";

    private static Connection.Builder defaultConnectionBuilder;

    /**
     * Provision a connection builder based on the test configuration.
     * <p>
     * Put a propertiy file called "test-config-override.properties" in the working
     * directory of the tests to override default values.
     * <p>
     * Example:
     * <pre>
     *     hostName=myHost
     *     port=12345
     * </pre>
     * </P>
     *
     * @return Default connection builder.
     */
    public static Connection.Builder defaultConnectionBuilder() {
        if (defaultConnectionBuilder == null) {
            Properties config = new Properties();

            try (InputStream is = TestingFramework.class.getClassLoader().getResourceAsStream(DEFAULT_CONFIG_RESOURCE)) {
                config.load(is);
            } catch (NullPointerException | IOException e) {
                throw new IllegalStateException(e);
            }

            // Check the local override file.
            String workdir = System.getProperty("user.dir");
            File defaultFile = new File(workdir, OVERRIDE_FILE_NAME);
            if (defaultFile.exists()) {
                try (InputStream is = new FileInputStream(defaultFile)) {
                    config.load(is);
                } catch (IOException e) {
                    throw new IllegalStateException(e);
                }
            }

            // provision connection builder based on configuration
            defaultConnectionBuilder = RethinkDB.r.connection();
            // mandatory fields
            defaultConnectionBuilder = defaultConnectionBuilder.hostname(config.getProperty(PROP_HOSTNAME).trim());
            defaultConnectionBuilder = defaultConnectionBuilder.port(Integer.parseInt(config.getProperty(PROP_PORT).trim()));
            // optinal fields
            final String authKey = config.getProperty(PROP_AUTHKEY);
            if (authKey != null) {
                defaultConnectionBuilder.authKey(config.getProperty(PROP_AUTHKEY).trim());
            }
        }

        return defaultConnectionBuilder;
    }

    /**
     * @return A new connection from the configuration.
     */
    public static Connection createConnection() throws Exception {
        return createConnection(defaultConnectionBuilder());
    }

    /**
     * @return A new connection from a specific builder to be used in tests where a specific connection is needed,
     * i.e. connection secured with SSL.
     */
    public static Connection createConnection(Connection.Builder builder) throws Exception {
        return builder.connect();
    }

}
