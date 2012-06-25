
module RethinkDB where

-- A Symbol is essentially an identifier, a way to refer to other
-- entities (tables, attributes, etc.) in the AST.
type Symbol = String

-- These are Aliases to symbols and are meant to clarify which
-- entities various parts of the AST refer to.
type TableName = Symbol   -- Reference to a table
type AttrName = Symbol    -- Reference to a JSON attribute
type ArgName = Symbol     -- Reference to an argument
type DBName = Symbol      -- Reference to a database

-- A way to refer to tables in the AST. A user can be explicit
-- (i.e. specify database name where table belongs) or implicit
-- (i.e. just specify the table name, in which case the table will be
-- selected from the ones in the scope, and an error will be thrown if
-- the name is ambigious).
data TableRef = TableRef (Maybe DBName) TableName
                deriving Show

-- A way to refer to JSON attributes in the AST.
data Attr = Attr (Maybe TableRef) AttrName   -- An attribute in a table (if implicit, the attribute will be searched for in the scope)
          | NestedAttr Attr AttrName         -- A nested attribute used to access inner hashes
            deriving Show

-- A way to define literal values in the AST
data Value = DBInt Int | DBString String    -- primitive types (we can expand this with dates, etc.)
           | DBArray [Value]                -- arrays
           | DBMap [(Attr, Value)]          -- hashes
             deriving Show

-- A way to express operations (e.g. attribute plus value, attribute
-- plus result of a query, etc.)
data DBOp = OpAdd Term Term | OpSub Term Term                          -- obviously we can extend this with more ops
          | OpPush Term Term | OpRemove Term Term                      -- array operations
            deriving Show

-- A term is an AST structure that allows referring to data
data Term = TAttr Attr               -- JSON attributes
          | TValue Value             -- Literal values
          | TArg (ArgName, Value)    -- Arguments passed by a user for query execution (used to avoid hardcoding literals in the AST)
          | TComputed DBOp           -- An expression over various entities
          | TScalar ScalarQuery      -- A scalar query (i.e. query that returns only one value)
          | ArrayFromQuery ReadQuery -- Converts a result of a query into a json array (e.g. so the result can be stored)
            deriving Show
          
-- Selectors compute to booleans and can be used 
data Selector = Eq Term Term | Lt Term Term | Lte Term Term | Gt Term Term | Gte Term Term -- Equality, order
              | IsTrue Attr                                                                -- Checking if a value is true
              | Exists Attr                                                                -- Existence of an attribute
              | Not Selector | Or Selector Selector | And Selector Selector                -- Logic operations
              | In Term ReadQuery                                                          -- Essentially a member predicate
                deriving Show

-- A subset query refers to a subset of a table. The subset can then
-- be returned, updated, or deleted.
data SubsetQuery = Table TableRef                -- All rows in a table
                 | Filter SubsetQuery Selector   -- Select a subset of a query (a subset of a table or of another subset)
                   deriving Show

-- You cannot run modification operations on top of extend queries
data ExQuery = Pluck ReadQuery [Attr]                       -- Take out only relevant attributes
             | OrderBy ReadQuery [Attr]                     -- Order by an attribute (in case of equal rows, order by next one, etc.)
             | Distinct ReadQuery [Attr]                    -- Only return distinct rows
             | GroupBy ReadQuery [Attr] [(Attr, ReduceFn)]  -- Group by a set of attributes and reduce the rest
             | ImmutableFilter ReadQuery Selector           -- Equivalent to filter, but return data can't be modified
             | Union ReadQuery ReadQuery                    -- Set/Bag operations...
             -- Operational (and declarative) way to do joins. For
             -- each row in the outer query we execute the inner
             -- query, and return the cross product. The inner query
             -- can refer to the row of the outer one. The optional
             -- symbols allow for renaming queries (so that the user
             -- can avoid potential name conflicts).
             | CrossEach ReadQuery (Maybe Symbol) ReadQuery (Maybe Symbol)
               deriving Show

-- This allows referring to all possible read queries in the AST,
-- other than the scalar query.
data ReadQuery = SQuery SubsetQuery       -- Reference to a subset query
               | EQuery ExQuery           -- Reference to an extended query
               | QueryFromArray Term      -- Convert an array to multiple rows so that it can be queried
                 deriving Show

-- Scalar query returns only a single document or a single value
data ScalarQuery = Nth ReadQuery Int    -- Return nth row
                 | Count ReadQuery      -- Return number of rows
                   deriving Show

-- The With node allows for specifying common subexpressions. Any
-- query within the With node can refer to the queries specified by
-- the node. These outer queries are only evaluated once.
data AnyReadQuery = RQAnyReadQuery ReadQuery | SQAnyReadQuery ScalarQuery
data AnyQuery = ARQ AnyReadQuery | AMQ ModQuery
data With = With [(Symbol, AnyReadQuery)] [AnyQuery]

-- Support for modification of data in the database
data ModQuery = Update SubsetQuery [(Attr, Term)]        -- Update every attribute in a row
              | Delete SubsetQuery                       -- Delete every document in the subset query
              | Insert TableRef [[(Attr, Value)]]        -- Insert
              -- documents into a table Modify data based on a
              -- query. For each row in the read query we perform each
              -- modification query (the modification queries have
              -- access to the row). The optional symbol can be used
              -- to rename the read query to avoid scope conflicts.
              | Each ReadQuery (Maybe Symbol) [ModQuery]
                deriving Show

-- Predefined reduce functions (example, there are more,
-- obviously). These can be used by the GroupBy operator.
newtype ReduceFn = ReduceFn (Value -> Value -> Value)
instance Show ReduceFn where
    showsPrec _ _ = showString "reduceFn"

dbSum :: Value -> Value -> Value
dbSum (DBInt acc) (DBInt val) = DBInt (acc + val)

-- A NOTE ON OPERATIONAL SEMANTICS

-- *Execution*

-- The language can be treated as declarative (i.e. specify which data
-- you want and let the system figure out how to do query execution),
-- and operational (i.e. if treated literally the user specifies to
-- the database exactly what to do). For the first version of
-- RethinkDB we will execute the AST depth first and as is, without
-- doing any optimizations (i.e. reordering, common subexpression
-- elimination, various join algorithms, etc). After 1.2. is out we
-- will start adding various optimizations. However, at any point the
-- user can tell the database to execute the query as is, in which
-- case no optimizations will be performed on the query.

-- *Indexing*

-- The one part of the language that does not specify operational
-- semantics is selection of data. The user cannot express in this
-- language which indexes to use. For 1.2. we will support a small set
-- of cases that optimize the selectors (e.g. point queries over a
-- primary key). We can also add porcelain commands that could be
-- compiled to this language, but are guaranteed to use certain
-- indexes (e.g. for point queries).

-- *Concurrency*

-- We will make an effort to provide atomicity on row level, however,
-- this is difficult to do in some cases, in which case we'll drop row
-- atomicity. We will not provide any cross-row atomicity or isolation
-- (though in practice, we'll end up using snapshots on a shard
-- level).

-- *Scope*

-- Some queries (such as ModQuery.Each and ExQuery.CrossEach) involve
-- having inner scopes. We will allow the user to refer to attributes
-- by name (in which case we'll search the scope from inside to
-- outside). We'll also allow users to refer to attributes by
-- qualifying them with table names (in which case we'll search the
-- scope inside to outside). We'll also allow the users to rename rows
-- to let them handle ambiguities manually.

