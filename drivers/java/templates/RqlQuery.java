package com.rethinkdb.ast;

import com.rethinkdb.Cursor;
import com.rethinkdb.ast.helper.Arguments;
import com.rethinkdb.ast.helper.OptionalArguments;
import com.rethinkdb.ast.query.gen.*;
import java.util.*;

public class RqlQuery extends RqlSerializable {
    /* Query level terms */
% for term, info in meta.iteritems():
    % if 'query' in info.get('include_in', ['query']):
    public ${camel(term)} ${info.get('alias', dromedary(term))}(Object... fields) {
        return new ${camel(term)}(this, new Arguments(fields), new Optargs());
    }

    % endif
% endfor
}
