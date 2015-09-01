package com.rethinkdb.ast;

import com.rethinkdb.gen.exc.ReqlDriverError;
import com.rethinkdb.gen.proto.TermType;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.gen.ast.Datum;
import com.rethinkdb.gen.ast.Func;
import com.rethinkdb.gen.ast.MakeArray;
import com.rethinkdb.gen.ast.MakeObj;
import com.rethinkdb.gen.ast.Iso8601;
import com.rethinkdb.model.ReqlLambda;


import java.lang.*;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.*;


public class Util {
    private Util(){}
    /**
     * Coerces objects from their native type to ReqlAst
     *
     * @param val val
     * @return ReqlAst
     */
    public static ReqlAst toReqlAst(java.lang.Object val) {
        return toReqlAst(val, 1000);
    }

    private static ReqlAst toReqlAst(java.lang.Object val, int remainingDepth) {
        if (val instanceof ReqlAst) {
            return (ReqlAst) val;
        }

        if (val instanceof List) {
            Arguments innerValues = new Arguments();
            for (java.lang.Object innerValue : (List) val) {
                innerValues.add(toReqlAst(innerValue, remainingDepth - 1));
            }
            return new MakeArray(innerValues, null);
        }

        if (val instanceof Map) {
            Map<String, ReqlAst> obj = new HashMap<>();
            for (Map.Entry<Object, Object> entry : (Set<Map.Entry>) ((Map) val).entrySet()) {
                if (!(entry.getKey() instanceof String)) {
                    throw new ReqlDriverError("Object key can only be strings");
                }

                obj.put((String) entry.getKey(), toReqlAst(entry.getValue()));
            }
            return MakeObj.fromMap(obj);
        }

        if (val instanceof ReqlLambda) {
            return Func.fromLambda((ReqlLambda) val);
        }

        if (val instanceof Date) {
            TimeZone tz = TimeZone.getTimeZone("UTC");
            DateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mmZ");
            df.setTimeZone(tz);
            return Iso8601.fromString(df.format((Date) val));
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

        throw new ReqlDriverError("Can't convert %s to a ReqlAst", val);
    }

    // /*
    //     Called on arguments that should be functions
    //  */
    // public static ReqlAst funcWrap(java.lang.Object o) {
    //     final ReqlAst ReqlQuery = toReqlAst(o);

    //     if (hasImplicitVar(ReqlQuery)) {
    //         return new Func(new ReqlFunction() {
    //             @Override
    //             public ReqlAst apply(ReqlAst row) {
    //                 return ReqlQuery;
    //             }
    //         });
    //     } else {
    //         return ReqlQuery;
    //     }
    // }


    // public static boolean hasImplicitVar(ReqlAst node) {
    //     if (node.getTermType() == Q2L.Term.TermType.IMPLICIT_VAR) {
    //         return true;
    //     }
    //     for (ReqlAst arg : node.getArgs()) {
    //         if (hasImplicitVar(arg)) {
    //             return true;
    //         }
    //     }
    //     for (Map.Entry<String, ReqlAst> kv : node.getOptionalArgs().entrySet()) {
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

    //     throw new ReqlError("Unknown Value can't create datatype for : " + value.getClass());
    // }

}
