package com.rethinkdb.net;

import com.rethinkdb.*;
import com.rethinkdb.ast.*;
import com.rethinkdb.gen.exc.ReqlError;
import com.rethinkdb.gen.proto.ErrorType;
import com.rethinkdb.gen.proto.ResponseType;
import com.rethinkdb.gen.proto.ResponseNote;
import com.rethinkdb.model.Backtrace;
import com.rethinkdb.model.Profile;
import org.json.simple.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;
import java.nio.ByteBuffer;
import java.util.stream.Collectors;


class Response {
    public final long token;
    public final ResponseType type;
    public final ArrayList<ResponseNote> notes;

    public final JSONArray data;
    public final Optional<Profile> profile;
    public final Optional<Backtrace> backtrace;
    public final Optional<ErrorType> errorType;

    static final Logger logger = LoggerFactory.getLogger(Query.class);

    public static Response parseFrom(long token, ByteBuffer buf) {
        if (Response.logger.isDebugEnabled()) {
            Response.logger.debug(
                    "JSON Recv: Token: {} {}", token, Util.bufferToString(buf));
        }
        JSONObject jsonResp = Util.toJSON(buf);
        ResponseType responseType = ResponseType.fromValue(
                ((Long) jsonResp.get("t")).intValue()
        );
        ArrayList<Long> responseNoteVals = (ArrayList<Long>) jsonResp
            .getOrDefault("n", new ArrayList());
        ArrayList<ResponseNote> responseNotes = responseNoteVals
            .stream()
                .map(Long::intValue)
                .map(ResponseNote::maybeFromValue)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .collect(Collectors.toCollection(ArrayList::new));
        Builder res = new Builder(token, responseType);
        if(jsonResp.containsKey("e")){
            res.setErrorType(((Long)jsonResp.get("e")).intValue());
        }
        return res.setNotes(responseNotes)
                .setProfile((JSONArray) jsonResp.getOrDefault("p", null))
                .setBacktrace((JSONArray) jsonResp.getOrDefault("b", null))
                .setData((JSONArray) jsonResp.getOrDefault("r", new JSONArray()))
                .build();
    }

    private Response(long token,
                     ResponseType responseType,
                     JSONArray data,
                     ArrayList<ResponseNote> responseNotes,
                     Optional<Profile> profile,
                     Optional<Backtrace> backtrace,
                     Optional<ErrorType> errorType
    ) {
        this.token = token;
        this.type = responseType;
        this.data = data;
        this.notes = responseNotes;
        this.profile = profile;
        this.backtrace = backtrace;
        this.errorType = errorType;
    }

    static class Builder {
        long token;
        ResponseType responseType;
        ArrayList<ResponseNote> notes = new ArrayList<>();
        JSONArray data = new JSONArray();
        Optional<Profile> profile = Optional.empty();
        Optional<Backtrace> backtrace = Optional.empty();
        Optional<ErrorType> errorType = Optional.empty();

        Builder(long token, ResponseType responseType){
            this.token = token;
            this.responseType = responseType;
        }

        Builder setNotes(ArrayList<ResponseNote> notes){
            this.notes.addAll(notes);
            return this;
        }

        Builder setData(JSONArray data){
            if(data != null){
                this.data = data;
            }
            return this;
        }

        Builder setProfile(JSONArray profile) {
            this.profile = Profile.fromJSONArray(profile);
            return this;
        }

        Builder setBacktrace(JSONArray backtrace) {
            this.backtrace = Backtrace.fromJSONArray(backtrace);
            return this;
        }

        Builder setErrorType(int value) {
            this.errorType = Optional.of(ErrorType.fromValue(value));
            return this;
        }

        Response build() {
            return new Response(
                    token,
                    responseType,
                    data,
                    notes,
                    profile,
                    backtrace,
                    errorType
            );
        }
    }

    static Builder make(long token, ResponseType type){
        return new Builder(token, type);
    }

    boolean isWaitComplete() {
        return type == ResponseType.WAIT_COMPLETE;
    }

    /* Whether the response is any kind of feed */
    boolean isFeed() {
        return notes.stream().anyMatch(ResponseNote::isFeed);
    }

    /* Whether the response is any kind of error */
    boolean isError() {
        return type.isError();
    }

    /* What type of success the response contains */
    boolean isAtom() {
        return type == ResponseType.SUCCESS_ATOM;
    }

    boolean isSequence() {
        return type == ResponseType.SUCCESS_SEQUENCE;
    }

    boolean isPartial() {
        return type == ResponseType.SUCCESS_PARTIAL;
    }

    ReqlError makeError(Query query) {
        String msg = data.size() > 0 ?
                (String) data.get(0)
                : "Unknown error message";
        return new ErrorBuilder(msg, type)
                .setBacktrace(backtrace)
                .setErrorType(errorType)
                .setTerm(query)
                .build();
    }

    @Override
    public String toString() {
        return "Response{" +
                "token=" + token +
                ", type=" + type +
                ", notes=" + notes +
                ", data=" + data +
                ", profile=" + profile +
                ", backtrace=" + backtrace +
                '}';
    }
}
