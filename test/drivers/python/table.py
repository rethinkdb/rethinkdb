import rethinkdb as r
import unittest

class TestTable(unittest.TestCase):
    # Shared db ref, ast
    def setUp(self):
        self.db = r.db_name
        self.ast = ''
    
    # Grabbing a table reference (the following tests are equivalent)
    def test_table_ref_attr(self):
        t = self.db.table_name
        assertTrue(protoEq(t, ast))
        
    def test_table_ref_dict(self):
        t = self.db['table_name']
        assertTrue(protoEq(t, ast))
        
    def test_table_ref_direct(self):
        t = r.table(self.db, 'table_name')
        assertTrue(protoEq(t, ast))
        

