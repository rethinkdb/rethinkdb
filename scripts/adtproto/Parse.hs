module Parse where

import ADTProto

import Control.Applicative ((<$>), (<*>), (<*), (<$))

import Text.Parsec hiding (State)
import qualified Text.Parsec.Token as P
import Text.Parsec.Language (javaStyle)

-- Parser
type Parse a = Parsec String () a

reservedIds = words "data message enum branch required optional repeated check"
operators = ["="]

languageDef :: P.LanguageDef ()
languageDef = javaStyle { P.nestedComments = False
                        , P.reservedNames = reservedIds
                        , P.caseSensitive = True
                        , P.reservedOpNames = operators
                        }

lexer = P.makeTokenParser languageDef

braces = P.braces lexer
comma = P.comma lexer
equals = P.reservedOp lexer "="
ident = P.identifier lexer
parens = P.parens lexer
reserved = P.reserved lexer
semi = P.semi lexer
whiteSpace = P.whiteSpace lexer

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
                  ADT <$> ident <*> parseBlock (concat <$> many parseADTDecl)

parseADTDecl :: Parse [ADTDecl]
parseADTDecl = choice [ map ADTDeclBranch <$> parseDeclBranch
                      , (:[]) . ADTDeclType <$> parseDeclType
                      , (:[]) . ADTDeclField <$> parseDeclField
                      , (:[]) . ADTDeclCheck <$> parseDeclCheck
                      ]

parseDeclBranch :: Parse [DeclBranch]
parseDeclBranch = do reserved "branch"
                     id <- ident
                     choice [ (:[]) . BranchMessage id <$> parseMessageBody
                            , equals >> branchIsType id <* semi
                            , branchesEmpty id <* semi ]
    where branchIsType id = do mod <- option Required parseModifier
                               typ <- parseType
                               return [BranchIsType id typ mod]
          branchesEmpty id = map BranchEmpty . (id:) <$> many (comma >> ident)

parseDeclCheck :: Parse DeclCheck
parseDeclCheck = do reserved "check"
                    DeclCheck <$> parseCXXBlock <* (whiteSpace >> semi)

-- embedding C++ is a hack: we just check that the number of braces match up.
-- FIXME: this currently includes braces inside string C++ string literals and
-- comments.
parseCXXBlock :: Parse String
parseCXXBlock = between (char '{') (char '}') parseCXXFragment

parseCXXFragment :: Parse String
parseCXXFragment = do cs <- many (noneOf "{}")
                      choice [ cs <$ lookAhead (char '}')
                             , do body <- parseCXXBlock
                                  rest <- parseCXXFragment
                                  return $ cs ++ "{" ++ body ++ "}" ++ rest ]

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
