package com.rethinkdb.response;

import com.rethinkdb.proto.DatumType;

public class Datum {
    private DatumType datumType;

    public Datum(){
        throw new RuntimeException("constructor for datum not written");
    }

    public DatumType getType() {
        return datumType;
    }

    % for datum_name in proto['Datum']['DatumType'].keys():
    public boolean is${camel(datum_name[2:])}() {
        return datumType == DatumType.${datum_name};
    }

    % endfor

}
