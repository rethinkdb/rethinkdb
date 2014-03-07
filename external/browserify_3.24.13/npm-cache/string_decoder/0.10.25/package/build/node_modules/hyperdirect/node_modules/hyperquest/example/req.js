var hyperquest = require('../');
hyperquest('http://localhost:8000').pipe(process.stdout);
