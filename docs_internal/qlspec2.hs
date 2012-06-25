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

data JSONTerm
    = Arg ArgName
    | With [(ArgName, JSONTerm)] [(ArgName, StreamTerm)] JSONTerm

    | BinaryOp BinaryOp JSONTerm JSONTerm
    | UnaryOp UnaryOp JSONTerm

    | If JSONTerm JSONTerm JSONTerm

    | Number JSONNumber
    | String String
    | Bool Bool
    | Null

    | EmptyArray
    | PushArray JSONTerm JSONTerm
    | ConcatArray JSONTerm JSONTerm
    | SliceArray JSONTerm JSONTerm JSONTerm
    | LengthArray JSONTerm
    | NthArray JSONTerm JSONTerm

    | EmptyMap
    | SetInMap AttrName JSONTerm JSONTerm
    | GetInMap AttrName JSONTerm
    | IsInMap AttrName JSONTerm

    | StreamAsArray StreamTerm
    | Fold StreamTerm Reduction
    | OrderedReduce StreamTerm Reduction
    | UnorderedReduce StreamTerm Reduction
    | GroupBy StreamTerm Mapping Reduction
    | Count StreamTerm

    | Javascript String [JSONTerm] [StreamTerm]

data BinaryOp
    = Add | Subtract | Multiply | Divide | Modulo
    | Equals | LessThan | GreaterThan | LessThanOrEquals | GreaterThanOrEquals | NotEquals
    | And | Or

data UnaryOp
    = Negate | Not

data Reduction 
    = Reduction {
        reductionBase :: JSONTerm,
        reductionArg1 :: ArgName,
        reductionArg2 :: ArgName,
        reductionBody :: JSONTerm
        }

data Mapping
    = Mapping {
        mappingArg :: ArgName,
        mappingBody :: JSONTerm
        }

data Predicate
    = Predicate {
        predicateArg :: ArgName,
        predicateBody :: JSONTerm
        }

data View
    = Table TableRef
    | Filter View ArgName JSONTerm

data OrderDirection = Ascending | Descending

data StreamTerm
    = Arg ArgName
    | With [(ArgName, JSONTerm)] [(ArgName, StreamTerm)]

    | ViewAsStream View

    | Filter StreamTerm Predicate
    | Map StreamTerm Mapping
    | Distinct StreamTerm Mapping
    | OrderBy StreamTerm OrderDirection Mapping
    | Limit Int StreamTerm
    | MergeSort [StreamTerm] Mapping

    | ArrayAsStream JSONTerm

    | Javascript String [JSONTerm] [StreamTerm]

data ReadQuery
    = JSONReadQuery JSONTerm
    | StreamReadQuery StreamTerm

data WriteQuery
    = Update View (ArgName, JSONTerm)
    | Insert TableRef JSONTerm
    | ForEach Stream ArgName [WriteQuery]
    | Javascript String [JSONTerm] [StreamTerm]
