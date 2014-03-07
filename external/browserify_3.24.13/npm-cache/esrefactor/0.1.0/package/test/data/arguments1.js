[1,4,2,3,-8].filter(function(i) {  // declaration:i
    var max = 3;

    function other(i) { i + 2; }

    return i < max   // cursor:i reference:i
});
