(function (tree) {

tree.Rule = function (name, value, important, merge, index, currentFileInfo, inline) {
    this.name = name;
    this.value = (value instanceof tree.Value) ? value : new(tree.Value)([value]);
    this.important = important ? ' ' + important.trim() : '';
    this.merge = merge;
    this.index = index;
    this.currentFileInfo = currentFileInfo;
    this.inline = inline || false;
    this.variable = name.charAt && (name.charAt(0) === '@');
};

tree.Rule.prototype = {
    type: "Rule",
    accept: function (visitor) {
        this.value = visitor.visit(this.value);
    },
    genCSS: function (env, output) {
        output.add(this.name + (env.compress ? ':' : ': '), this.currentFileInfo, this.index);
        try {
            this.value.genCSS(env, output);
        }
        catch(e) {
            e.index = this.index;
            e.filename = this.currentFileInfo.filename;
            throw e;
        }
        output.add(this.important + ((this.inline || (env.lastRule && env.compress)) ? "" : ";"), this.currentFileInfo, this.index);
    },
    toCSS: tree.toCSS,
    eval: function (env) {
        var strictMathBypass = false, name = this.name;    
        if (typeof name !== "string") {
            // expand 'primitive' name directly to get
            // things faster (~10% for benchmark.less):
            name = (name.length === 1) 
                && (name[0] instanceof tree.Keyword)
                    ? name[0].value : evalName(env, name);
        }
        if (name === "font" && !env.strictMath) {
            strictMathBypass = true;
            env.strictMath = true;
        }
        try {
            return new(tree.Rule)(name,
                              this.value.eval(env),
                              this.important,
                              this.merge,
                              this.index, this.currentFileInfo, this.inline);
        }
        catch(e) {
            e.index = e.index || this.index;
            throw e;
        }
        finally {
            if (strictMathBypass) {
                env.strictMath = false;
            }
        }
    },
    makeImportant: function () {
        return new(tree.Rule)(this.name,
                              this.value,
                              "!important",
                              this.merge,
                              this.index, this.currentFileInfo, this.inline);
    }
};

function evalName(env, name) {
    var value = "", i, n = name.length,
        output = {add: function (s) {value += s;}};
    for (i = 0; i < n; i++) {
        name[i].eval(env).genCSS(env, output);
    }
    return value;
}

})(require('../tree'));
