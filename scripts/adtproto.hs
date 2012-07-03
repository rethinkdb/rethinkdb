module Main where

{- DOCUMENTATION

To compile:

    $ ghc --make adtproto

Takes input from stdin, writes to stdout. Input is approximately protobuf
syntax, with a few extensions and a few missing parts. Output is protobuf
syntax.

In particular:

1. Fields are "required" by default and are automatically given tags based on
   their order in the file. So instead of:

       required int foo = 1;
       optional string bar = 2;

   You write:

       int foo;
       optional string bar;

   These are arguably misfeatures.

2. Adds a "data" construct to define an algebraic data type / variant / tagged
   union. Inside of "data" you define "branches". Branches can have associated
   data, which can either be a message, another type, or nothing.

   Example:

       data Example {
           branch Branch1 {
               // A message body goes here.
               int foo;
               string blah;
               repeated Example children;
           };

           // this branch is just an int, no message type
           branch Branch2 = int;

           // this branch has no associated data
           branch Branch3;
       };

   This turns into the following protobuf code:

       message Example {
           enum ExampleType {
               BRANCH1 = 0;
               BRANCH2 = 1;
               BRANCH3 = 2;
           };
           required ExampleType example_type = 1;

           message Branch1 {
               required int foo = 1;
               required string blah = 2;
               repeated Example children = 3;
           };
           optional Branch1 branch1;

           optional int branch2;
       };

-}

{-

-- EXAMPLE SYNTAX

  data Bool { branch True; branch False; };

  data Tree {
    branch Node { repeated Tree children; };
    branch Leaf = string;
  };

  message VarTermTuple { string var; Term term; };

  data Term {
    branch Var = string;
    branch Number = int32;
    branch String = string;
    branch Json_Null;

    branch Let {
      repeated VarTermTuple binds;
      Term expr;
    };

    branch If {
      Term test;
      Term true_branch;
      Term false_branch;
    };

    // TODO: Ideally this could be "branch Array = repeated Term".
    branch Array = {
      repeated Term elements;
    };

    branch Call {
      Builtin builtin;
      repeated Term args;
    };

  };

  enum OrderDirection { Ascending; Descending; };
  message Mapping { string arg; Term body; };
  message Predicate { string arg; Term body; };
  message Reduction { Term base; string var1; string var2; Term body; };

  data Builtin {
    branch Not;
    branch GetAttr { string attr; };
    branch HasAttr { string attr; };
    branch PickAttrs { repeated string attrs; };
    branch MapMerge;
    // etc, etc...

    enum Comparison { EQ; NE; LT; LE; GT; GE; };
    branch Comparison = Comparison;

    branch Filter { Predicate predicate; };
    branch Map { Mapping mapping; };
    branch ConcatMap { Mapping mapping; };
    branch Reduce { Reduction reduction; };
    branch OrderBy {
      OrderDirection order_direction;
      Mapping mapping;
    };

    // etc, etc...
  };

-}

import System.IO
import System.Exit

import Data.Char (toUpper, toLower, isUpper)
import Data.List (findIndices, intercalate)

import Control.Monad.State
import Control.Applicative ((<$>), (<*>), (<*), (<$))

import Text.PrettyPrint hiding (char, semi, equals, braces)
import qualified Text.PrettyPrint as PP

import Text.Parsec hiding (State)
import qualified Text.Parsec.Token as P
import Text.Parsec.Language (javaStyle)

-- The main program
main :: IO ()
main = do input <- getContents
          case runP parseProgram () "stdin" input of
            Right result -> hPrint stdout (prettyProgram result)
            Left err -> do hPrint stderr err
                           exitFailure


type Ident = String

newtype Type = TypeName Ident
    deriving Show

type Program = [DeclType]

data DeclType = TypeDeclADT DeclADT
              | TypeDeclMessage DeclMessage
              | TypeDeclEnum DeclEnum
                deriving Show

-- ADTs
data DeclADT = ADT { adtName :: Ident, adtBody :: [ADTDecl] }
               deriving Show

