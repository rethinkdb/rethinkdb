import rethinkdb as r
import unittest
import query_language_pb2 as p

class TestTableRef(unittest.TestCase):
    # Shared db ref, ast
    def setUp(self):
        # A database object
        self.db = r.db_name

        # The AST these tests should generate
        self.ast = p.TableRef()
        self.ast.db_name = self.db.ast
        self.ast.table_name = 'table_name'
    
    # Grabbing a table reference (the following tests should produce
    # equivalent ASTs)
    def test_table_ref_attr(self):
        t = self.db.table_name
        assertEqual(t.ast, self.ast)
        
    def test_table_ref_dict(self):
        t = self.db['table_name']
        assertEqual(t.ast, self.ast)
        
    def test_table_ref_direct(self):
        t = r.table(self.db, 'table_name')
        assertEqual(t.ast, self.ast)
        
