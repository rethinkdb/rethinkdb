package com.rethinkdb.model;

import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.ast.Util;
import org.json.simple.JSONObject;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

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

    public static OptArgs fromMap(Map<String, ReqlAst> map) {
        OptArgs oa = new OptArgs();
        oa.putAll(map);
        return oa;
    }

    public static OptArgs of(String key, Object val) {
        OptArgs oa = new OptArgs();
        oa.with(key, val);
        return oa;
    }

}
