#!/usr/bin/env python
# Copyright 2015-2016 RethinkDB, all rights reserved.

import itertools, os, random, re, shutil, sys, traceback, unittest, warnings

try:
    int
except NameError:
    long = int

import driver, utils

def main():
    runner = unittest.TextTestRunner(verbosity=2)
    unittest.main(argv=[sys.argv[0]], testRunner=runner)

class TestCaseCompatible(unittest.TestCase):
    '''Replace missing bits from various versions of Python'''
    
    def __init__(self, *args, **kwargs):
        super(TestCaseCompatible, self).__init__(*args, **kwargs)
        if not hasattr(self, 'assertIsNone'):
            self.assertIsNone = self.replacement_assertIsNone
        if not hasattr(self, 'assertIsNotNone'):
            self.assertIsNotNone = self.replacement_assertIsNotNone
        if not hasattr(self, 'assertGreater'):
            self.assertGreater = self.replacement_assertGreater
        if not hasattr(self, 'assertGreaterEqual'):
            self.assertGreaterEqual = self.replacement_assertGreaterEqual
        if not hasattr(self, 'assertLess'):
            self.assertLess = self.replacement_assertLess
        if not hasattr(self, 'assertLessEqual'):
            self.assertLessEqual = self.replacement_assertLessEqual
        if not hasattr(self, 'assertIn'):
            self.assertIn = self.replacement_assertIn
        if not hasattr(self, 'assertRaisesRegexp'):
            self.assertRaisesRegex = self.replacement_assertRaisesRegexp
        
        if not hasattr(self, 'skipTest'):
            self.skipTest = self.replacement_skipTest
        
    def replacement_assertIsNone(self, val):
        if val is not None:
            raise AssertionError('%s is not None' % val)
    
    def replacement_assertIsNotNone(self, val):
        if val is None:
            raise AssertionError('%s is None' % val)
    
    def replacement_assertGreater(self, actual, expected):
        if not actual > expected:
            raise AssertionError('%s not greater than %s' % (actual, expected))
    
    def replacement_assertGreaterEqual(self, actual, expected):
        if not actual >= expected:
            raise AssertionError('%s not greater than or equal to %s' % (actual, expected))
    
    def replacement_assertLess(self, actual, expected):
        if not actual < expected:
            raise AssertionError('%s not less than %s' % (actual, expected))
    
    def replacement_assertLessEqual(self, actual, expected):
        if not actual <= expected:
            raise AssertionError('%s not less than or equal to %s' % (actual, expected))
    
    def replacement_assertIsNotNone(self, val):
        if val is None:
            raise AssertionError('Result is None')
    
    def replacement_assertIn(self, val, iterable):
        if not val in iterable:
            raise AssertionError('%s is not in %s' % (val, iterable))
    
    def replacement_assertRaisesRegexp(self, exception, regexp, callable_func, *args, **kwds):
        try:
            callable_func(*args, **kwds)
        except Exception as e:
            self.assertTrue(isinstance(e, exception), '%s expected to raise %s but instead raised %s: %s\n%s' % (repr(callable_func), repr(exception), e.__class__.__name__, str(e), traceback.format_exc()))
            self.assertTrue(re.search(regexp, str(e)), '%s did not raise the expected message "%s", but rather: %s' % (repr(callable_func), str(regexp), str(e)))
        else:
            self.fail('%s failed to raise a %s' % (repr(callable_func), repr(exception)))
    
    def replacement_skipTest(self, message):
        sys.stderr.write("%s " % message)

