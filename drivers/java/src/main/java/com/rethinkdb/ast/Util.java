package com.rethinkdb.ast;

import com.rethinkdb.gen.ast.*;
import com.rethinkdb.gen.exc.ReqlDriverCompileError;
import com.rethinkdb.gen.exc.ReqlDriverError;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.MapObject;
import com.rethinkdb.model.ReqlLambda;
import com.rethinkdb.net.Cursor;

import java.lang.*;
import java.lang.reflect.*;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.time.LocalDateTime;
import java.time.OffsetDateTime;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Stream;


public class Util {
    private Util(){}
    /**
     * Coerces objects from their native type to ReqlAst
     *
     * @param val val
     * @return ReqlAst
     */
    public static ReqlAst toReqlAst(Object val) {
        return toReqlAst(val, 100);
    }

    public static ReqlExpr toReqlExpr(Object val){
        ReqlAst converted = toReqlAst(val);
        if(converted instanceof ReqlExpr){
            return (ReqlExpr) converted;
        }else{
            throw new ReqlDriverError("Cannot convert %s to ReqlExpr", val);
        }
    }

    private static ReqlAst toReqlAst(Object val, int remainingDepth) {
        if (remainingDepth <= 0) {
            throw new ReqlDriverCompileError("Recursion limit reached converting to ReqlAst");
        }
        if (val instanceof ReqlAst) {
            return (ReqlAst) val;
        }

        if (val instanceof Object[]){
            Arguments innerValues = new Arguments();
            for (Object innerValue : Arrays.asList((Object[])val)){
                innerValues.add(toReqlAst(innerValue, remainingDepth - 1));
            }
            return new MakeArray(innerValues, null);
        }

        if (val instanceof List) {
            Arguments innerValues = new Arguments();
            for (java.lang.Object innerValue : (List) val) {
                innerValues.add(toReqlAst(innerValue, remainingDepth - 1));
            }
            return new MakeArray(innerValues, null);
        }

        if (val instanceof Map) {
            Map<String, ReqlAst> obj = new MapObject();
            for (Map.Entry<Object, Object> entry : (Set<Map.Entry>) ((Map) val).entrySet()) {
                if (!(entry.getKey() instanceof String)) {
                    throw new ReqlDriverCompileError("Object keys can only be strings");
                }

                obj.put((String) entry.getKey(), toReqlAst(entry.getValue()));
            }
            return MakeObj.fromMap(obj);
        }

        if (val instanceof ReqlLambda) {
            return Func.fromLambda((ReqlLambda) val);
        }

        final DateTimeFormatter fmt = DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss.SSSX");

        if (val instanceof LocalDateTime) {
            ZoneId zid = ZoneId.systemDefault();
            DateTimeFormatter fmt2 = fmt.withZone(zid);
            return Iso8601.fromString(((LocalDateTime) val).format(fmt2));
        }
        if (val instanceof ZonedDateTime) {
            return Iso8601.fromString(((ZonedDateTime) val).format(fmt));
        }
        if (val instanceof OffsetDateTime) {
            return Iso8601.fromString(((OffsetDateTime) val).format(fmt));
        }

        if (val instanceof Integer) {
            return new Datum((Integer) val);
        }

        if (val instanceof Number) {
            return new Datum((Number) val);
        }

        if (val instanceof Boolean) {
            return new Datum((Boolean) val);
        }

        if (val instanceof String) {
            return new Datum((String) val);
        }

        if (val == null) {
            return new Datum(null);
        }

        // val is a non-null POJO, let's introspect its public properties
        return toReqlAst(toMap(val));
    }

    /**
     * Converts a POJO to a map of its public properties collected using bean introspection.<br>
     * The POJO's class must be public, or a ReqlDriverError would be thrown.<br>
     * Numeric properties should be Long instead of Integer
     * @param pojo POJO to be introspected
     * @return Map of POJO's public properties
     */
    private static Map<String, Object> toMap(Object pojo) {
        try {
            Map<String, Object> map = new HashMap<String, Object>();
            Class pojoClass = pojo.getClass();

            if (!Modifier.isPublic(pojoClass.getModifiers())) {
                throw new IllegalAccessException(String.format("%s's class should be public", pojo));
            }

            BeanInfo info = Introspector.getBeanInfo(pojoClass);

            for (PropertyDescriptor descriptor : info.getPropertyDescriptors()) {
                Method reader = descriptor.getReadMethod();

                if (reader != null && reader.getDeclaringClass() == pojoClass) {
                    Object value = reader.invoke(pojo);

                    if (value instanceof Integer) {
                        throw new IllegalAccessException(String.format(
                                "Make %s of %s Long instead of Integer", reader.getName(), pojo));
                    }

                    map.put(descriptor.getName(), value);
                }
            }

            return map;
        }
        catch (IntrospectionException | IllegalAccessException | InvocationTargetException e) {
            throw new ReqlDriverError("Can't convert %s to a ReqlAst: %s", pojo, e.getMessage());
        }
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
     * @param pojoClass POJO's class to be instantiated
     * @param map Map to be converted
     * @return Instantiated POJO
     */
    @SuppressWarnings("unchecked")
    public static <T> T toPojo(Class pojoClass, Map<String, Object> map) {
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

    /**
     * Converts a cursor of String-to-Object maps to a list of POJOs using {@link #toPojo(Class, Map) toPojo}.
     * @param cursor Cursor to be iterated
     * @param pojoClass POJO's class
     * @return List of POJOs
     */
    @SuppressWarnings("unchecked")
    public static <T> List<T> toPojoList(Cursor cursor, Class<T> pojoClass) {
        List<T> list = new ArrayList<T>();

        for (Object value : cursor) {
            if (!(value instanceof Map)) {
                throw new ReqlDriverError("Can't convert %s to a POJO, should be of Map<String, Object>", value);
            }

            list.add(toPojo(pojoClass, (Map<String, Object>) value));
        }

        return list;
    }
}