data ADTDecl = ADTDeclBranch DeclBranch
             | ADTDeclType DeclType
               deriving Show

data DeclBranch = Branch { branchName :: Ident, branchBody :: BranchBody }
                  deriving Show

data BranchBody
    -- branch Foo { fields... };
    = BranchMessage [MessageDecl]
    -- branch Foo = int;
    | BranchIsType Type
    -- branch Quux;
    | BranchEmpty
      deriving Show

-- Messages
data DeclMessage = Message { messageName :: Ident
                           , messageBody :: [MessageDecl] }
                   deriving Show

data MessageDecl = MessageDeclType DeclType
                 | MessageDeclField DeclField
                   deriving Show

data DeclField = Field { fieldName :: Ident
                       , fieldType :: Type
                       , fieldModifier :: Modifier }
                 deriving Show

data Modifier = Required | Optional | Repeated
                deriving Show

-- Enums
data DeclEnum = Enum { enumName :: Ident, enumBody :: [EnumDecl] }
                deriving Show

data EnumDecl = EnumDeclTag { tagIdent :: Ident }
                deriving Show


-- "Compiler" (translates ADTs into other things)
compileADT :: DeclADT -> DeclMessage
compileADT (ADT name decls) = Message name mdecls
    where mdecls = MessageDeclType (TypeDeclEnum $ Enum enumName enumTags) :
                   MessageDeclField enumField :
                   concatMap transDecl decls
          enumName = name ++ "Type"
          enumTags = [EnumDeclTag (map toUpper (branchName b))
                          | ADTDeclBranch b <- decls]
          enumField = Field (toFieldName enumName) (TypeName enumName) Required
          transDecl :: ADTDecl -> [MessageDecl]
          transDecl (ADTDeclType t) = [MessageDeclType t]
          transDecl (ADTDeclBranch b) = compileBranch b

compileBranch :: DeclBranch -> [MessageDecl]
compileBranch (Branch name body) =
    case body of BranchEmpty -> []
                 BranchIsType t -> [field t]
                 BranchMessage ds ->
                     [ MessageDeclType (TypeDeclMessage (Message mtypename ds))
                     , field (TypeName mtypename) ]
     where
       field typ = MessageDeclField (Field (toFieldName name) typ Optional)
       mtypename = name

-- Transforms a type name "FooBarBaz" to a field name "foo_bar_baz"
toFieldName :: Ident -> Ident
toFieldName x = intercalate "_" $ map (map toLower) $ splitOn isUpper x

-- FIXME: splitOn is probably too lazy; may require O(n^2) memory?
splitOn :: (a -> Bool) -> [a] -> [[a]]
splitOn p s = splitter is s
    where is = findIndices p s
          splitter [] s = [s]
          splitter (0:is) s = splitter is s
          splitter (i:is) s = fst : splitter (map (\x -> x - i) is) rest
              where (fst,rest) = splitAt i s


-- Pretty-printer (outputs protobuf)
indent = nest 4
block preamble body = vcat [preamble <+> lbrace, indent body, rbrace <> PP.semi]

prettyProgram :: Program -> Doc
prettyProgram decls = vcat $ map prettyDeclType decls

prettyDeclType :: DeclType -> Doc
prettyDeclType (TypeDeclADT d) = prettyDeclADT d
prettyDeclType (TypeDeclMessage d) = prettyDeclMessage d
prettyDeclType (TypeDeclEnum d) = prettyDeclEnum d

prettyDeclADT :: DeclADT -> Doc
prettyDeclADT adt = prettyDeclMessage (compileADT adt)

prettyDeclMessage :: DeclMessage -> Doc
prettyDeclMessage (Message name decls) = block pre body
    where pre = text "message" <+> text name
          body = vcat $ evalState (mapM prettyMessageDecl decls) 1

prettyMessageDecl :: MessageDecl -> State Int Doc
prettyMessageDecl (MessageDeclType d) = return (prettyDeclType d)
prettyMessageDecl (MessageDeclField d) = prettyDeclField d

