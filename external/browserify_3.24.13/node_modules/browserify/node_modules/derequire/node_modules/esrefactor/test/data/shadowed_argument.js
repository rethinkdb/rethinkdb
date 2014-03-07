function test(alpha, beta, zeta) {
    var gamma = alpha + beta;

    // 'zeta' does not refer to the above function parameter
    var zeta = 42 / gamma;  // declaration:zeta reference:zeta

    return zeta; // cursor:zeta reference:zeta
}
