package com.rethinkdb.model;


import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.ast.Util;

import java.util.*;
import java.util.stream.Collectors;

public class Arguments extends ArrayList<ReqlAst> {

    public Arguments() {}

    public Arguments(Object arg){
        if(arg instanceof List){
            coerceAndAddAll((List) arg);
        } else {
            coerceAndAdd(arg);
        }
    }
    public Arguments(Arguments args) {
        addAll(args);
    }
    public Arguments(ReqlAst arg) {
        add(arg);
    }

    public Arguments(Object[] args) {
        coerceAndAddAll(args);
    }

    public Arguments(List<Object> args) {
        addAll(Collections.singletonList(args).stream()
                .map(Util::toReqlAst)
                .collect(Collectors.toList()));
    }

    public static Arguments make(Object... args){
        return new Arguments(args);
    }

    public void coerceAndAdd(Object obj) {
        add(Util.toReqlAst(obj));
    }

    public void coerceAndAddAll(Object[] args) {
        coerceAndAddAll(Arrays.asList(args));
    }

    public void coerceAndAddAll(List<Object> args){
        addAll(args.stream()
                .map(Util::toReqlAst)
                .collect(Collectors.toList()));
    }
}
