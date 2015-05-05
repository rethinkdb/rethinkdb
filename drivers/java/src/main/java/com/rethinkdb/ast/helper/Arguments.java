package com.rethinkdb.ast.helper;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class Arguments extends ArrayList<Object> {
    public Arguments(Object arg1) {
        super();
        add(arg1);
    }

    public Arguments(Object... args) {
        super();
        addAll(Arrays.asList(args));
    }

    public Arguments(List<Object> args) {
        super();
        addAll(args);
    }
}
