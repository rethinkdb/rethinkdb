import rethinkdb as r
import unittest

class TestDBRef(unittest.TestCase):
    # Shared ast
    def setUp(self):
        # The AST these tests should generate
        self.ast = 'db_name'
        
    # Grabbing a db reference (the following tests should produce
    # equivalent ASTs)
    def test_db_ref_attr(self):
        db = r.db_name
        assertEqual(db.ast == self.ast)
        
    def test_db_ref_dict(self):
        db = r['db_name']
        assertEqual(db.ast == self.ast)
        
    def test_db_ref_direct(self):
        db = r.db('db_name')
        assertEqual(db.ast == self.ast)

