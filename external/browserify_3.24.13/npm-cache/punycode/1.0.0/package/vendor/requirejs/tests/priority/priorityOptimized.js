var master = new doh.Deferred(),
    count = 0;
doh.register(
    "priorityOptimized",
    [
        {
            name: "priorityOptimized",
            timeout: 5000,
            runTest: function () {
                return master;
            }
        }
    ]
);
doh.run();

//START simulated optimization of the "one" dependency, it should not be
//be requested as a separate script.
define("alphaPrime",
    function () {
        return {
            name: "alphaPrime"
        };
    }
);

define("betaPrime",
    ["alphaPrime"],
    function (alphaPrime) {
        return {
            name: "betaPrime",
            alphaPrimeName: alphaPrime.name
        };
    }
);

define("three", function () {});
//END simulated optimization.

require(
    {
        baseUrl: "./",
        priority: ["one", "two", "three"]
    },
    ["alpha", "beta", "gamma", "epsilon", "alphaPrime", "betaPrime"],
    function (alpha, beta, gamma, epsilon, alphaPrime, betaPrime) {
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
        doh.is("alphaPrime", alphaPrime.name);
        doh.is("alphaPrime", betaPrime.alphaPrimeName);
        doh.is("betaPrime", betaPrime.name);

        var regExp = /alpha|beta|gamma|theta|three|alphaPrime|betaPrime/,
            i,
            scripts = document.getElementsByTagName("script");
        for (i = scripts.length - 1; i > -1; i--) {
            doh.f(regExp.test(scripts[i].src));
        }
        master.callback(true);
    }
);
