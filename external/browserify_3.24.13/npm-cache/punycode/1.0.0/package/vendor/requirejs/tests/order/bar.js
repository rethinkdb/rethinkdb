var bar = 0;
define(function () {
    bar += 1;
    return function () {
        bar += 2;
    }
});
