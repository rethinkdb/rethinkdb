require({
        baseUrl: require.isBrowser ? './' : './plugins/fromText',
        paths: {
            'text': '../../../text'
        }
},      ['refine!a'],
function (a) {

    doh.register(
        'pluginsFromText',
        [
            function pluginsFromText(t){
                t.is('a', a.name);
             }
        ]
    );
    doh.run();
});
