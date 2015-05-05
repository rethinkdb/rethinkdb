package com.rethinkdb.ast.helper;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class Arguments extends ArrayList<Object> {

    public Arguments() {}
    public Arguments(Object arg1) {
        super();
        add(arg1);
    }

    public Arguments(Object[] args) {
        addAll(Arrays.asList(args));
    }

    public Arguments(List<Object> args) {
        addAll(args);
    }
}
