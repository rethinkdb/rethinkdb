define(['error'], function (err) {
    return {
        name: 'main',
        errorName: err && err.name
    }
});
