package com.rethinkdb.net;

import com.rethinkdb.*;
import com.rethinkdb.ReqlError;
import com.rethinkdb.proto.ErrorType;
import com.rethinkdb.proto.ResponseType;
import com.rethinkdb.proto.ResponseNote;
import com.rethinkdb.ast.Query;
import com.rethinkdb.response.Backtrace;
import com.rethinkdb.response.Profile;
import org.json.simple.*;

import java.util.*;
import java.nio.ByteBuffer;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.io.IOException;
import java.util.stream.Collectors;


class Response {
    public final long token;
    public final ResponseType type;
    public final ArrayList<ResponseNote> notes;

    public final Optional<JSONArray> data;
    public final Optional<Profile> profile;
    public final Optional<Backtrace> backtrace;
    public final Optional<ErrorType> errorType;


    private static class ByteBufferInputStream extends InputStream {
        ByteBuffer buf;
        ByteBufferInputStream(ByteBuffer buf) {
            this.buf = buf;
        }

        public synchronized int read() throws IOException {
            if(!buf.hasRemaining()){
                return -1;
            }
            return buf.get();
        }

        public synchronized int read(byte[] bytes, int off, int len) throws IOException {
            len = Math.min(len, buf.remaining());
            buf.get(bytes, off, len);
            return len;
        }
    }

    public static Response parseFrom(long token, ByteBuffer buf) {
        InputStreamReader codepointReader =
            new InputStreamReader(new ByteBufferInputStream(buf));
        JSONObject jsonResp = (JSONObject) JSONValue.parse(codepointReader);
        ResponseType responseType = ResponseType.fromValue((int)jsonResp.get("t"));
        ArrayList<Integer> responseNoteVals = (ArrayList<Integer>) jsonResp
            .getOrDefault("n", new ArrayList());
        ArrayList<ResponseNote> responseNotes = responseNoteVals
            .stream()
            .map(ResponseNote::fromValue)
            .collect(Collectors.toCollection(ArrayList::new));
        ErrorType et = (ErrorType) jsonResp.getOrDefault("e", null);
        ResponseBuilder res = new ResponseBuilder(token, responseType);
        if(jsonResp.containsKey("e")){
            res.setErrorType((int) jsonResp.get("e"));
        }
        return res.setProfile((JSONObject) jsonResp.getOrDefault("p", null))
                .setBacktrace((JSONObject) jsonResp.getOrDefault("b", null))
                .setData((JSONArray) jsonResp.getOrDefault("r", null))
                .build();
    }

    private Response(long token,
             ResponseType responseType,
             ArrayList<ResponseNote> responseNotes,
             Optional<Profile> profile,
             Optional<Backtrace> backtrace,
             Optional<JSONArray> data,
             Optional<ErrorType> errorType
    ) {
        this.token = token;
        this.type = responseType;
        this.notes = responseNotes;
        this.profile = profile;
        this.backtrace = backtrace;
        this.data = data;
        this.errorType = errorType;
    }

    static class ResponseBuilder {
        long token;
        ResponseType responseType;
        ArrayList<ResponseNote> notes = new ArrayList<>();

        Optional<JSONArray> data;
        Optional<Profile> profile;
        Optional<Backtrace> backtrace;
        Optional<ErrorType> errorType;

        ResponseBuilder(long token, ResponseType responseType){
            this.token = token;
            this.responseType = responseType;
        }

        ResponseBuilder setNotes(ArrayList<ResponseNote> notes){
            this.notes.addAll(notes);
            return this;
        }

        ResponseBuilder setData(JSONArray data){
            this.data = Optional.ofNullable(data);
            return this;
        }

        ResponseBuilder setProfile(JSONObject profile) {
            this.profile = Optional.of(Profile.fromJSONObject(profile));
            return this;
        }

        ResponseBuilder setBacktrace(JSONObject backtrace) {
            this.backtrace = Optional.of(
                    Backtrace.fromJSONObject(backtrace));
            return this;
        }

        ResponseBuilder setErrorType(int value) {
            this.errorType = Optional.of(ErrorType.fromValue(value));
            return this;
        }

        Response build() {
            return new Response(
                    token,
                    responseType,
                    notes,
                    profile,
                    backtrace,
                    data,
                    errorType
            );
        }
    }

    public boolean isWaitComplete() {
        return type == ResponseType.WAIT_COMPLETE;
    }

    /* Whether the response is any kind of feed */
    public boolean isFeed() {
        return notes.stream().allMatch(ResponseNote::isFeed);
    }

    /* Whether the response is any kind of error */
    public boolean isError() {
        return type.isError();
    }

    public static JSONObject convertPseudotypes(
            JSONObject obj, Optional<Profile> profile) {
        throw new RuntimeException("convertPseudotypes not implemented");
    }

    /* What type of success the response contains */
    public boolean isAtom() {
        return type == ResponseType.SUCCESS_ATOM;
    }

    public boolean isSequence() {
        return type == ResponseType.SUCCESS_SEQUENCE;
    }

    public boolean isPartial() {
        return type == ResponseType.SUCCESS_PARTIAL;
    }

    ReqlError makeError(Query query) {
        return new ErrorBuilder(data.map(d -> (String) d.get(0))
                .orElse("Unknown error message"), type)
                .setBacktrace(backtrace)
                .setErrorType(errorType)
                .setTerm(query)
                .build();
    }
}
