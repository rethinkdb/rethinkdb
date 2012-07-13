module Compile (protoProgram, checkProgram) where

import ADTProto

import Control.Applicative ((<$>), (<*>), (<*), (<$))
import Control.Monad (when)
import Control.Monad.Reader
import Control.Monad.State
import Control.Monad.Writer

import Data.Char (toUpper, toLower, isUpper)
import Data.List (findIndices, intercalate, intersperse)
-- (<>) gets re-exported by Monad.Writer, originally from Data.Monoid, and is
-- the same (<>) as Text.PrettyPrint uses (since Doc is a monoid). sigh.
import Text.PrettyPrint hiding ((<>))

indent = nest 4
block preamble body = vcat [preamble <+> lbrace, indent body, rbrace <> semi]
vsep = vcat . intersperse (text "")


-- Translate syntax trees into pretty-printed protobufs
protoProgram :: Program -> Doc
protoProgram decls = vsep $ map protoDeclType decls

protoDeclType :: DeclType -> Doc
protoDeclType (TypeDeclADT d) = protoDeclADT d
protoDeclType (TypeDeclMessage d) = protoDeclMessage d
protoDeclType (TypeDeclEnum d) = protoDeclEnum d

protoDeclADT :: DeclADT -> Doc
protoDeclADT adt = protoDeclMessage (transADT adt)

protoDeclMessage :: DeclMessage -> Doc
protoDeclMessage (Message name decls) = block pre body
    where pre = text "message" <+> text name
          body = vcat $ evalState (mapM protoMessageDecl decls) 1

protoMessageDecl :: MessageDecl -> State Int Doc
protoMessageDecl (MessageDeclType d) = return (protoDeclType d)
protoMessageDecl (MessageDeclField d) = protoDeclField d

protoDeclField :: DeclField -> State Int Doc
protoDeclField (Field name typ mod) = do
  field_id <- get
  put (field_id + 1)
  return $ hsep [protoModifier mod, protoType typ, text name, equals,
                 text (show field_id) <> semi]

protoType :: Type -> Doc
protoType (TypeName n) = text n

protoModifier :: Modifier -> Doc
protoModifier Required = text "required"
protoModifier Optional = text "optional"
protoModifier Repeated = text "repeated"

protoDeclEnum :: DeclEnum -> Doc
protoDeclEnum (Enum name decls) = block pre (vcat pdecls)
    where pre = text "enum" <+> text name
          pdecls = evalState (mapM protoEnumDecl decls) 0

protoEnumDecl :: EnumDecl -> State Int Doc
protoEnumDecl (EnumDeclTag id) = do
  tag_id <- get
  put (tag_id + 1)
  return $ hsep [text id, equals, text (show tag_id) <> semi]


-- Generates checking code for an ADT.
type Check a = ReaderT [Ident] (Writer [Doc]) a

getScope :: Check String
getScope = asks (intercalate "_" . reverse)

checkProgram :: Program -> Doc
checkProgram decls = vsep $ execWriter $ runReaderT prog []
    where prog = mapM_ checkDeclType decls

checkDeclType :: DeclType -> Check ()
checkDeclType (TypeDeclADT d) = checkDeclADT d
checkDeclType (TypeDeclMessage d) = checkDeclMessage d
checkDeclType (TypeDeclEnum d) = checkDeclEnum d

checkDeclADT :: DeclADT -> Check ()
checkDeclADT adt = local (adtName adt :) $ do
                     name <- getScope
                     checks <- concat <$> mapM checkADTDecl (adtBody adt)
                     tell [wellformed name checks]

wellformed :: String -> [Doc] -> Doc
wellformed name checks =
    vcat [ text ("bool well_formed(const " ++ name ++ "& self) {")
         , indent $ vsep $ checks ++ [text "return true;"]
         , text "}" ]

checkADTDecl :: ADTDecl -> Check [Doc]
checkADTDecl (ADTDeclBranch d) = checkDeclBranch d
checkADTDecl (ADTDeclType d) = [] <$ checkDeclType d
checkADTDecl (ADTDeclField d) = return []
checkADTDecl (ADTDeclCheck (DeclCheck s)) = return [text s]

checkDeclBranch :: DeclBranch -> Check [Doc]
checkDeclBranch (BranchMessage i decls) = do mapM_ checkMessageDecl decls
                                             branchCheck i Required <$> getScope
checkDeclBranch (BranchIsType i _ mod) = branchCheck i mod <$> getScope
checkDeclBranch (BranchEmpty _) = return []

branchCheck :: Ident -> Modifier -> String -> [Doc]
branchCheck name mod scope = tagImpliesField ++ fieldImpliesTag
    where
      tagImpliesField = [failIf ("self.type == " ++ tag ++ " && !" ++ hasField)
                             | Required == mod]
      fieldImpliesTag = [failIf (hasField ++ " && self.type != " ++ tag)]
      failIf x = text ("if (" ++  x ++ ")") $$ nest 4 (text "return false;")
      tag = scope ++ "::" ++ toTagName name
      field = toFieldName name
      hasField = case mod of Repeated -> "self." ++ field ++ "_size() > 0"
                             _ -> "self.has_" ++ field ++ "()"

checkDeclMessage :: DeclMessage -> Check ()
checkDeclMessage d = mapM_ checkMessageDecl (messageBody d)

checkMessageDecl :: MessageDecl -> Check ()
checkMessageDecl (MessageDeclType d) = checkDeclType d
checkMessageDecl (MessageDeclField d) = return ()

checkDeclEnum :: DeclEnum -> Check ()
checkDeclEnum d = return ()


-- ADT "transformer" (translates ADTs into messages)
transADT :: DeclADT -> DeclMessage
transADT (ADT name decls) = Message name mdecls
    where mdecls = MessageDeclType (TypeDeclEnum $ Enum enumName enumTags) :
                   MessageDeclField enumField :
                   concatMap transDecl decls
          enumName = name ++ "Type"
          enumTags = [EnumDeclTag (toTagName (branchName b))
                          | ADTDeclBranch b <- decls]
          enumField = Field "type" (TypeName enumName) Required
          transDecl :: ADTDecl -> [MessageDecl]
          transDecl (ADTDeclType t) = [MessageDeclType t]
          transDecl (ADTDeclBranch b) = transBranch b
          transDecl (ADTDeclField d) = [MessageDeclField d]
          transDecl (ADTDeclCheck _) = []

transBranch :: DeclBranch -> [MessageDecl]
transBranch b =
    case b of BranchEmpty _ -> []
              BranchIsType name typ mod -> [field name typ mod]
              BranchMessage name decls ->
                  [ MessageDeclType (TypeDeclMessage (Message name decls))
                  , field name (TypeName name) Required ]
     where
       -- NB. "Required" on a branch gets turned into "Optional" b/c it really
       -- means "the field is required IF the branch tag indicates this branch"
       field name typ mod = MessageDeclField (Field (toFieldName name) typ mod')
           where mod' = case mod of Required -> Optional
                                    _ -> mod

-- Transforms a type name "FooBarBaz" to a field name "foo_bar_baz"
toFieldName :: Ident -> Ident
toFieldName x = intercalate "_" $ map (map toLower) $ splitOn isUpper x

toTagName = map toUpper

-- FIXME: splitOn is probably too lazy; may require O(n^2) memory?
splitOn :: (a -> Bool) -> [a] -> [[a]]
splitOn p s = splitter is s
    where is = findIndices p s
          splitter [] s = [s]
          splitter (0:is) s = splitter is s
          splitter (i:is) s = fst : splitter (map (\x -> x - i) is) rest
              where (fst,rest) = splitAt i s
