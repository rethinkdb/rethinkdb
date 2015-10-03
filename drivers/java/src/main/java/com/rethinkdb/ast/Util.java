package com.rethinkdb.ast;

import com.rethinkdb.gen.ast.*;
import com.rethinkdb.gen.exc.ReqlDriverError;
import com.rethinkdb.gen.proto.TermType;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.ReqlLambda;


import java.lang.*;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.Date;
import java.util.Map;


public class Util {
    private Util(){}
    /**
     * Coerces objects from their native type to ReqlAst
     *
     * @param val val
     * @return ReqlAst
     */
    public static ReqlAst toReqlAst(Object val) {
        return toReqlAst(val, 1000);
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
        if (val instanceof ReqlAst) {
            return (ReqlAst) val;
        }

        if (val instanceof Object[]){
            Arguments innerValues = new Arguments();
            for (Object innerValue : Arrays.asList((Object[])val)){
                innerValues.add(toReqlAst(innerValue, remainingDepth -1));
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
            DateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ");
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
        if (val == null){
            return new Datum(null);
        }

        throw new ReqlDriverError("Can't convert %s to a ReqlAst", val);
    }

}
