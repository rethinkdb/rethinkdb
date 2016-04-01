package com.rethinkdb;

import com.rethinkdb.ast.Query;
import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.gen.proto.ErrorType;
import com.rethinkdb.gen.proto.ResponseType;
import com.rethinkdb.model.Backtrace;
import com.rethinkdb.gen.exc.*;

import java.util.Optional;
import java.util.function.Function;

public class ErrorBuilder {
    final String msg;
    final ResponseType responseType;
    Optional<Backtrace> backtrace = Optional.empty();
    Optional<ErrorType> errorType = Optional.empty();
    Optional<ReqlAst> term = Optional.empty();

    public ErrorBuilder(String msg, ResponseType responseType) {
        this.msg = msg;
        this.responseType = responseType;
    }

    public ErrorBuilder setBacktrace(Optional<Backtrace> backtrace) {
        this.backtrace = backtrace;
        return this;
    }

    public ErrorBuilder setErrorType(Optional<ErrorType> errorType) {
        this.errorType = errorType;
        return this;
    }

    public ErrorBuilder setTerm(Query query) {
        this.term = query.term;
        return this;
    }

    public ReqlError build() {
        assert (msg != null);
        assert (responseType != null);
        Function<String,ReqlError> con;
        switch (responseType) {
            case CLIENT_ERROR:
                con = ReqlClientError::new;
                break;
            case COMPILE_ERROR:
                con = ReqlServerCompileError::new;
                break;
            case RUNTIME_ERROR: {
                con = errorType.<Function<String,ReqlError>>map(et -> {
                    switch (et) {
                        case INTERNAL:
                            return ReqlInternalError::new;
                        case RESOURCE_LIMIT:
                            return ReqlResourceLimitError::new;
                        case QUERY_LOGIC:
                            return ReqlQueryLogicError::new;
                        case NON_EXISTENCE:
                            return ReqlNonExistenceError::new;
                        case OP_FAILED:
                            return ReqlOpFailedError::new;
                        case OP_INDETERMINATE:
                            return ReqlOpIndeterminateError::new;
                        case USER:
                            return ReqlUserError::new;
                        case PERMISSION_ERROR:
                            return ReqlPermissionError::new;
                        default:
                            return ReqlRuntimeError::new;
                    }
                }).orElse(ReqlRuntimeError::new);
                break;
            }
            default:
                con = ReqlError::new;
        }
        ReqlError res = con.apply(msg);
        backtrace.ifPresent(res::setBacktrace);
        term.ifPresent(res::setTerm);
        return res;
    }
}