prettyDeclField :: DeclField -> State Int Doc
prettyDeclField (Field name typ mod) = do
  field_id <- get
  put (field_id + 1)
  return $ hsep [prettyModifier mod, prettyType typ, text name, PP.equals,
                 text (show field_id) <> PP.semi]

prettyType :: Type -> Doc
prettyType (TypeName n) = text n

prettyModifier :: Modifier -> Doc
prettyModifier Required = text "required"
prettyModifier Optional = text "optional"
prettyModifier Repeated = text "repeated"

prettyDeclEnum :: DeclEnum -> Doc
prettyDeclEnum (Enum name decls) = block pre (vcat pdecls)
    where pre = text "enum" <+> text name
          pdecls = evalState (mapM prettyEnumDecl decls) 0

prettyEnumDecl :: EnumDecl -> State Int Doc
prettyEnumDecl (EnumDeclTag id) = do
  tag_id <- get
  put (tag_id + 1)
  return $ hsep [text id, PP.equals, text (show tag_id) <> PP.semi]


-- Parser
type Parse a = Parsec String () a

reservedIds = words "data message enum branch required optional repeated"
operators = ["="]

languageDef :: P.LanguageDef ()
languageDef = javaStyle { P.nestedComments = False
                        , P.reservedNames = reservedIds
                        , P.caseSensitive = True
                        , P.reservedOpNames = operators
                        }

lexer = P.makeTokenParser languageDef
parens = P.parens lexer
braces = P.braces lexer
ident = P.identifier lexer
reserved = P.reserved lexer
semi = P.semi lexer
whiteSpace = P.whiteSpace lexer
equals = P.reservedOp lexer "="

parseBlock p = braces p <* semi

-- Actual parsers
parseProgram :: Parse Program
parseProgram = whiteSpace >> many parseDeclType <* eof

parseType :: Parse Type
parseType = TypeName <$> ident

parseDeclType :: Parse DeclType
parseDeclType = choice [ TypeDeclADT <$> parseDeclADT
                       , TypeDeclMessage <$> parseDeclMessage
                       , TypeDeclEnum <$> parseDeclEnum ]

parseDeclADT :: Parse DeclADT
parseDeclADT = do reserved "data"
                  ADT <$> ident <*> parseBlock (many parseADTDecl)

parseADTDecl :: Parse ADTDecl
parseADTDecl = (ADTDeclBranch <$> parseDeclBranch) <|>
               (ADTDeclType <$> parseDeclType)

parseDeclBranch :: Parse DeclBranch
parseDeclBranch = do reserved "branch"
                     Branch <$> ident <*> parseBranchBody

parseBranchBody :: Parse BranchBody
parseBranchBody = choice [ BranchMessage <$> parseMessageBody
                         , BranchIsType <$> (equals >> parseType <* semi)
                         , BranchEmpty <$ semi ]

-- messages
parseDeclMessage :: Parse DeclMessage
parseDeclMessage = do reserved "message"
                      Message <$> ident <*> parseMessageBody

parseMessageBody :: Parse [MessageDecl]
parseMessageBody = parseBlock (many parseMessageDecl)

parseMessageDecl :: Parse MessageDecl
parseMessageDecl = (MessageDeclType <$> parseDeclType) <|>
                   (MessageDeclField <$> parseDeclField)

parseDeclField :: Parse DeclField
parseDeclField = do mod <- option Required parseModifier
                    typ <- parseType
                    name <- ident
                    semi
                    return $ Field name typ mod

parseModifier :: Parse Modifier
parseModifier = choice [ Required <$ reserved "required"
                       , Optional <$ reserved "optional"
                       , Repeated <$ reserved "repeated" ]

-- enums
parseDeclEnum :: Parse DeclEnum
parseDeclEnum = do reserved "enum"
                   Enum <$> ident <*> parseBlock (many parseEnumDecl)

parseEnumDecl :: Parse EnumDecl
parseEnumDecl = EnumDeclTag <$> ident <* semi
