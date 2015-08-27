<%page args="package,classname,items,value_type='int'"/>
package com.rethinkdb.gen.${package};

public enum ${classname} {

    ${",\n    ".join("{k}({v})".format(k=key, v=val) for key, val in items)};

    public final ${value_type} value;

    private ${classname}(${value_type} value){
        this.value = value;
    }

    public static ${classname} fromValue(${value_type} value) {
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
