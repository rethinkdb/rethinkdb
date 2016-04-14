package com.rethinkdb;

import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.net.Connection;
import com.rethinkdb.net.Cursor;

import java.beans.BeanInfo;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.reflect.Array;
import java.lang.reflect.Method;
import java.time.Instant;
import java.time.OffsetDateTime;
import java.time.ZoneOffset;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;
import java.util.stream.Collectors;
import java.util.stream.LongStream;

public final class TestingCommon {

    // Python test conversion compatibility definitions

    public static int len(List array) {
        return array.size();
    }

    public static class Lst {
        final List lst;
        public Lst(List lst) {
            this.lst = lst;
        }

        public boolean equals(Object other) {
            return lst.equals(other);
        }
    }

    public static class Bag {
        final List lst;
        public Bag(List lst) {
            stringSort(lst);
            this.lst = lst;
        }

        public boolean equals(Object other) {
            if(!(other instanceof List)) {
                return false;
            }
            List otherList = (List) other;
            stringSort(otherList);
            return lst.equals(otherList);
        }

        public void stringSort(List input) {
            Collections.sort(input, (Object a, Object b) ->
                    a.toString().compareTo(b.toString()));
        }

        public String toString() {
            return "Bag(" + lst + ")";
        }
    }

    public static Bag bag(List lst) {
        return new Bag(lst);
    }

    public static class Partial {}

    public static class PartialLst extends Partial {
        final List lst;
        public PartialLst(List lst){
            this.lst = lst;
        }

        public String toString(){
            return "PartialLst(" + lst + ")";
        }

        public boolean equals(Object other) {
            if(!(other instanceof List)) {
                return false;
            }
            List otherList = (List) other;
            if(lst.size() > otherList.size()){
                return false;
            }
            for(Object item: lst) {
                if(otherList.indexOf(item) == -1){
                    return false;
                }
            }
            return true;
        }
    }

    public static PartialLst partial(List lst) {
        return new PartialLst(lst);
    }

    public static class Dct {
        final Map dct;
        public Dct(Map dct){
            this.dct = dct;
        }

        public boolean equals(Object other) {
            return dct.equals(other);
        }
    }

    public static class PartialDct extends Partial {
        final Map dct;
        public PartialDct(Map dct){
            this.dct = dct;
        }

        public boolean equals(Object other_) {
            if(!(other_ instanceof Map)) {
                return false;
            }
            Map other = ((Map) other_);
            for(Map.Entry<Object,Object> entry : ((Map<Object,Object>)dct).entrySet()){
                if(!other.containsKey(entry.getKey())){
                    System.out.println("Obtained didn't have key " + entry.getKey());
                    return false;
                }
                Object val = other.get(entry.getKey());
                if(entry.getValue() == null && val == null){
                    continue;
                }
                if(entry.getValue() == null && val != null ||
                        val == null && entry.getValue() != null){
                    System.out.println("One was null and the other wasn't for key " + entry.getKey());
                    return false;
                }
                if(!entry.getValue().equals(val)){
                    System.out.println("Weren't equal: " + entry.getValue() + " and " + val);
                    return false;
                }
            }
            return true;
        }

        public String toString() {
            return "PartialDct(" + dct + ")";
        }
    }
    public static PartialDct partial(Map dct) {
        return new PartialDct(dct);
    }

    public static class ArrLen {
        final int length;
        final Object thing;
        public ArrLen(int length, Object thing) {
            this.length = length;
            this.thing = thing;
        }

        public String toString() {
            return "ArrLen(length="+length+" of "+thing+")";
        }

        public boolean equals(Object other) {
            if(!(other instanceof List)){
                return false;
            }
            List otherList = (List) other;
            if(length != otherList.size()) {
                return false;
            }
            if(thing == null) {
                return true;
            }
            for(Object item: otherList) {
                if(!thing.equals(item)){
                    return false;
                }
            }
            return true;
        }
    }

    public static ArrLen arrlen(Long length, Object thing) {
        return new ArrLen(length.intValue(), thing);
    }

    public static ArrLen arrlen(Long length) {
        return new ArrLen(length.intValue(), null);
    }

    public static class UUIDMatch {
        static final String uuidRgx =
                "[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}";

        public boolean equals(Object other) {
            if(!(other instanceof String)) {
                return false;
            }
            return Pattern.matches(uuidRgx, (String) other);
        }

        public String toString() {
            return "Uuid()";
        }
    }

