define('req/config', [
	'req/begin'
], function() {
	var config = {
		test: 'Hello require',
		state: 'alpha'
	};
	return ip.config = config;
});
