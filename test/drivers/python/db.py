import rethinkdb as r
import unittest

class TestDB(unittest.TestCase):
    # Shared ast
    def setUp(self):
        self.ast = ''
        
    # Grabbing a db reference (the following tests are equivalent)
    def test_db_ref_attr(self):
        db = r.db_name
        assertTrue(protoEq(db, ast))
        
    def test_db_ref_dict(self):
        db = r['db_name']
        assertTrue(protoEq(db, ast))
        
    def test_db_ref_direct(self):
        db = r.db('db_name')
        assertTrue(protoEq(db, ast))

