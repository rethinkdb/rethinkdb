package com.rethinkdb.ast.helper;

import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.ast.Util;

import java.util.HashMap;
import java.util.List;

public class OptArgs extends HashMap<String, ReqlAst> {
    public OptArgs with(String key, Object value) {
        if (key != null) {
            put(key, Util.toReqlAst(value));
        }
        return this;
    }

    public OptArgs with(String key, List<Object> value) {
        if (key != null) {
            put(key, Util.toReqlAst(value));
        }
        return this;
    }

    public static OptArgs fromMap(java.util.Map<String, ReqlAst> map) {
        OptArgs oa = new OptArgs();
        oa.putAll(map);
        return oa;
    }

}
