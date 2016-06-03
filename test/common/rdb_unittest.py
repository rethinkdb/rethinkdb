#!/usr/bin/env python
# Copyright 2015-2016 RethinkDB, all rights reserved.

import itertools, os, random, re, shutil, sys, unittest, warnings

import driver, utils

def main():
    runner = unittest.TextTestRunner(verbosity=2)
    unittest.main(argv=[sys.argv[0]], testRunner=runner)

class RdbTestCase(unittest.TestCase):
    
    # -- settings
    
    servers = None # defaults to shards * replicas
    shards = 1
    replicas = 1
    
    server_command_prefix = None
    server_extra_options = None
    
    fieldName = 'id'
    recordsToGenerate = 0
    generateRecords = None # a method on some subclasses
    
    samplesPerShard = 5 # when making changes the number of changes to make per shard
    
    destructiveTest = False # if true the cluster should be restarted after this test
    
    # -- class variables
    
    dbName = None
    tableName = None
    
    db = None
    table = None
    
    cluster = None
    _conn = None
    
    r = utils.import_python_driver()
    
    # -- unittest subclass variables 
    
    __currentResult = None
    __problemCount = None
    
    # --
    
    def run(self, result=None):
        
        if not all([self.dbName, self.tableName]):
            defaultDb, defaultTable = utils.get_test_db_table()
            
            if self.dbName is None:
                self.__class__.dbName = defaultDb
            if self.tableName is None:
                self.__class__.tableName = defaultTable
            elif self.tableName is False:
                self.__class__.tableName = None
        
        self.__class__.db = self.r.db(self.dbName)
        self.__class__.table = self.db.table(self.tableName)
        
        # Allow detecting test failure in tearDown
        self.__currentResult = result or self.defaultTestResult()
        self.__problemCount = 0 if result is None else len(self.__currentResult.errors) + len(self.__currentResult.failures)
        
        super(RdbTestCase, self).run(self.__currentResult)
    
    @property
    def conn(self):
        '''Retrieve a valid connection to some server in the cluster'''
        
        # -- check if we already have a good cached connection
        if self.__class__._conn and self.__class__._conn.is_open():
            try:
                self.r.expr(1).run(self.__class__._conn)
                return self.__class__._conn
            except Exception: pass
        if self.__class__.conn is not None:
            try:
                self.__class__._conn.close()
            except Exception: pass
            self.__class__._conn = None
        
        # -- try a new connection to each server in order
        for server in self.cluster:
            if not server.ready:
                continue
            try:
                self.__class__._conn = self.r.connect(host=server.host, port=server.driver_port)
                return self.__class__._conn
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
                self.__class__.cluster = None
                self.__class__._conn = None
                self.__class__.table = None
        
        # - ensure we have a cluster
        
        if self.cluster is None:
            self.__class__.cluster = driver.Cluster()
        
        # - make sure we have any named servers
        
        if hasattr(self.servers, '__iter__'):
            for name in self.servers:
                firstServer = len(self.cluster) == 0
                if not name in self.cluster:
                    driver.Process(cluster=self.cluster, name=name, console_output=True, command_prefix=self.server_command_prefix, extra_options=self.server_extra_options, wait_until_ready=firstServer)
        
        # - ensure we have the proper number of servers
        # note: we start up enough servers to make sure they each have only one role
        print(self.servers)
        serverCount = max(self.shards * self.replicas, len(self.servers) if hasattr(self.servers, '__iter__') else self.servers or 0)
        for _ in range(serverCount - len(self.cluster)):
            firstServer = len(self.cluster) == 0
            driver.Process(cluster=self.cluster, console_output=True, command_prefix=self.server_command_prefix, extra_options=self.server_extra_options, wait_until_ready=firstServer)
        
        self.cluster.wait_until_ready()
        
        # -- ensure db is available
        
        if self.dbName is not None and self.dbName not in self.r.db_list().run(self.conn):
            self.r.db_create(self.dbName).run(self.conn)
        
        # -- setup test table
        
        if self.tableName is not None:
            
            # - ensure we have a clean table
            
            if self.tableName in self.r.db(self.dbName).table_list().run(self.conn):
                self.r.db(self.dbName).table_drop(self.tableName).run(self.conn)
            self.r.db(self.dbName).table_create(self.tableName).run(self.conn)
            
            self.__class__.table = self.r.db(self.dbName).table(self.tableName)
            
            # - add initial records
            
            if hasattr(self.generateRecords, '__call__'):
                self.generateRecords()
            elif self.recordsToGenerate:
                utils.populateTable(conn=self.conn, table=self.table, records=self.recordsToGenerate, fieldName=self.fieldName)
            
            # - shard and replicate the table
            
            primaries = iter(self.cluster[:self.shards])
            replicas = iter(self.cluster[self.shards:])
            
            shardPlan = []
            for primary in primaries:
                chosenReplicas = [replicas.next().name for _ in range(0, self.replicas - 1)]
                shardPlan.append({'primary_replica':primary.name, 'replicas':[primary.name] + chosenReplicas})
            assert (self.r.db(self.dbName).table(self.tableName).config().update({'shards':shardPlan}).run(self.conn))['errors'] == 0
            self.r.db(self.dbName).table(self.tableName).wait().run(self.conn)
        
        # -- run setUpClass if not run otherwise
        
        if not hasattr(unittest.TestCase, 'setUpClass') and hasattr(self.__class__, 'setUpClass') and not hasattr(self.__class__, self.__class__.__name__ + '_setup'):
            self.setUpClass()
            setattr(self.__class__, self.__class__.__name__ + '_setup', True)
    
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
            
            self.__class__.cluster = None
            self.__class__._conn = None
            self.__class__.table = None
            if lastError:
                raise lastError
        
        if self.destructiveTest:
            try:
                self.cluster.check_and_stop()
            except Exception: pass
            self.__class__.cluster = None
            self.__class__._conn = None
            self.__class__.table = None
    
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
        for lower, upper in utils.getShardRanges(connections.next(), tableName):
            
            conn = connections.next()
            sampleIds = (x['id'] for x in self.r.db(dbName).table(tableName).between(lower, upper).sample(samplesPerShard).run(conn))
            
            for thisId in sampleIds:
                self.r.db(dbName).table(tableName).get(thisId).update({'randomChange':random.randint(0, 65536)}).run(conn)
                changedRecordIds.append(thisId)
        
        changedRecordIds.sort()
        return changedRecordIds

# ==== class fixups

if not hasattr(RdbTestCase, 'assertRaisesRegex'):
    # -- patch the Python2.6 version of unittest to have assertRaisesRegex
    def assertRaisesRegex_replacement(self, exception, regexp, function, *args, **kwds):
        result = None
        try:
            result = function(*args, **kwds)
        except Exception as e:
            if not isinstance(e, exception):
                raise AssertionError('Got the wrong type of exception: %s vs. expected: %s' % (e.__class__.__name__, exception.__name__))
            if not re.match(regexp, str(e)):
                raise AssertionError('Error message: "%s" does not match "%s"' % (str(regexp), str(e)))
            return
        else:
            raise AssertionError('%s not raised for: %s, rather got: %s' % (exception.__name__, repr(function), repr(result)))
    RdbTestCase.assertRaisesRegex = assertRaisesRegex_replacement
