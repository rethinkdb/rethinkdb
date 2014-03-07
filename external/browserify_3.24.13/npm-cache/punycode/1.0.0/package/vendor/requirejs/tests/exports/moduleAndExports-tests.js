require({
        baseUrl: require.isBrowser ? "./" : "./exports/"
    },
    ['am'],
    function(am) {
        doh.register(
            "moduleAndExports",
            [
                function moduleAndExports(t){
                    t.is('am', am.name);
                    t.is('bm', am.bName);
                    t.is('cm', am.cName);
                }
            ]
        );
        doh.run();
    }
);
