-- fundamental problem noted by slava:

-- too complicated. too hard to optimize. too "programmy" instead of
-- "relational".

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

data Term
    = Var Var
    | Let [(Var, Term)] Term
    -- NB. can have terms under the builtin. think carefully about how to handle
    -- this in code that deals with ASTs.
    | Call Builtin [Term]
    | If Term Term Term
    -- `try e1 (v. e2)`
    -- runs and returns e1;
    -- if it fails, binds v to the error message and runs & returns e2.
    | Try Term (Var, Term)
    | Error String              -- raises an error with a given message

    -- Literals
    | Number JSONNumber
    | String String
    | Bool Bool
    | Null
    | Array [Term]
    | Map [(AttrName, Term)]

    -- no "And", "Or" needed; can be desugared to "If".
    | ViewAsStream View         -- turns a view into a stream

    -- `GetByKey tref attr expr`
    -- is porcelain for:
    --     try (nth 0 (filter tref (\x. x.attr == expr))) null
    | GetByKey TableRef AttrName Term

data Builtin
    = Not                       -- logical negation

    -- Map/record operations
    | GetAttr AttrName
    | HasAttr AttrName
    | PickAttrs [AttrName]      -- technically porcelain

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
    | Map Mapping               -- mapping function has type (json -> json)
    | ConcatMap Mapping         -- mapping function has type (json -> stream)
    | OrderBy OrderDirection Mapping
    | Distinct Mapping
    | Limit Int
    | Length                    -- counts # of elements in a stream
    | Union                     -- merges streams in undefined order
    | Nth                       -- nth element from stream

    | StreamToArray
    | ArrayToStream

    | Reduce Reduction
    | GroupedMapReduce Mapping Mapping Reduction -- TODO: document

    -- Arbitrary javascript function.
    -- js funcs need to be type-annotated
    -- TODO: figure out precise semantics
    | Javascript String
    -- Tim wants this, ask him what it should do
    | JavascriptReturningStream String

    -- "Porcelain"
    | MapReduce Mapping Reduction

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
    -- TODO: pick closed/open-ness convention
    -- ie. does (range tref "a" 0 3) include or exclude rows where a = 0, 3?
    | RangeView View AttrName (Maybe Term, Maybe Term)

data OrderDirection = Ascending | Descending

-- "magically" uses the query's type to determine whether to stream results?
data ReadQuery = ReadQuery Term

-- TODO: how are we doing deletes?

-- tim proposes returning "null" from a mapping to indicate "delete this row"
-- for an update. GetByKey can also return "null" to indicate "no row found".

data WriteQuery
    = Update View Mapping
    | Delete View 
    | Mutate View Mapping
    -- TODO: how does insert work if we insert a row with a primary key that
    -- already exists in the table?
    | Insert TableRef [Term]
    | InsertStream TableRef Stream
    | ForEach Stream Var [WriteQuery]
    -- `PointUpdate tref attr key updater`
    -- updates THE row in tref with attr equal to key, using updater
    -- NB. very restricted: attrname MUST be primary key
    | PointUpdate TableRef AttrName Term Mapping
    | PointDelete TableRef AttrName Term
    | PointMutate TableRef AttrName Term Mapping
