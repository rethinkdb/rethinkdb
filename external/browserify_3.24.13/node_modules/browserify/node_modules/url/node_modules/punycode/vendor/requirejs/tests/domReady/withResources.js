/*jslint strict: false */
/*global requirejs: false, doh: false, document: false */

requirejs.config({
    paths: {
        domReady: '../../domReady'
    }
});

requirejs(['domReady'], function (domReady) {
    requirejs(['one'], function (one) {
        domReady(function () {
            one.addToDom();
        });
    });

    requirejs(['two'], function (two) {
        domReady(function () {
            two.addToDom();
        });
    });

    domReady.withResources(function () {
        doh.register(
            "domReadyWithResources",
            [
                function domReadyWithResources(t) {
                    t.is('one', document.getElementById('one').getAttribute('data-name'));
                    t.is('two', document.getElementById('two').getAttribute('data-name'));
                }
            ]
        );
        doh.run();
    });

});