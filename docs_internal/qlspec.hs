
module RethinkDB where

-- JSON attributes and values
data Attr = Attr (Maybe TableQuery) String | NestedAttr Attr Attr
            deriving Show
data Value = DBInt Int | DBString String | DBMap [(Attr, Value)]
             deriving Show

-- When we refer to data (say, to compare it, or to do operations on
-- it), we can mean the data in the db (attributes), or date we ran
-- some operations on, or inline data (values). The Term type
-- generalizes this concept.
data DBOp = OpAdd Term Term | OpSub Term Term -- obviously we can extend this with more ops
            deriving Show
data Term = TAttr Attr | TValue Value | TComputed DBOp | TScalar ScalarQuery
            deriving Show
          
-- Selectors for filtering
data Selector = Eq Term Term | Lt Term Term | Lte Term Term | Gt Term Term | Gte Term Term
              | Exists Attr | IsTrue Attr
              | Not Selector | Or Selector Selector | And Selector Selector
              | In Term ReadQuery
             deriving Show

-- List of possible queries that can be performed
data TableQuery = Table String                        -- Return full table data
                  deriving Show

data FilterQuery = Filter SubsetQuery Selector        -- Filter some data
                   deriving Show

data SubsetQuery = TQ TableQuery | FQ FilterQuery
                   deriving Show

-- You cannot run modification operations on top of extend queries
data ExQuery = Pluck ReadQuery [Attr]                      -- Take out only relevant attributes
             | OrderBy ReadQuery [Attr]                    -- Order by an attribute (in case of equal rows, order by next one, etc.)
             | Distinct ReadQuery [Attr]                   -- Only return distinct rows
             | GroupBy ReadQuery [Attr] [(Attr, ReduceFn)] -- Group by a set of attributes and reduce the rest
             | ImmutableFilter ReadQuery Selector          -- Equivalent to filter, but return data can't be modified
             | Union ReadQuery ReadQuery                   -- Set operations...
               deriving Show

data ReadQuery = SQuery SubsetQuery | EQuery ExQuery
                 deriving Show

-- Scalar query returns only a single document or a single value
data ScalarQuery = Nth ReadQuery Int    -- return nth row
                 | Count ReadQuery      -- return number of rows
                   deriving Show

-- Whoo, let's let people change stuff
data ModQuery = Update SubsetQuery [(Attr, Term)]        -- Update every attribute in a row
              | Delete SubsetQuery                       -- Delete every document in the subset query
              | Insert TableQuery [[(Attr, Value)]]      -- Insert documents into a table
                deriving Show

-- Predefined reduce functions (example, there are more, obviously)
newtype ReduceFn = ReduceFn (Value -> Value -> Value)
instance Show ReduceFn where
    showsPrec _ _ = showString "reduceFn"

dbSum :: Value -> Value -> Value
dbSum (DBInt acc) (DBInt val) = DBInt (acc + val)

-- Open questions
-- * Joins
-- * Args
-- * Scope
-- * Handling arrays and what can be done over them
-- * Comparing with non-scalar queries
-- * Table/DB/Arg references

