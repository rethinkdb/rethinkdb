require.onError = function (err) {
    var isDefineError = err.toString().indexOf('#defineerror') !== -1,
        //IE 6 seems to have an odd error object, but we still
        //get called correctly, so it is OK.
        expected = typeof navigator !== 'undefined' && navigator.userAgent.indexOf('MSIE 6.0') !== -1 ? false : true;

    doh.register(
        "defineError",
        [
            function defineError(t) {
                t.is(expected, isDefineError);
            }
        ]
    );

    doh.run();
};

require({
        baseUrl: require.isBrowser ? './' : './defineError',
        catchError: {
            define: true
        }
    },
    ["main"],
    function(main) {
        doh.register(
            "defineError2",
            [
                function defineError2(t){
                    t.is(undefined, main && main.errorName);
                }
            ]
        );

        doh.run();
    }
);
