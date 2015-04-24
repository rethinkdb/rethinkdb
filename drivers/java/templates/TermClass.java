package com.rethinkdb.ast.query.gen;

import com.rethinkdb.ast.query.RqlQuery;
import com.rethinkdb.proto.TermType;

import java.util.List;
import java.util.Map;


public class ${dromedary(term)} extends RqlQuery {
    public ${camel(term)}(RqlQuery prev, List<Object> args, Map<String, Object> kwargs){
        super(prev, TermType.${term}, args, kwargs);
    }
}