    public static UUIDMatch uuid() {
        return new UUIDMatch();
    }

    public static class IntCmp {
        final Long nbr;
        public IntCmp(Long nbr) {
            this.nbr = nbr;
        }
        public boolean equals(Object other) {
            return nbr.equals(other);
        }
    }

    public static IntCmp int_cmp(Long nbr) {
        return new IntCmp(nbr);
    }

    public static class FloatCmp {
        final Double nbr;
        public FloatCmp(Double nbr) {
            this.nbr = nbr;
        }
        public boolean equals(Object other) {
            return nbr.equals(other);
        }
    }

    public static FloatCmp float_cmp(Double nbr) {
        return new FloatCmp(nbr);
    }

    public static class Regex {
        public final Pattern pattern;

        public Regex(String regexString){
            this.pattern = Pattern.compile(regexString, Pattern.DOTALL);
        }

        public String toString(){
            return "Regex(" + pattern + ")";
        }

        public boolean equals(Object other){
            if(!(other instanceof String)){
                return false;
            }else{
                return pattern.matcher((String) other).matches();
            }
        }
    }

    public static Regex regex(String regexString){
        return new Regex(regexString);
    }

    public static class Err {
        public final Class clazz;
        public final String message;
        public final Pattern inRegex = Pattern.compile(
                "^(?<message>[^\n]*?)(?: in)?:\n.*$",
                Pattern.DOTALL);
        public final Pattern assertionRegex = Pattern.compile(
                "^(?<message>[^\n]*?)\nFailed assertion:.*$",
                Pattern.DOTALL);

        public String toString() {
            return "Err(" + clazz + ": " + message + ")";
        }

        public Err(String classname, String message) {
            String clazzname = "com.rethinkdb.gen.exc." + classname;
            try {
                this.clazz = Class.forName(clazzname);
            } catch (ClassNotFoundException cnfe) {
                throw new RuntimeException("Bad exception class: "+clazzname, cnfe);
            }
            this.message = message;
        }

        public boolean equals(Object other) {
            if(!clazz.isInstance(other)) {
                System.out.println("Classes didn't match: "
                        + clazz + " vs. " + other.getClass());
                return false;
            }
            String otherMessage = ((Exception) other).getMessage();
            otherMessage = inRegex.matcher(otherMessage)
                    .replaceFirst("${message}:");
            otherMessage = assertionRegex.matcher(otherMessage)
                    .replaceFirst("${message}");
            return message.equals(otherMessage);
        }
    }

    public static Err err(String classname, String message) {
        return new Err(classname, message);
    }

    public static Err err(String classname, String message, List _unused) {
        return err(classname, message);
    }

    public static class ErrRegex {
        public final Class clazz;
        public final String message_rgx;

        public ErrRegex(String classname, String message_rgx) {
            String clazzname = "com.rethinkdb.gen.exc." + classname;
            try {
                this.clazz = Class.forName(clazzname);
            } catch (ClassNotFoundException cnfe) {
                throw new RuntimeException("Bad exception class: "+clazzname, cnfe);
            }
            this.message_rgx = message_rgx;
        }

        public boolean equals(Object other) {
            if(!clazz.isInstance(other)) {
                return false;
            }
            return Pattern.matches(message_rgx, ((Exception)other).getMessage());
        }
    }

    public static ErrRegex err_regex(String classname, String message_rgx) {
        return new ErrRegex(classname, message_rgx);
    }

    public static ErrRegex err_regex(String classname, String message_rgx, Object dummy) {
        // Some invocations pass a stack frame as a third argument
        return new ErrRegex(classname, message_rgx);
    }

    public static ArrayList fetch(Object cursor_, long limit) throws Exception {
        if(limit < 0) {
            limit = Long.MAX_VALUE;
        }
        Cursor cursor = (Cursor) cursor_;
        long total = 0;
        ArrayList result = new ArrayList((int) limit);
        for(long i = 0; i < limit; i++) {
            if(!cursor.hasNext()){
                break;
            }
            result.add(cursor.next(500));
        }
        return result;
    }

    public static ArrayList fetch(Cursor cursor) throws Exception {
        return fetch(cursor, -1);
    }

    public static Object runOrCatch(Object query, OptArgs runopts, Connection conn) {
        if(query == null) {
            return null;
        }
        if(query instanceof List) {
            return query;
        }
        try {
            Object res = ((ReqlAst)query).run(conn, runopts);
            if(res instanceof com.rethinkdb.net.Cursor) {
                ArrayList ret = new ArrayList();
                ((com.rethinkdb.net.Cursor) res).forEachRemaining(ret::add);
                return ret;
            }else{
                return res;
            }
        } catch (Exception e) {
            return e;
        }
    }

