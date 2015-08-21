package com.rethinkdb.${package};

public enum ${classname} {

    ${",\n    ".join("{k}({v})".format(k=key, v=val) for key, val in items)};

    public final int value;

    private ${classname}(int value){
        this.value = value;
    }

    public static ${classname} fromValue(int value) {
        switch (value) {
    % for key, val in items:
            case ${val}: return ${classname}.${key};
    % endfor
            default:
                throw new IllegalArgumentException(String.format(
                "%s is not a legal value for ${classname}", value));
        }
    }
<%block name="custom_methods"/>
}
