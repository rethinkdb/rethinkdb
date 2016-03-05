package com.rethinkdb.net;

import com.rethinkdb.gen.exc.ReqlDriverError;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.reflect.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.stream.Stream;

public class Util {

    public static long deadline(long timeout){
        return System.currentTimeMillis() + timeout;
    }

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

    public static <T, P> T convertToPojo(Object value, Optional<Class<P>> pojoClass) {
        return !pojoClass.isPresent() || !(value instanceof Map)
                ? (T) value
                : (T) toPojo(pojoClass.get(), (Map<String, Object>) value);
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
        try {
            if (map == null) {
                return null;
            }

            if (!Modifier.isPublic(pojoClass.getModifiers())) {
                throw new IllegalAccessException(String.format("%s should be public", pojoClass));
            }

            Constructor[] allConstructors = pojoClass.getDeclaredConstructors();

            if (getPublicParameterlessConstructors(allConstructors).count() == 1) {
                return (T) constructViaPublicParameterlessConstructor(pojoClass, map);
            }

            Constructor[] constructors = getSuitablePublicParametrizedConstructors(allConstructors, map);

            if (constructors.length == 1) {
                return (T) constructViaPublicParametrizedConstructor(constructors[0], map);
            }

            throw new IllegalAccessException(String.format(
                    "%s should have a public parameterless constructor " +
                            "or a public constructor with %d parameters", pojoClass, map.keySet().size()));
        } catch (InstantiationException | IllegalAccessException | IntrospectionException | InvocationTargetException e) {
            throw new ReqlDriverError("Can't convert %s to a POJO: %s", map, e.getMessage());
        }
    }

    private static Stream<Constructor> getPublicParameterlessConstructors(Constructor[] constructors) {
        return Arrays.stream(constructors).filter(constructor ->
                Modifier.isPublic(constructor.getModifiers()) &&
                        constructor.getParameterCount() == 0
        );
    }

    @SuppressWarnings("unchecked")
    private static Object constructViaPublicParameterlessConstructor(Class pojoClass, Map<String, Object> map)
            throws IllegalAccessException, InstantiationException, IntrospectionException, InvocationTargetException {
        Object pojo = pojoClass.newInstance();
        BeanInfo info = Introspector.getBeanInfo(pojoClass);

        for (PropertyDescriptor descriptor : info.getPropertyDescriptors()) {
            String propertyName = descriptor.getName();

            if (!map.containsKey(propertyName)) {
                continue;
            }

            Method writer = descriptor.getWriteMethod();

            if (writer != null && writer.getDeclaringClass() == pojoClass) {
                Object value = map.get(propertyName);
                Class valueClass = writer.getParameterTypes()[0];

                if (valueClass.isEnum() && value instanceof String)
                    writer.invoke(pojo, Enum.valueOf((Class<? extends Enum>)valueClass, (String)value));
                else
                    writer.invoke(pojo, value instanceof Map
                            ? toPojo(valueClass, (Map<String, Object>) value)
                            : valueClass.cast(value));
            }
        }

        return pojo;
    }

    private static Constructor[] getSuitablePublicParametrizedConstructors(Constructor[] allConstructors, Map<String, Object> map) {
        return Arrays.stream(allConstructors).filter(constructor ->
                Modifier.isPublic(constructor.getModifiers()) &&
                        areParametersMatching(constructor.getParameters(), map)
        ).toArray(Constructor[]::new);
    }

    private static boolean areParametersMatching(Parameter[] parameters, Map<String, Object> values) {
        return Arrays.stream(parameters).allMatch(parameter ->
                values.containsKey(parameter.getName()) &&
                        values.get(parameter.getName()).getClass() == parameter.getType()
        );
    }

    private static Object constructViaPublicParametrizedConstructor(Constructor constructor, Map<String, Object> map)
            throws IllegalAccessException, InstantiationException, IntrospectionException, InvocationTargetException {
        Object[] values = Arrays.stream(constructor.getParameters()).map(parameter -> {
            Object value = map.get(parameter.getName());

            return value instanceof Map
                    ? toPojo(value.getClass(), (Map<String, Object>) value)
                    : value;
        }).toArray();

        return constructor.newInstance(values);
    }
}
