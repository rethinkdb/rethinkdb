package com.rethinkdb.ast.helper;

import com.rethinkdb.ast.RqlAst;
import com.rethinkdb.ast.Util;

import java.util.HashMap;
import java.util.List;

public class OptArgs extends HashMap<String, RqlAst> {

    public OptArgs() {}

    public OptArgs with(String key, Object value) {
        if (key != null) {
            put(key, Util.toRqlAst(value));
        }
        return this;
    }

    public OptArgs with(String key, List<Object> value) {
        if (key != null) {
            put(key, Util.toRqlAst(value));
        }
        return this;
    }

    public static OptArgs fromMap(java.util.Map<String, RqlAst> map) {
        OptArgs oa = new OptArgs();
        oa.putAll(map);
        return oa;
    }

}
