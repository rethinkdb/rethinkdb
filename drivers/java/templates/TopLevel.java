<%page args="all_terms" />
package com.rethinkdb.gen.model;

import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.MapObject;
import com.rethinkdb.gen.ast.Error;
import com.rethinkdb.gen.ast.*;
import com.rethinkdb.ast.Util;
import com.rethinkdb.gen.exc.ReqlDriverError;

import java.util.Arrays;
import java.util.List;

public class TopLevel {

    public ReqlExpr expr(Object value){
        return Util.toReqlExpr(value);
    }

    public ReqlExpr row(Object... values) {
        throw new ReqlDriverError("r.row is not implemented in the Java driver."+
                                  " Use lambda syntax instead");
    }

    public MapObject hashMap(Object key, Object val){
        return new MapObject().with(key, val);
    }

    public MapObject hashMap() {
        return new MapObject();
    }

    public List array(Object... vals){
        return Arrays.asList(vals);
    }

%for term in all_terms.values():
  %if "TopLevel" in term["include_in"]:
    %for methodname in term['methodnames']:
      %for signature in term['signatures']:
    public ${term['classname']} ${methodname}(${
        ', '.join('%s %s' % (arg['type'], arg['var'])
                  for arg in signature['args'])}){
          % if methodname == 'binary':
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
          % if methodname == 'binary':
        }
          %endif
    }
      %endfor
    %endfor
  %endif
%endfor
}
