({
    appDir: '.',
    baseUrl: '.',
    dir: 'built',
    optimize: 'none',

    packagePaths: {
        'packages': [
            'engine',
            'tires',
            'fuel'
        ]
    },

    modules: [
        { name: "main" },
        { name: "engine" },
        { name: "tires" },
        { name: "fuel" }
    ]

})