class RdbTestCase(TestCaseCompatible):
    
    # -- settings
    
    servers  = None # defaults to shards * replicas
    shards   = 1
    replicas = 1
    tables   = 1 # either a number, a name, or a list of names
    
    use_tls               = False
    server_command_prefix = None
    server_extra_options  = None
    
    cleanTables       = True # set to False if the nothing will be modified in the table
    destructiveTest   = False # if true the cluster should be restarted after this test
    
    fieldName         = 'id'
    recordsToGenerate = 0
    samplesPerShard   = 5 # when making changes the number of changes to make per shard
    
    # -- class variables
    
    dbName            = None # typically 'test'
    tableName         = None # name of the first table
    tableNames        = None
    
    __cluster         = None
    __conn            = None
    __db              = None # r.db(dbName)
    __table           = None # r.db(dbName).table(tableName)
    
    r = utils.import_python_driver()
    
    # -- unittest subclass variables 
    
    __currentResult   = None
    __problemCount    = None
    
    # --
    
    def run(self, result=None):
        if self.tables:
            defaultDb, defaultTable = utils.get_test_db_table()
            
            if not self.dbName:
                self.dbName = defaultDb
            
            if isinstance(self.tables, int):
                if self.tables == 1:
                    self.tableNames = [defaultTable]
                else:
                    self.tableNames = ['%s_%d' % (defaultTable, i) for i in range(1, self.tables + 1)]
            elif isinstance(self.tables, str):
                self.tableNames = [self.tables]
            elif hasattr(self.tables, '__iter__'):
                self.tableNames = [str(x) for x in self.tables]
            else:
                raise Exception('The value of tables was not recogised: %r' % self.tables)
            
            self.tableName = self.tableNames[0]
        
        # Allow detecting test failure in tearDown
        self.__currentResult = result or self.defaultTestResult()
        self.__problemCount = 0 if result is None else len(self.__currentResult.errors) + len(self.__currentResult.failures)
        
        super(RdbTestCase, self).run(self.__currentResult)
    
    @property
    def cluster(self):
        return self.__cluster
    
    @property
    def db(self):
        if self.__db is None and self.dbName:
            return self.r.db(self.dbName)
        return self.__db
    
    @property
    def table(self):
        if self.__table is None and self.tableName and self.db:
            self.__class__.__table = self.db.table(self.tableName)
        return self.__table
    
    @property
    def conn(self):
        '''Retrieve a valid connection to some server in the cluster'''
        
        return self.conn_function()
    
    def conn_function(self, alwaysNew=False):
        '''Retrieve a valid connection to some server in the cluster'''
        
        # -- check if we already have a good cached connection
        if not alwaysNew:
            if self.__class__.__conn and self.__class__.__conn.is_open():
                try:
                    self.r.expr(1).run(self.__class__.__conn)
                    return self.__class__.__conn
                except Exception: pass
            if self.__class__.conn is not None:
                try:
                    self.__class__.__conn.close()
                except Exception: pass
                self.__class__.__conn = None
        
        # -- try a new connection to each server in order
        for server in self.cluster:
            if not server.ready:
                continue
            try:
                ssl  = {'ca_certs':self.Cluster.tlsCertPath} if self.use_tls else None
                conn = self.r.connect(host=server.host, port=server.driver_port, ssl=ssl)
                if not alwaysNew:
                    self.__class__.__conn = conn
                return conn
            except Exception as e: pass
        else:        
            # fail as we have run out of servers
            raise Exception('Unable to get a connection to any server in the cluster')
    
    def getPrimaryForShard(self, index, tableName=None, dbName=None):
        if tableName is None:
            tableName = self.tableName
        if dbName is None:
            dbName = self.dbName
        
        serverName = self.r.db(dbName).table(tableName).config()['shards'].nth(index)['primary_replica'].run(self.conn)
        for server in self.cluster:
            if server.name == serverName:
                return server
        return None
    
    def getReplicasForShard(self, index, tableName=None, dbName=None):
    
        if tableName is None:
            tableName = self.tableName
        if dbName is None:
            dbName = self.dbName
        
        shardsData = self.r.db(dbName).table(tableName).config()['shards'].nth(index).run(self.conn)
        replicaNames = [x for x in shardsData['replicas'] if x != shardsData['primary_replica']]
        
        replicas = []
        for server in self.cluster:
            if server.name in replicaNames:
                replicas.append(server)
        return replicas
    
    def getReplicaForShard(self, index, tableName=None, dbName=None):
        replicas = self.getReplicasForShard(index, tableName=None, dbName=None)
        if replicas:
            return replicas[0]
        else:
            return None
    
    def checkCluster(self):
        '''Check that all the servers are running and the cluster is in good shape. Errors on problems'''
        
        assert self.cluster is not None, 'The cluster was None'
        self.cluster.check()
        res = list(self.r.db('rethinkdb').table('current_issues').filter(self.r.row["type"] != "memory_error").run(self.conn))
        assert res == [], 'There were unexpected issues: \n%s' % utils.RePrint.pformat(res)
    
    def setUp(self):
        
        # -- start the servers
        
        # - check on an existing cluster
        
        if self.cluster is not None:
            try:
                self.checkCluster()
            except:
                try:
                    self.cluster.check_and_stop()
                except Exception: pass
                self.__class__.__cluster = None
                self.__class__.__conn    = None
                self.__class__.__table   = None
        
        # - ensure we have a cluster
        
        if self.cluster is None:
            self.__class__.__cluster = driver.Cluster(tls=self.use_tls)
        
        # - make sure we have any named servers
        
        if hasattr(self.servers, '__iter__'):
            for name in self.servers:
                firstServer = len(self.cluster) == 0
                if not name in self.cluster:
                    driver.Process(cluster=self.cluster, name=name, console_output=True, command_prefix=self.server_command_prefix, extra_options=self.server_extra_options, wait_until_ready=firstServer)
        
        # - ensure we have the proper number of servers
        # note: we start up enough servers to make sure they each have only one role
        serverCount = max(self.shards * self.replicas, len(self.servers) if hasattr(self.servers, '__iter__') else self.servers or 0)
        for _ in range(serverCount - len(self.cluster)):
            firstServer = len(self.cluster) == 0
            driver.Process(cluster=self.cluster, console_output=True, command_prefix=self.server_command_prefix, extra_options=self.server_extra_options, wait_until_ready=firstServer)
        
        self.cluster.wait_until_ready()
        
        # -- ensure db is available
        
        self.r.expr([self.dbName]).set_difference(self.r.db_list()).for_each(self.r.db_create(self.r.row)).run(self.conn)
        
        # -- setup test tables
        
        # - drop all tables unless cleanTables is set to False
        if self.cleanTables:
            self.r.db('rethinkdb').table('table_config').filter({'db':self.dbName}).delete().run(self.conn)
        
        for tableName in (x for x in (self.tableNames or []) if x not in self.r.db(self.dbName).table_list().run(self.conn)):
            # - create the table
            self.r.db(self.dbName).table_create(tableName).run(self.conn)
            table = self.db.table(tableName)
            
            # - add initial records
            self.populateTable(conn=self.conn, table=table, records=self.recordsToGenerate, fieldName=self.fieldName)
            
            # - shard and replicate the table
            primaries = iter(self.cluster[:self.shards])
            replicas = iter(self.cluster[self.shards:])
            
            shardPlan = []
            for primary in primaries:
                chosenReplicas = [next(replicas).name for _ in range(0, self.replicas - 1)]
                shardPlan.append({'primary_replica':primary.name, 'replicas':[primary.name] + chosenReplicas})
            assert (table.config().update({'shards':shardPlan}).run(self.conn))['errors'] == 0
            table.wait().run(self.conn)
        
        # -- run setUpClass if not run otherwise
        
        if not hasattr(unittest.TestCase, 'setUpClass') and hasattr(self.__class__, 'setUpClass') and not hasattr(self.__class__, self.__class__.__name__ + '_setup'):
            self.setUpClass()
            setattr(self.__class__, self.__class__.__name__ + '_setup', True)
    
    def populateTable(self, conn=None, table=None, records=None, fieldName=None):
        if conn is None:
            conn = self.conn
        if table is None:
            table = self.table
        if records is None:
            records = self.recordsToGenerate
        
        utils.populateTable(conn=conn, table=table, records=records, fieldName=fieldName)
    
    def tearDown(self):
        
        # -- verify that the servers are still running
        
        lastError = None
        for server in self.cluster:
            if server.running is False:
                continue
            try:
                server.check()
            except Exception as e:
                lastError = e
        
        # -- check that there were not problems in this test
        
        allGood = self.__problemCount == len(self.__currentResult.errors) + len(self.__currentResult.failures)
        
        if lastError is not None or not allGood:
            
            # -- stop all of the servers
            
            try:
                self.cluster.check_and_stop()
            except Exception: pass
            
            # -- save the server data
                
            try:
                # - create enclosing dir
                
                name = self.id()
                if name.startswith('__main__.'):
                    name = name[len('__main__.'):]
                outputFolder = os.path.realpath(os.path.join(os.getcwd(), name))
                if not os.path.isdir(outputFolder):
                    os.makedirs(outputFolder)
                
                # - copy the servers data
                
                for server in self.cluster:
                    shutil.copytree(server.data_path, os.path.join(outputFolder, os.path.basename(server.data_path)))
            
            except Exception as e:
                warnings.warn('Unable to copy server folder into results: %s' % str(e))
            
            self.__class__.__cluster = None
            self.__class__.__conn    = None
            if lastError:
                raise lastError
        
        elif self.destructiveTest:
            try:
                self.cluster.check_and_stop()
            except Exception: pass
            self.__class__.__cluster = None
            self.__class__.__conn    = None
    
    def makeChanges(self, tableName=None, dbName=None, samplesPerShard=None, connections=None):
        '''make a minor change to records, and return those ids'''
        
        if tableName is None:
            tableName = self.tableName
        if dbName is None:
            dbName = self.dbName
        
        if samplesPerShard is None:
            samplesPerShard = self.samplesPerShard
        
        if connections is None:
            connections = itertools.cycle([self.conn])
        else:
            connections = itertools.cycle(connections)
        
        changedRecordIds = []
        for lower, upper in utils.getShardRanges(next(connections), tableName):
            
            conn = next(connections)
            sampleIds = (x['id'] for x in self.r.db(dbName).table(tableName).between(lower, upper).sample(samplesPerShard).run(conn))
            
            for thisId in sampleIds:
                self.r.db(dbName).table(tableName).get(thisId).update({'randomChange':random.randint(0, 65536)}).run(conn)
                changedRecordIds.append(thisId)
        
        changedRecordIds.sort()
        return changedRecordIds
