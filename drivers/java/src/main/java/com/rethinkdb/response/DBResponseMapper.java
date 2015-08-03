package com.rethinkdb.response;

import com.rethinkdb.ReqlError;

import java.lang.reflect.Field;
import java.util.*;

public class DBResponseMapper {

    /**
     * Maps a ProtoBuf Datum object to a java object
     *
     * @param datum datum
     * @return DBObject
     */
    public static <T> T fromDatumObject(Datum datum) {
        throw new RuntimeException("fromDatumObject not implemented");
        // // Null is Null
        // if (datum.isNull()) {
        //     return null;
        // }

        // // Number, Str, bool go to simple java datatypes
        // if (datum.isNum() || datum.isStr() || datum.isBool()) {
        //     return (T)handleType(datum);
        // }

        // // R_OBJECT is Map
        // if (datum.isObject()) {
        //     Map<String, Object> repr = datum.getHashMap();
        //     return (T)repr;
        // }

        // // Array goes to List
        // if (datum.isArray()) {
        //     return (T)makeArray(datum.getArrayList());
        // }

        // throw new ReqlError("Can't map datum to JavaObject for {}" + datum.getType());
    }

    private static Date asDate(Datum datum) {
        String timezone = "";
        Double epoch_time = 0.0;

        throw new RuntimeException("asDate not implemented");
        // for (Q2L.Datum.AssocPair assocPair : datum.getRObjectList()) {

        //     if (assocPair.getKey().equals("epoch_time")) {
        //         epoch_time = assocPair.getVal().getRNum();
        //     }
        //     if (assocPair.getKey().equals("timezone")) {
        //         timezone = assocPair.getVal().getRStr();
        //     }

        // }

        // try {
        //     Calendar calendar = Calendar.getInstance();
        //     calendar.setTimeInMillis(epoch_time.longValue() * 1000);
        //     calendar.setTimeZone(TimeZone.getTimeZone(timezone));
        //     return calendar.getTime();
        // }
        // catch (Exception ex) {
        //     throw new ReqlError("Error handling date",ex);
        // }
    }

    /**
     * Maps a list of Datum Objects to a list of java objects
     *
     * @param datums datum objects
     * @return DBObject
     */
    public static <T> List<T> fromDatumObjectList(List<Datum> datums) {
        throw new RuntimeException("fromDatumObjectList not implemented");
        // List<T> results = new ArrayList<T>();
        // for (Q2L.Datum datum : datums) {
        //     results.add((T)fromDatumObject(datum));
        // }
        // return results;
    }

    private static Object handleType(Datum val) {
        throw new RuntimeException("handleType not implemented");
        // switch (val.getType()) {
        //     case R_OBJECT:
        //         return fromDatumObject(val);
        //     case R_STR:
        //         return val.getRStr();
        //     case R_BOOL:
        //         return val.getRBool();
        //     case R_NULL:
        //         return null;
        //     case R_NUM:
        //         return val.getRNum();
        //     case R_ARRAY:
        //         return makeArray(val.getRArrayList());
        //     case R_JSON:
        //     default:
        //         throw new RuntimeException("Not implemented" + val.getType());
        // }
    }

    private static List<Object> makeArray(List<Datum> elements) {
        List<Object> objects = new ArrayList<Object>();
        for (Datum element : elements) {
            objects.add(handleType(element));
        }
        return objects;
    }

    /**
     * Populates the fields in to based on values out of from
     *
     * @param to   the object to map into
     * @param from the object to map from
     * @param <T>  the type of the into object
     * @return the into object
     */
    public static <T> T populateObject(T to, Map<String,Object> from) {
        for (Field field : to.getClass().getDeclaredFields()) {
            try {
                field.setAccessible(true);

                Object result = convertField(field, from);
                if (result != null || !field.getType().isPrimitive()) {
                    field.set(to, result);
                }

            } catch (IllegalAccessException e) {
                throw new ReqlError("Error populating from DBObject: " + field.getName(), e);
            }
        }
        return to;
    }

    public static <T> List<T> populateList(Class<T> clazz, List<Map<String, Object>> from) {
        List<T> results = new ArrayList<T>();
        for (Map<String, Object> stringObjectMap : from) {
            try {
                results.add(populateObject(clazz.newInstance(),stringObjectMap));
            } catch (InstantiationException e) {
                throw new ReqlError("Error instantiating " + clazz, e);
            } catch (IllegalAccessException e) {
                throw new ReqlError("Illegal access on " + clazz, e);
            }
        }
        return results;
    }



    private static Object convertField(Field toField, Map<String, Object> from) {
        if (from.get(toField.getName()) == null) {
            return null;
        }
        if (toField.getType().equals(Integer.class) || toField.getType().equals(int.class)) {
            return ((Number) from.get(toField.getName())).intValue();
        }
        if (toField.getType().equals(Float.class) || toField.getType().equals(float.class)) {
            return ((Number) from.get(toField.getName())).floatValue();
        }
        return from.get(toField.getName());
    }


}
