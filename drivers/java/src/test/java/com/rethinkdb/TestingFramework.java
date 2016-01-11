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
    private static final String HOST_NAME = "hostName";
    private static final String PORT      = "port";

    private static Configuration configuration;

    /**
     * Read the test configuration. Put a propertiy file called "test-config-override.properties" in the working
     * directory of the tests to override default values.
     * <P>
     * Example:
     * <pre>
     *     hostName=myHost
     *     port=12345
     * </pre>
     * </P>
     */
    public static Configuration getConfig() {
        if (configuration == null) {
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

            configuration = new Configuration();
            configuration.setHostName(config.getProperty("hostName").trim());
            configuration.setPort(Integer.parseInt(config.getProperty("port").trim()));
        }
        return configuration;
    }

    /**
     * @return A new connection from the configuration.
     */
    public static Connection createConnection() throws Exception {
        return RethinkDB.r.connection()
                .hostname( getConfig().getHostName() )
                .port( getConfig().getPort() )
                .connect();
    }

    /**
     * Test framework configuration.
     */
    public static class Configuration {

        private String hostName;
        private int port;

        public String getHostName() {
            return hostName;
        }

        public void setHostName( String hostName ) {
            this.hostName = hostName;
        }

        public int getPort() {
            return port;
        }

        public void setPort( int port ) {
            this.port = port;
        }
    }

}
