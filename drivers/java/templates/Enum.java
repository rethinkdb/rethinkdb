<%page args="package,classname,items,value_type='int'"/>
package com.rethinkdb.gen.${package};

import java.util.Optional;

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

    public static Optional<${classname}> maybeFromValue(${value_type} value) {
        try {
            return Optional.of(fromValue(value));
        } catch (IllegalArgumentException iae) {
            return Optional.empty();
        }
    }
<%block name="custom_methods"/>
}
