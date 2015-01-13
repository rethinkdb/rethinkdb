#!/usr/bin/env python
# Copyright 2015 RethinkDB, all rights reserved.

import itertools, os, random, shutil, unittest, warnings

import driver, utils

class RdbTestCase(unittest.TestCase):
    
    # -- settings
    
    shards = 1
    replicas = 1
    recordsToGenerate = 100
    
    samplesPerShard = 5 # when making changes the number of changes to make per shard
    
    destructiveTest = False # if true the cluster will be disposed of
    
    # -- class variables
    
    dbName = None
    tableName = None
    
    table = None
    
    cluster = None
    conn = None
    
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
        
        # Allow detecting test failure in tearDown
        self.__currentResult = result or self.defaultTestResult()
        self.__problemCount = 0 if result is None else len(self.__currentResult.errors) + len(self.__currentResult.failures)
        
        super(RdbTestCase, self).run(self.__currentResult)
    
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
    
    def getReplicaForShard(self, index, tableName=None, dbName=None):
        if tableName is None:
            tableName = self.tableName
        if dbName is None:
            dbName = self.dbName
        
        shardsData = self.r.db(dbName).table(tableName).config()['shards'].nth(index).run(self.conn)
        replicaNames = [x for x in shardsData['replicas'] if x != shardsData['primary_replica']]
        
        for server in self.cluster:
            if server.name in replicaNames:
                return server
        return None
    
    def setUp(self):
        
        # -- start the servers
        
        # - check on an existing cluster
        
        if self.cluster is not None:
            try:
                self.cluster.check()
            except:
                self.__class__.cluster = None
                self.__class__.conn = None
                self.__class__.table = None
        
        # - start new servers if necessary
        
        # note: we start up enough servers to make sure they only have only one role
        
        if self.cluster is None:
            self.__class__.cluster = driver.Cluster(initial_servers=self.shards * self.replicas)
        if self.conn is None:
            server = self.cluster[0]
            self.__class__.conn = self.r.connect(host=server.host, port=server.driver_port)
        
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
            
            if self.recordsToGenerate:
                self.populateTable()
            
            # - shard and replicate the table
            
            primaries = iter(self.cluster[:self.shards])
            replicas = iter(self.cluster[self.shards:])
            
            shardPlan = []
            for primary in primaries:
                chosenReplicas = [replicas.next().name for _ in range(0, self.replicas - 1)]
                shardPlan.append({'primary_replica':primary.name, 'replicas':[primary.name] + chosenReplicas})
            assert (self.r.db(self.dbName).table(self.tableName).config().update({'shards':shardPlan}).run(self.conn))['errors'] == 0
            self.r.db(self.dbName).table(self.tableName).wait().run(self.conn)
    
    def populateTable(self, recordsToGenerate=None, tableName=None, dbName=None):
        if recordsToGenerate is None:
            recordsToGenerate = self.recordsToGenerate
        if tableName is None:
            tableName = self.tableName
        if dbName is None:
            dbName = self.dbName
        
        self.r.db(dbName).table(tableName).insert(self.r.range(1, recordsToGenerate + 1).map({'id':self.r.row})).run(self.conn)
    
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
            
            # -- get a list of server folders
            
            serverFolders = [server.files.db_path for server in self.cluster]
            
            # -- stop all of the servers
            
            try:
                self.cluster.check_and_stop()
            except Exception: pass
            
            # -- save the server data
                
            try:
                # - create enclosing dir
                
                outputFolder = os.path.realpath(os.path.join(os.getcwd(), self.id().lstrip('__main__.')))
                if not os.path.isdir(outputFolder):
                    os.makedirs(outputFolder)
                
                # - copy the servers data
                
                for serverFolder in serverFolders:
                    shutil.copytree(serverFolder, os.path.join(outputFolder, os.path.basename(serverFolder)))
            
            except Exception as e:
                warnings.warn('Unable to copy server folder into results: %s' % str(e))
            
            self.__class__.cluster = None
            self.__class__.conn = None
            self.__class__.table = None
            if lastError:
                raise lastError
        
        if self.destructiveTest:
            try:
                self.cluster.check_and_stop()
            except Exception: pass
            self.__class__.cluster = None
            self.__class__.conn = None
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
