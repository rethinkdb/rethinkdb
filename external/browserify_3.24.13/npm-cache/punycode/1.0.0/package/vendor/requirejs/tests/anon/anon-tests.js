require({
        baseUrl: require.isBrowser ? "./" : "anon/",
        paths: {
            text: "../../text",
            i18n: "../../i18n"
        }
    },
    ["require", "magenta", "red", "blue", "green", "yellow", "a", "c"],
    function(require, magenta, red, blue, green, yellow, a, c) {

        doh.register(
            "anonSimple",
            [
                function colors(t){
                    t.is("redblue", magenta.name);
                    t.is((require.isBrowser ? "./foo.html" : "anon/foo.html"), magenta.path);
                    t.is("red", red.name);
                    t.is("blue", blue.name);
                    t.is("green", green.name);
                    t.is("yellow", yellow.name);
                    t.is("a", a.name);
                    t.is("sub/b", a.bName);
                    t.is("c", c.name);
                    t.is("a", c.aName);

                    //Also try a require call after initial
                    //load that uses already loaded modules,
                    //to be sure the require callback is called.
                    if (require.isBrowser) {
                        setTimeout(function () {
                            require(["blue", "red", "magenta"], function (blue, red) {
                                doh.register(
                                    "anonSimpleCached",
                                    [
                                        function colorsCached(t){
                                            t.is("red", red.name);
                                            t.is("blue", blue.name);
                                            t.is("redblue", magenta.name);
                                            t.is("hello world", magenta.message);
                                       }
                                    ]
                                );
                                doh.run();
                            });
                        }, 300);
                    }
                }
            ]
        );
        doh.run();
    }
);
