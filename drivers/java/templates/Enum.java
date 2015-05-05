package com.rethinkdb.proto;

public enum ${classname} {

    ${",\n    ".join("{k}({v})".format(k=key, v=val) for key, val in items)};

    public final int value;

    private ${classname}(int value){
        this.value = value;
    }
}
