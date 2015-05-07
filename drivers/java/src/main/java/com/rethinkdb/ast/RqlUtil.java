package com.rethinkdb.ast;

import com.rethinkdb.RethinkDB;
import com.rethinkdb.RethinkDBException;
import com.rethinkdb.model.RqlFunction;
import com.rethinkdb.ast.gen.Datum;
import com.rethinkdb.ast.gen.Func;
import com.rethinkdb.ast.gen.MakeArray;
import com.rethinkdb.ast.gen.MakeObj;
import com.rethinkdb.ast.gen.ISO8601;
import com.rethinkdb.model.RqlLambda;

import java.lang.*;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.*;


public class RqlUtil {
    /**
     * Coerces objects from their native type to RqlAst
     *
     * @param val val
     * @return RqlAst
     */
    public static RqlAst toRqlAst(java.lang.Object val) {
        return toRqlAst(val, 20);
    }

    private static RqlAst toRqlAst(java.lang.Object val, int remainingDepth) {
        if (val instanceof RqlAst) {
            return (RqlAst) val;
        }

        if (val instanceof List) {
            List<java.lang.Object> innerValues = new ArrayList<Object>();
            for (java.lang.Object innerValue : (List) val) {
                innerValues.add(toRqlAst(innerValue, remainingDepth - 1));
            }
            return new MakeArray(innerValues, null);
        }

        if (val instanceof Map) {
            Map<String, RqlAst> obj = new HashMap<String, RqlAst>();
            for (Map.Entry<Object, Object> entry : (Set<Map.Entry>) ((Map) val).entrySet()) {
                if (!(entry.getKey() instanceof String)) {
                    throw new RethinkDBException("Object key can only be strings");
                }

                obj.put((String) entry.getKey(), toRqlAst(entry.getValue()));
            }
            return new MakeObj(obj);
        }

        if (val instanceof RqlLambda) {
            return new Func((RqlLambda) val);
        }

        if (val instanceof Date) {
            TimeZone tz = TimeZone.getTimeZone("UTC");
            DateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mmZ");
            df.setTimeZone(tz);
            return ISO8601(df.format((Date) val));
        }

        return new Datum(val);
    }


    // /*
    //     Called on arguments that should be functions
    //  */
    // public static RqlAst funcWrap(java.lang.Object o) {
    //     final RqlAst rqlQuery = toRqlAst(o);

    //     if (hasImplicitVar(rqlQuery)) {
    //         return new Func(new RqlFunction() {
    //             @Override
    //             public RqlAst apply(RqlAst row) {
    //                 return rqlQuery;
    //             }
    //         });
    //     } else {
    //         return rqlQuery;
    //     }
    // }


    // public static boolean hasImplicitVar(RqlAst node) {
    //     if (node.getTermType() == Q2L.Term.TermType.IMPLICIT_VAR) {
    //         return true;
    //     }
    //     for (RqlAst arg : node.getArgs()) {
    //         if (hasImplicitVar(arg)) {
    //             return true;
    //         }
    //     }
    //     for (Map.Entry<String, RqlAst> kv : node.getOptionalArgs().entrySet()) {
    //         if (hasImplicitVar(kv.getValue())) {
    //             return true;
    //         }
    //     }

    //     return false;
    // }

    // public static Q2L.Datum createDatum(java.lang.Object value) {
    //     Q2L.Datum.Builder builder = Q2L.Datum.newBuilder();

    //     if (value == null) {
    //         return builder
    //                 .setType(Q2L.Datum.DatumType.R_NULL)
    //                 .build();
    //     }

    //     if (value instanceof String) {
    //         return builder
    //                 .setType(Q2L.Datum.DatumType.R_STR)
    //                 .setRStr((String) value)
    //                 .build();
    //     }

    //     if (value instanceof Number) {
    //         return builder
    //                 .setType(Q2L.Datum.DatumType.R_NUM)
    //                 .setRNum(((Number) value).doubleValue())
    //                 .build();
    //     }

    //     if (value instanceof Boolean) {
    //         return builder
    //                 .setType(Q2L.Datum.DatumType.R_BOOL)
    //                 .setRBool((Boolean) value)
    //                 .build();
    //     }

    //     if (value instanceof Collection) {
    //         Q2L.Datum.Builder arr = builder
    //                 .setType(Q2L.Datum.DatumType.R_ARRAY);

    //         for (java.lang.Object o : (Collection) value) {
    //             arr.addRArray(createDatum(o));
    //         }

    //         return arr.build();

    //     }

    //     throw new RethinkDBException("Unknown Value can't create datatype for : " + value.getClass());
    // }

}
