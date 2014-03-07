define('Carousel', function () {
    return function Carousel(service) {
        this.service = service;
        this.someType = 'Carousel';
    };
});

define('app', ['Carousel'], function () {

});
