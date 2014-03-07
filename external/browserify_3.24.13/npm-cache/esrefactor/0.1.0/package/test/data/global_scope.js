var test; // declaration:test
function f() {
    var s = test / 2; // reference:test
    return test * s;  // cursor:test reference:test
}
