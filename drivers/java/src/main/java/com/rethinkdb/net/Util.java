package com.rethinkdb.net;

import com.rethinkdb.RethinkDB;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.ByteArrayInputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class Util {

    public static long deadline(long timeout) {
        return System.currentTimeMillis() + timeout;
    }

    private static Logger log = LoggerFactory.getLogger(Util.class);
    public static ByteBuffer leByteBuffer(int capacity) {
        // Creating the ByteBuffer over an underlying array makes
        // it easier to turn into a string later.
        byte[] underlying = new byte[capacity];
        return ByteBuffer.wrap(underlying)
                .order(ByteOrder.LITTLE_ENDIAN);
    }

    public static String bufferToString(ByteBuffer buf) {
        // This should only be used on ByteBuffers we've created by
        // wrapping an array
        return new String(buf.array(), StandardCharsets.UTF_8);
    }

    public static JSONObject toJSON(String str) {
        return (JSONObject) JSONValue.parse(str);
    }

    public static JSONObject toJSON(ByteBuffer buf) {
        InputStreamReader codepointReader =
                new InputStreamReader(new ByteArrayInputStream(buf.array()));
        return (JSONObject) JSONValue.parse(codepointReader);
    }

    public static <T, P> T convertToPojo(Object value, Optional<Class<P>> pojoClass) {
        return !pojoClass.isPresent() || !(value instanceof Map)
                ? (T) value
                : (T) toPojo(pojoClass.get(), (Map<String, Object>) value);
    }

    public static byte[] toUTF8(String s) {
        return s.getBytes(StandardCharsets.UTF_8);
    }

    public static String fromUTF8(byte[] ba) {
        return new String(ba, StandardCharsets.UTF_8);
    }

    /**
     * Converts a String-to-Object map to a POJO using bean introspection.<br>
     * The POJO's class must be public and satisfy one of the following conditions:<br>
     * 1. Should have a public parameterless constructor and public setters for all properties
     * in the map. Properties with no corresponding entries in the map would have default values<br>
     * 2. Should have a public constructor with parameters matching the contents of the map
     * either by names and value types. Names of parameters are only available since Java 8
     * and only in case <code>javac</code> is run with <code>-parameters</code> argument.<br>
     * If the POJO's class doesn't satisfy the conditions, a ReqlDriverError is thrown.
     * @param <T> POJO's type
     * @param pojoClass POJO's class to be instantiated
     * @param map Map to be converted
     * @return Instantiated POJO
     */
    @SuppressWarnings("unchecked")
    private static <T> T toPojo(Class<T> pojoClass, Map<String, Object> map) {
        // Jackson will throw an error if the POJO is not annotated with an ignore
        // annotation and the server gives a value that is not a field within the POJO To
        // prevent this, we get a list of all the field names from the class, and iterate
        // through the map. If the map contains a key that the does not correlate to a
        // field name, then that entry from the map is removed and we log an error.
        List<String> nameFields = new ArrayList<>();
        Arrays.asList(pojoClass.getDeclaredFields()).forEach(field -> nameFields.add(field.getName()));
        List<String> toRemove = new ArrayList<>();

        map.keySet().forEach(s -> {
            if (!nameFields.contains(s))
            {
                log.error("Got JSON field [" + s + "] from server. POJO does not contain field, removing from map!");
                toRemove.add(s);
            }
        });
        toRemove.forEach(map::remove);

        return RethinkDB.getObjectMapper().convertValue(map, pojoClass);
    }
}



