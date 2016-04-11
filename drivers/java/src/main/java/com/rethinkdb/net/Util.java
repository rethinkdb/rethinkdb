package com.rethinkdb.net;

import com.rethinkdb.gen.exc.ReqlDriverError;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.io.ByteArrayInputStream;
import java.io.InputStreamReader;
import java.lang.reflect.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.time.*;
import java.time.format.DateTimeParseException;
import java.util.*;
import java.util.stream.Stream;

public class Util {

    public static long deadline(long timeout) {
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

    public static <T> T convertToPojo(Object value, Class pojoClass) {
        return !(value instanceof Map)
                ? (T) value
                : (T) toPojo(pojoClass, (Map<String, Object>) value);
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
        try {
            if (map == null) {
                return null;
            }

            if (LocalDate.class.equals(pojoClass)) {
                return (T) LocalDate.of(
                        ((Long) map.get("year")).intValue(),
                        ((Long) map.get("monthValue")).intValue(),
                        ((Long) map.get("dayOfMonth")).intValue());
            }
            else if (LocalTime.class.equals(pojoClass)) {
                return (T) LocalTime.of(
                        ((Long) map.get("hour")).intValue(),
                        ((Long) map.get("minute")).intValue(),
                        ((Long) map.get("second")).intValue(),
                        ((Long) map.get("nano")).intValue());
            }
            else if (LocalDateTime.class.equals(pojoClass)) {
                return (T) LocalDateTime.of(
                        ((Long) map.get("year")).intValue(),
                        ((Long) map.get("monthValue")).intValue(),
                        ((Long) map.get("dayOfMonth")).intValue(),
                        ((Long) map.get("hour")).intValue(),
                        ((Long) map.get("minute")).intValue(),
                        ((Long) map.get("second")).intValue(),
                        ((Long) map.get("nano")).intValue());
            }
            else if (OffsetDateTime.class.equals(pojoClass)) {
                String zoneOffsetId = (String)((Map<String, Object>) map.get("offset")).get("id");
                return (T) OffsetDateTime.of(
                        ((Long) map.get("year")).intValue(),
                        ((Long) map.get("monthValue")).intValue(),
                        ((Long) map.get("dayOfMonth")).intValue(),
                        ((Long) map.get("hour")).intValue(),
                        ((Long) map.get("minute")).intValue(),
                        ((Long) map.get("second")).intValue(),
                        ((Long) map.get("nano")).intValue(),
                        ZoneOffset.of(zoneOffsetId)
                );
            }
            else if (ZonedDateTime.class.equals(pojoClass)) {
                String zoneId = (String)((Map<String, Object>) map.get("zone")).get("id");
                return (T) ZonedDateTime.of(
                        ((Long) map.get("year")).intValue(),
                        ((Long) map.get("monthValue")).intValue(),
                        ((Long) map.get("dayOfMonth")).intValue(),
                        ((Long) map.get("hour")).intValue(),
                        ((Long) map.get("minute")).intValue(),
                        ((Long) map.get("second")).intValue(),
                        ((Long) map.get("nano")).intValue(),
                        ZoneId.of(zoneId)
                );
            }
            else if (Date.class.isAssignableFrom(pojoClass)) {
                /**
                 * map: { date: 27, day: 0, hours: 2, minutes: 50, month: 2, seconds: 46,
                 *       time: 1459014646488, year: 116, timezoneOffset: -540 }
                 * if not do conversion explicitly here, then later logic in constructViaPublicParameterlessConstructor
                 * will treat Date as a normal java bean, ridiculously, it will call 8 setXxx to the Date instance,
                 * e.g. d.setDate(27);d.setDay(0);d.setHours(2)......
                 */
                long epochMilli = (Long) map.get("time");
                return (T) new Date(epochMilli);
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
                /**
                 * If the property of a java bean(or says POJO) is a generic List class, e.g.
                 *  java.util.List<com.rethinkdb.TestPojoInner>
                 * So far rethinkdb java driver will only convert db data to following type for us:
                 *  java.util.List<HashMap<String, Object>>
                 * So it's better convert each item in the list to com.rethinkdb.TestPojoInner.
                 *
                 * I do want to directly convert the `value` to the generic type, but unfortunately,
                 * Java can not load generic class by Class.forName("java.util.List<com.rethinkdb.TestPojoInner>"),
                 * so i have to use the fixed class java.util.List, and pass listItemClassName to smartCast
                 * and let it convert each HashMap to TestPojoInner
                 */
                String listItemClassName = null;
                if (value instanceof List) {
                    String genName = descriptor.getWriteMethod().getGenericParameterTypes()[0].getTypeName();
                    int len = valueClass.getName().length();
                    int genLen = genName.length();
                    if (genLen > len + 2 && genName.charAt(len) == '<' && genName.charAt(genLen - 1) == '>') {
                        listItemClassName = genName.substring(len + 1, genLen - 1);
                    }
                }
                writer.invoke(pojo, value instanceof Map
                        ? toPojo(valueClass, (Map<String, Object>) value)
                        : smartCast(valueClass, value, listItemClassName));
            }
        }

        return pojo;
    }

    static java.text.SimpleDateFormat dtf = new java.text.SimpleDateFormat("EEE MMM dd kk:mm:ss z yyyy", Locale.ENGLISH);

    private static Object smartCast(Class valueClass, Object value, String listItemClassName) {
        try {
            value = valueClass.cast(value);

            if (listItemClassName != null) {
                /**
                 * convert each list element to wanted item class
                 */
                if (value instanceof List) {

                    Class innerClass = null;
                    try {
                        innerClass = Class.forName(listItemClassName);
                    } catch (ClassNotFoundException e) {
                        e = e; //just for setting breakpoint easier
                    }

                    if (innerClass != null) {
                        List list = (List) value;
                        int len = list.size();
                        for (int i = 0; i < len; i++) {
                            Object obj = list.get(i);
                            Object convertedObj = convertToPojo(obj, innerClass);
                            if (convertedObj != obj)
                                list.set(i, convertedObj);
                        }
                    }
                }
            }
            return value;
        }
        catch (ClassCastException ex) {
            if (valueClass.isEnum()) {
                try {
                    return Enum.valueOf(valueClass, value.toString());
                } catch(IllegalArgumentException e) {
                    return Enum.valueOf(valueClass, value.toString().toUpperCase());
                }
            }
            else if (OffsetDateTime.class.equals(valueClass)) {
                return OffsetDateTime.ofInstant(parseDate(value), ZoneId.systemDefault());
            }
            else if (LocalDateTime.class.equals(valueClass)) {
                return LocalDateTime.ofInstant(parseDate(value), ZoneId.systemDefault());
            }
            else if (LocalDate.class.equals(valueClass)) {
                return LocalDateTime.ofInstant(parseDate(value), ZoneId.systemDefault()).toLocalDate();
            }
            else if (LocalTime.class.equals(valueClass)) {
                try {
                    return LocalTime.parse(value.toString());
                } catch (DateTimeParseException e) {
                    throw new ClassCastException("Can not convert \"" + value + "\" to LocalTime. Valid data samples: 23:59:59.007");
                }
            }
            else if (ZonedDateTime.class.equals(valueClass)) {
                return ZonedDateTime.ofInstant(parseDate(value), ZoneId.systemDefault());
            }
            else if (Boolean.class.equals(valueClass) || boolean.class.equals(valueClass)) {
                return Boolean.valueOf(value.toString());
            }
            else if (Integer.class.equals(valueClass) || int.class.equals(valueClass)) {
                return Integer.valueOf(value.toString());
            }
            else if (Long.class.equals(valueClass) || long.class.equals(valueClass)) {
                return Double.valueOf(value.toString()).longValue();
            }
            else if (Double.class.equals(valueClass) || double.class.equals(valueClass)) {
                return Double.valueOf(value.toString());
            }
            else if (Float.class.equals(valueClass) || float.class.equals(valueClass)) {
                return Float.valueOf(value.toString());
            }
            else if (Short.class.equals(valueClass) || short.class.equals(valueClass)) {
                return Short.valueOf(value.toString());
            }
            else if (Byte.class.equals(valueClass) || byte.class.equals(valueClass)) {
                return Byte.valueOf(value.toString());
            }
            else if (java.math.BigDecimal.class.isAssignableFrom(valueClass)) {
                return new java.math.BigDecimal(value.toString());
            }
            else if (java.math.BigInteger.class.isAssignableFrom(valueClass)) {
                return new java.math.BigInteger(value.toString());
            }
            else if (java.util.Date.class.isAssignableFrom(valueClass)) {
                return new Date(parseDate(value).toEpochMilli());
            }
            else if (valueClass.isArray() && value instanceof List) {
                List list = (List)value;
                int len = list.size();
                Class aryComponentType = valueClass.getComponentType();
                Object[] ary = (Object[])Array.newInstance(aryComponentType, list.size());
                for(int i = 0; i < len; i++) {
                    ary[i] = convertToPojo(list.get(i), aryComponentType);
                }
                return ary;
            }
            else
                throw ex;
        }
    }

    private static Instant parseDate(Object value) {
        try {
            return Instant.ofEpochMilli((long) value);
        } catch (ClassCastException e) {
            String sValue = value.toString();
            try {
                //try "2016-03-16"
                return LocalDate.parse(sValue).atStartOfDay().atZone(ZoneId.systemDefault()).toInstant();
            } catch (DateTimeParseException e1) {
                try {
                    //try 2016-03-16T09:58:59.007
                    return LocalDateTime.parse(sValue).atZone(ZoneId.systemDefault()).toInstant();
                } catch (DateTimeParseException e2) {
                    try {
                        //try 2016-03-16T09:58:59.007+09:00
                        return OffsetDateTime.parse(sValue).toInstant();
                    } catch (DateTimeParseException e3) {
                        try {
                            //try 2016-03-16T09:58:59.007+09:00[Asia/Tokyo]
                            return ZonedDateTime.parse(value.toString()).toInstant();
                        } catch (DateTimeParseException e4) {
                            try {
                                //try "Sat Dec 30 09:58:59 JST 2016"  (the result of new Date().toString())
                                return dtf.parse(sValue).toInstant();
                            } catch (java.text.ParseException e5) {
                                throw new ClassCastException("Can not convert \"" + sValue + "\" to date related type. Valid data samples: 2016-03-16, 2016-03-16T09:58:59, 2016-03-16T09:58:59.007, 2016-03-16T09:58:59.007+09:00, Sat Dec 30 09:58:59 JST 2016");
                            }
                        }
                    }
                }
            }
        }
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
