package com.rethinkdb.net;


import com.rethinkdb.net.Response;

public class DBResultFactory {

    private DBResultFactory() {
    }

    public static <T> T convert(Response response) {
        throw new RuntimeException("convert not implemented");

        // switch (response.getType()) {
        //     case SUCCESS_ATOM:
        //         return DBResponseMapper.fromDatumObject(response.getResponse(0));
        //     case SUCCESS_PARTIAL:
        //     case SUCCESS_SEQUENCE:
        //         return (T) DBResponseMapper.fromDatumObjectList(response.getResponseList());
        //     case WAIT_COMPLETE:
        //         throw new UnsupportedOperationException();
        //     case CLIENT_ERROR:
        //     case COMPILE_ERROR:
        //     case RUNTIME_ERROR:
        //         throw new ReqlError(response.getType() + ": " + response.getResponse(0).getRStr());

        //     default:
        //         throw new ReqlError("Unknown Response Type: " + response.getType());
        // }
    }
}
