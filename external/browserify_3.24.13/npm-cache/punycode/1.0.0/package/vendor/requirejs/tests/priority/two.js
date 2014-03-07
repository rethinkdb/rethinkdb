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
