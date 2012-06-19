module RethinkDB where

-- A Symbol is essentially an identifier, a way to refer to other
-- entities (tables, attributes, etc.) in the AST.
type Symbol = String

-- These are Aliases to symbols and are meant to clarify which
-- entities various parts of the AST refer to.
type TableName = Symbol   -- Reference to a table
type AttrName = Symbol    -- Reference to a JSON attribute
type DBName = Symbol      -- Reference to a database
type Var = Symbol         -- Reference to a variable/argument

-- A way to refer to tables in the AST. A user can be explicit
-- (i.e. specify database name where table belongs) or implicit
-- (i.e. just specify the table name, in which case the table will be
-- selected from the ones in the scope, and an error will be thrown if
-- the name is ambigious).
data TableRef = TableRef (Maybe DBName) TableName
                deriving Show

data Type = -- TODO

data Term
    = Var Var
    | Let [(Var, Term)] Term
    | Call Builtin [Term]
    | If Term Term Term

    -- Literals
    | Number JSONNumber
    | String String
    | Bool Bool
    | Null
    | Array [Term]
    | Map [(AttrName, Term)]

    -- no "And", "Or" needed; can be desugared to "If".
    | ViewAsStream View         -- turns a view into a stream

data Builtin
    = Not                       -- logical negation

    -- Map/record operations
    | GetAttr AttrName
    | HasAttr AttrName

    -- merges two maps, preferring the key-value pairs from its second argument
    -- in case of conflict. used eg. to do field update.
    | MapMerge

    -- Array operations
    | ArrayCons | ArrayConcat | ArraySlice | ArrayNth | ArrayLength

    -- Arithmetic & comparison operations
    | Add | Subtract | Multiply | Divide | Modulo
    | Compare Comparison
    -- don't need unary negation "-x", desugars to "0 - x".

    -- Stream operations
    | Filter Predicate
    | Map Mapping
    | OrderBy OrderDirection Mapping
    | Distinct Mapping
    | Limit Int
    | Count                     -- counts # of elements in a stream
    | Union                     -- merges streams in undefined order

    | StreamToArray
    | ArrayToStream

    | Reduce Reduction
    | GroupBy Mapping Reduction -- TODO: document

    -- Arbitrary javascript function.
    -- js funcs need to be type-annotated
    -- TODO: figure out precise semantics
    | Javascript ([Type], Type) String

data Comparison = EQ | NE | LT | LE | GT | GE

-- TODO: possibly reductions, mappings, predicates should be *either* lambdas,
-- as they are now, or Javascript functions. (We could just desugar the latter
-- to the former, though...)
data Reduction
    = Reduction {
        reductionBase :: Term,
        reductionArg1 :: Var,
        reductionArg2 :: Var,
        reductionBody :: Term
        }

data Mapping
    = Mapping {
        mappingArg :: Var,
        mappingBody :: Term
        }

data Predicate
    = Predicate {
        predicateArg :: Var,
        predicateBody :: Term
        }

data View
    = Table TableRef
    | FilterView View Predicate

data OrderDirection = Ascending | Descending

-- "magically" uses the query's type to determine whether to stream results?
data ReadQuery = ReadQuery Term

data WriteQuery
    = Update View (Var, Term)
    | Insert TableRef [Term]
    | InsertStream TableRef Stream -- or should this just be a ForEach?
    | ForEach Stream Var [WriteQuery]
