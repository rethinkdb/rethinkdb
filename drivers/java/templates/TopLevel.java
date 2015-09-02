<%page args="all_terms" />
package com.rethinkdb.gen.model;

import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.gen.ast.Error;
import com.rethinkdb.gen.ast.*;
import com.rethinkdb.ast.Util;
import com.rethinkdb.gen.exc.ReqlDriverError;

public class TopLevel {

    public ReqlExpr expr(Object value){
        return Util.toReqlExpr(value);
    }

    public ReqlExpr row(Object... values) {
        throw new ReqlDriverError("r.row is not implemented in the Java driver."+
                                  " Use lambda syntax instead");
    }

%for term in all_terms.values():
  %if "TopLevel" in term["include_in"]:
    %for signature in term['signatures']:
    public ${term['classname']} ${term['methodname']}(${
        ', '.join('%s %s' % (arg['type'], arg['var'])
                  for arg in signature['args'])}){
        % if term['methodname'] == 'binary':
        <% firstarg = signature['args'][0]['var'] %>
        if(${firstarg} instanceof byte[]){
            return new ${term['classname']}((byte[]) ${firstarg});
        }else{
        %endif
        Arguments args = new Arguments();
        %for arg in signature['args']:
          %if arg['type'] == 'Object...':
        args.coerceAndAddAll(${arg['var']});
          %else:
        args.coerceAndAdd(${arg['var']});
          %endif
        %endfor
        return new ${term['classname']}(args);
        % if term['methodname'] == 'binary':
        }
        %endif
    }
    %endfor
  %endif
%endfor
}
