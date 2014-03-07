define('req/utils/utils', [
	'req/begin',
	'req/config'
], function() {
	var utils = {
		test:  function() {
			return ('utils ' + ip.config.test);
		}
	};
	return ip.utils = utils;
});
