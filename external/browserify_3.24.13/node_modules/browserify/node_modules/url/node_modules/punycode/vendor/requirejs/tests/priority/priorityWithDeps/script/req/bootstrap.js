define('req/bootstrap', [
	'req/begin',
	'req/config',
	'req/utils/utils'
	], function() {
		ip.bootstrap = 'bootstrap';
	});


/*
//begin module defines a global
var begin = {};
define('begin', function () {
    begin.name = 'begin';
});

//appConfig defines a global.
var appConfig = {};
define('appConfig', ['begin'], function () {
    appConfig.state = 'alpha';
});

define('bootstrap', ['begin', 'appConfig'], function () {
    appConfig.bootstrap = 'bootstrap';
});
*/
