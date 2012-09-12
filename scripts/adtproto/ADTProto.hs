module ADTProto where

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
             | ADTDeclField DeclField
             | ADTDeclCheck DeclCheck
               deriving Show

data DeclBranch
    -- branch Foo { fields... };
    = BranchMessage Ident [MessageDecl]
    -- branch Foo = int;
    | BranchIsType Ident Type Modifier
    -- branch Quux, Xyzzy, Plugh;
    | BranchEmpty Ident
      deriving Show

branchName :: DeclBranch -> Ident
branchName (BranchMessage i _) = i
branchName (BranchIsType i _ _) = i
branchName (BranchEmpty i) = i

newtype DeclCheck = DeclCheck String -- just some C++ code
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
                deriving (Eq, Show)

-- Enums
data DeclEnum = Enum { enumName :: Ident, enumBody :: [EnumDecl] }
                deriving Show

data EnumDecl = EnumDeclTag { tagIdent :: Ident }
                deriving Show
