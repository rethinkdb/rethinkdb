(function (tree) {

tree.Directive = function (name, value, index, currentFileInfo) {
    this.name = name;

    if (Array.isArray(value)) {
        this.rules = [new(tree.Ruleset)(null, value)];
        this.rules[0].allowImports = true;
    } else {
        this.value = value;
    }
    this.index = index;
    this.currentFileInfo = currentFileInfo;

};
tree.Directive.prototype = {
    type: "Directive",
    accept: function (visitor) {
        if (this.rules) {
            this.rules = visitor.visitArray(this.rules);
        }
        if (this.value) {
            this.value = visitor.visit(this.value);
        }
    },
    genCSS: function (env, output) {
        output.add(this.name, this.currentFileInfo, this.index);
        if (this.rules) {
            tree.outputRuleset(env, output, this.rules);
        } else {
            output.add(' ');
            this.value.genCSS(env, output);
            output.add(';');
        }
    },
    toCSS: tree.toCSS,
    eval: function (env) {
        var evaldDirective = this;
        if (this.rules) {
            env.frames.unshift(this);
            evaldDirective = new(tree.Directive)(this.name, null, this.index, this.currentFileInfo);
            evaldDirective.rules = [this.rules[0].eval(env)];
            evaldDirective.rules[0].root = true;
            env.frames.shift();
        }
        return evaldDirective;
    },
    variable: function (name) { return tree.Ruleset.prototype.variable.call(this.rules[0], name); },
    find: function () { return tree.Ruleset.prototype.find.apply(this.rules[0], arguments); },
    rulesets: function () { return tree.Ruleset.prototype.rulesets.apply(this.rules[0]); },
    markReferenced: function () {
        var i, rules;
        this.isReferenced = true;
        if (this.rules) {
            rules = this.rules[0].rules;
            for (i = 0; i < rules.length; i++) {
                if (rules[i].markReferenced) {
                    rules[i].markReferenced();
                }
            }
        }
    }
};

})(require('../tree'));
