
module RethinkDB where

-- Identifier
type Symbol = String
type TableName = Symbol
type AttrName = Symbol
type ArgName = Symbol
type DBName = Symbol

-- JSON attributes and values
data TableRef = TableRef (Maybe DBName) (Maybe TableName)
                deriving Show
data Attr = Attr TableRef AttrName | NestedAttr Attr AttrName
            deriving Show
data Value = DBInt Int | DBString String | DBMap [(Attr, Value)]
             deriving Show

-- When we refer to data (say, to compare it, or to do operations on
-- it), we can mean the data in the db (attributes), or date we ran
-- some operations on, or inline data (values). The Term type
-- generalizes this concept.
data DBOp = OpAdd Term Term | OpSub Term Term                          -- obviously we can extend this with more ops
          | OpPush Term Term | OpRemove Term Term | OpUnion Term Term  -- array operations             
            deriving Show
data Term = TAttr Attr | TValue Value
          | TArg (ArgName, Value)                                      -- An argument passed to the query
          | TComputed DBOp | TScalar ScalarQuery
          | ArrayFromQuery ReadQuery                                   -- converts a result of a query into a json array
            deriving Show
          
-- Selectors for filtering
data Selector = Eq Term Term | Lt Term Term | Lte Term Term | Gt Term Term | Gte Term Term
              | Exists Attr | IsTrue Attr
              | Not Selector | Or Selector Selector | And Selector Selector
              | In Term ReadQuery
                deriving Show

-- List of possible queries that can be performed
data TableQuery = Table TableRef                      -- Return full table data
                  deriving Show

data FilterQuery = Filter SubsetQuery Selector        -- Filter some data
                   deriving Show

data SubsetQuery = TQ TableQuery | FQ FilterQuery
                   deriving Show

-- You cannot run modification operations on top of extend queries
data ExQuery = Pluck ReadQuery [Attr]                       -- Take out only relevant attributes
             | OrderBy ReadQuery [Attr]                     -- Order by an attribute (in case of equal rows, order by next one, etc.)
             | Distinct ReadQuery [Attr]                    -- Only return distinct rows
             | GroupBy ReadQuery [Attr] [(Attr, ReduceFn)]  -- Group by a set of attributes and reduce the rest
             | ImmutableFilter ReadQuery Selector           -- Equivalent to filter, but return data can't be modified
             | Union ReadQuery ReadQuery                    -- Set/BagXS operations...
             | CrossEach ReadQuery (Maybe Symbol) ReadQuery -- Operational (and declarative) way to do joins
               deriving Show

data ReadQuery = SQuery SubsetQuery | EQuery ExQuery | QueryFromArray Term
                 deriving Show

-- Scalar query returns only a single document or a single value
data ScalarQuery = Nth ReadQuery Int    -- return nth row
                 | Count ReadQuery      -- return number of rows
                   deriving Show

-- Common subexpression elimination
data AnyReadQuery = RQAnyReadQuery ReadQuery | SQAnyReadQuery ScalarQuery
data AnyQuery = ARQ AnyReadQuery | AMQ ModQuery
data With = With [(Symbol, AnyReadQuery)] [AnyQuery]

-- Whoo, let's let people change stuff
data ModQuery = Update SubsetQuery [(Attr, Term)]        -- Update every attribute in a row
              | Delete SubsetQuery                       -- Delete every document in the subset query
              | Insert TableQuery [[(Attr, Value)]]      -- Insert documents into a table
              | Each ReadQuery (Maybe Symbol) [ModQuery] -- Modify data based on a query
                deriving Show

-- Predefined reduce functions (example, there are more, obviously)
newtype ReduceFn = ReduceFn (Value -> Value -> Value)
instance Show ReduceFn where
    showsPrec _ _ = showString "reduceFn"

dbSum :: Value -> Value -> Value
dbSum (DBInt acc) (DBInt val) = DBInt (acc + val)




