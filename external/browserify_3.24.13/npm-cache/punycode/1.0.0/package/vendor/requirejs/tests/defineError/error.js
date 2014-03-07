define(['require', 'main'], function (require, main) {
    return {
        name: 'error',
        bad: doesNotExist.bad(),
        doSomething: function () {
            require('main').doSomething();
        }
    };
});
