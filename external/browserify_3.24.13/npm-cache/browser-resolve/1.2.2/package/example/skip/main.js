var tar = require('tar');

exports.add = function (a, b) {
    return a + b;
};

exports.parse = function () {
    return tar.Parse();
};
