var Stream = require('stream');
var json = typeof JSON === 'object' ? JSON : require('jsonify');

module.exports = Render;

function Render () {
    Stream.call(this);
    this.readable = true;
    this.count = 0;
    this.fail = 0;
    this.pass = 0;
}

Render.prototype = new Stream;

Render.prototype.pipe = function () {
    this.piped = true;
    return Stream.prototype.pipe.apply(this, arguments);
};

Render.prototype.begin = function () {
    this.emit('data', 'TAP version 13\n');
};

Render.prototype.push = function (t) {
    var self = this;
    this.emit('data', '# ' + t.name + '\n');
    
    t.on('result', function (res) {
        if (typeof res === 'string') {
            self.emit('data', '# ' + res + '\n');
            return;
        }

        self.emit('data', encodeResult(res, self.count + 1));
        self.count ++;
        
        if (res.ok) self.pass ++
        else self.fail ++
    });
};

Render.prototype.close = function () {
    this.emit('data', '\n1..' + this.count + '\n');
    this.emit('data', '# tests ' + this.count + '\n');
    this.emit('data', '# pass  ' + this.pass + '\n');
    if (this.fail) {
        this.emit('data', '# fail  ' + this.fail + '\n');
    }
    else {
        this.emit('data', '\n# ok\n');
    }
    
    this.emit('end');
};

function encodeResult (res, count) {
    var output = '';
    output += (res.ok ? 'ok ' : 'not ok ') + count;
    output += res.name ? ' ' + res.name.replace(/\s+/g, ' ') : '';
    
    if (res.skip) output += ' # SKIP';
    else if (res.todo) output += ' # TODO';
    
    output += '\n';
    
    if (!res.ok) {
        var outer = '  ';
        var inner = outer + '  ';
        output += outer + '---\n';
        output += inner + 'operator: ' + res.operator + '\n';
        
        var ex = json.stringify(res.expected) || '';
        var ac = json.stringify(res.actual) || '';
        
        if (Math.max(ex.length, ac.length) > 65) {
            output += inner + 'expected:\n' + inner + '  ' + ex + '\n';
            output += inner + 'actual:\n' + inner + '  ' + ac + '\n';
        }
        else {
            output += inner + 'expected: ' + ex + '\n';
            output += inner + 'actual:   ' + ac + '\n';
        }
        if (res.at) {
            output += inner + 'at: ' + res.at + '\n';
        }
        if (res.operator === 'error' && res.actual && res.actual.stack) {
            var lines = String(res.actual.stack).split('\n');
            output += inner + 'stack:\n';
            output += inner + '  ' + lines[0] + '\n';
            for (var i = 1; i < lines.length; i++) {
                output += inner + lines[i] + '\n';
            }
        }
        
        output += outer + '...\n';
    }
    
    return output;
}
