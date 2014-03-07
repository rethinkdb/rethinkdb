var master = new doh.Deferred(),
    count = 0;
doh.register(
    "priority",
    [
        {
            name: "priority",
            timeout: 5000,
            runTest: function () {
                return master;
            }
        }
    ]
);
doh.run();

require(
    {
        baseUrl: "./",
        priority: ["one", "two"]
    },
    ["alpha", "beta", "gamma", "epsilon"],
    function (alpha, beta, gamma, epsilon) {
        count += 1;

        //Make sure callback is only called once.
        doh.is(1, count);

        doh.is("alpha", alpha.name);
        doh.is("beta", alpha.betaName);
        doh.is("beta", beta.name);
        doh.is("gamma", beta.gammaName);
        doh.is("gamma", gamma.name);
        doh.is("theta", gamma.thetaName);
        doh.is("epsilon", gamma.epsilonName);
        doh.is("epsilon", epsilon.name);

        var regExp = /alpha|beta|gamma|theta/,
            i,
            scripts = document.getElementsByTagName("script");
        for (i = scripts.length - 1; i > -1; i--) {
            doh.f(regExp.test(scripts[i].src));
        }
        master.callback(true);

    }
);
