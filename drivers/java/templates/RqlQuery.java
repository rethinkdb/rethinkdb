package com.rethinkdb.ast.query;

import com.google.common.collect.Lists;
import com.rethinkdb.Cursor;
import com.rethinkdb.RethinkDBConnection;
import com.rethinkdb.ast.helper.Arguments;
import com.rethinkdb.ast.helper.OptionalArguments;
import com.rethinkdb.ast.query.gen.Date;
import com.rethinkdb.model.Durability;
import com.rethinkdb.model.RqlFunction;
import com.rethinkdb.ast.query.gen.*;
import com.rethinkdb.model.RqlFunction2;
import com.rethinkdb.proto.Q2L;
import com.rethinkdb.response.GroupedResponseConverter;
import sun.security.krb5.internal.crypto.Des;

import java.security.Key;
import java.util.*;

public class RqlQuery {
% for term, data in meta.iteritems():
    public ${camel(term)} ${dromedary(data.get('r_alias', term)}(Object... fields) {
        return new ${camel(term)}(this, new Arguments(fields), new Optargs());
    }

% endfor
}