    public static LongStream range(long start, long stop) {
        return LongStream.range(start, stop);
    }

    public static List list(LongStream str) {
        return str.boxed().collect(Collectors.toList());
    }

    public static class sys {
        public static class floatInfo {
            public static final Double min = Double.MIN_VALUE;
            public static final Double max = Double.MAX_VALUE;
        }
    }

    public static ZoneOffset PacificTimeZone() {
        return ZoneOffset.ofHours(-7);
    }

    public static ZoneOffset UTCTimeZone() {
        return ZoneOffset.ofHours(0);
    }

    public static class datetime {
        public static OffsetDateTime fromtimestamp(double seconds, ZoneOffset offset) {
            Instant inst = Instant.ofEpochMilli(
                    (new Double(seconds * 1000)).longValue());
            return OffsetDateTime.ofInstant(inst, offset);
        }

        public static OffsetDateTime now() {
            return OffsetDateTime.now();
        }
    }

    public static class ast {
        public static ZoneOffset rqlTzinfo(String offset) {
            if(offset.equals("00:00")){
                offset = "Z";
            }
            return ZoneOffset.of(offset);
        }
    }

    public static Double float_(Double nbr) {
        return nbr;
    }

    public static Object wait_(long length) {
        try {
            Thread.sleep(length * 1000);
        }catch(InterruptedException ie) {}
        return null;
    }

    public static Object maybeRun(Object query, Connection conn, OptArgs runopts) {
        if (query instanceof ReqlAst) {
            return ((ReqlAst)query).run(conn, runopts);
        } else {
            return query;
        }
    }

    public static Object maybeRun(Object query, Connection conn) {
        if (query instanceof ReqlAst) {
            return ((ReqlAst)query).run(conn);
        } else {
            return query;
        }
    }

    public static Object AnythingIsFine = new Object() {
        public boolean equals(Object other) {
            return true;
        }
        public String toString() {
            return "AnythingIsFine";
        }
    };

    /**
     * deep compare java bean, generic List, array
     * @param obj1
     * @param obj2
     * @return
     */
    public static boolean smartDeepEquals(Object obj1, Object obj2) {
        if ((obj1 == null) != (obj2 == null))
            return false;
        if (obj1 == null)
            return true;
        if (obj1.equals(obj2))
            return true;

        /**
         * compare List<T>  (e.g. ArrayList<SomeClass>)
         */
        if (obj1 instanceof List && obj2 instanceof List) {
            List list1 = (List) obj1;
            List list2 = (List) obj2;

            int len = list1.size();
            if (list2.size() != len)
                return false;

            for (int i = 0; i < len; i++) {
                if (!smartDeepEquals(list1.get(i), list2.get(i)))
                    return false;
            }
            return true;
        }

        Class clazz1 = obj1.getClass();

        /**
         * compare Xxx[] or xxx[]    (xxx means primitive type)
         */
        if (clazz1.isArray() && obj2.getClass().isArray()) {
            int len = Array.getLength(obj1);
            if (Array.getLength(obj2) != len)
                return false;

            for (int i = 0; i < len; i++) {
                if (!smartDeepEquals(Array.get(obj1, i), Array.get(obj2, i)))
                    return false;
            }
            return true;
        }
        /**
         * compare java beans
         */
        if (clazz1 == obj2.getClass()) {
            try {
                BeanInfo info = Introspector.getBeanInfo(clazz1);

                int beanPropertyCount = 0;
                for (PropertyDescriptor descriptor : info.getPropertyDescriptors()) {
                    Method reader = descriptor.getReadMethod();
                    if (reader != null && descriptor.getWriteMethod() != null)
                        beanPropertyCount++;
                }
                if (beanPropertyCount == 0)
                    return false;

                for (PropertyDescriptor descriptor : info.getPropertyDescriptors()) {
                    Method reader = descriptor.getReadMethod();
                    if (reader != null && descriptor.getWriteMethod() != null) {
                        Object prop1 = reader.invoke(obj1);
                        Object prop2 = reader.invoke(obj2);

                        if (!smartDeepEquals(prop1, prop2))
                            return false;
                    }
                }
                return true;
            } catch (Exception e) {
                return false;
            }
        }

        return false;
    }
}
