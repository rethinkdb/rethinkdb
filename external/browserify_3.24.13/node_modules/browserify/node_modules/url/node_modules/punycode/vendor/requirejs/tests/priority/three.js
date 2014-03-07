//Example layer file.

define("alpha",
    ["beta", "gamma"],
    function (beta, gamma) {
        return {
            name: "alpha",
            betaName: beta.name
        };
    }
);

define("beta",
    ["gamma"],
    function (gamma) {
        return {
            name: "beta",
            gammaName: gamma.name
        };
    }
);


//Example layer file.

define("gamma",
    ["theta", "epsilon"],
    function (theta, epsilon) {
        return {
            name: "gamma",
            thetaName: theta.name,
            epsilonName: epsilon.name
        };
    }
);

define("theta",
    function () {
        return {
            name: "theta"
        };
    }
);

define("epsilon",
    {
        name: "epsilon"
    }
);
