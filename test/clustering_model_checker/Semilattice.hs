module Semilattice where

import qualified Data.Map as Map
import Data.Map (Map)
import qualified Data.Set

class Semilattice a where

    -- Read "join"
    (^*) :: a -> a -> a

    -- Semilattice laws:
    -- x ^* y = y ^* x
    -- (a ^* b) ^* c = a ^* (b ^* c)
    -- q ^* q = q

{- An `Set` is a set of objects such that addition and removal of objects will
be communicated through the semilattice join operation. -}

data Set k a = Set {
    slActive :: Map k a,
    slDefunct :: Data.Set.Set k
    }
    deriving (Eq, Ord, Show)

lookup :: Ord k => k -> Set k a -> Maybe a
lookup key set = Map.lookup key (slActive set)

singleton :: Ord k => k -> a -> Set k a
singleton k v = Set { slActive = Map.singleton k v, slDefunct = Data.Set.empty }

fromList :: Ord k => [(k, a)] -> Set k a
fromList l = Set { slActive = Map.fromList l, slDefunct = Data.Set.empty }

instance (Ord k, Semilattice a) => Semilattice (Set k a) where
    a ^* b = Set {
            slActive = Map.unionWith (^*) (slActive a \\ defunct) (slActive b \\ defunct),
            slDefunct = defunct
        }
        where
            defunct = Data.Set.union (slDefunct a) (slDefunct b)
            x \\ y = foldr Map.delete x (Data.Set.toList y)

{- A `VersionedValue` is an object that keeps track of when it was changed. -}

data VersionedValue a = VersionedValue {
    slVersion :: Integer,
    slValue :: a
    }
    deriving (Eq, Ord, Show)

latestVersion :: VersionedValue a -> a
latestVersion = slValue

instance Semilattice a => Semilattice (VersionedValue a) where
    a ^* b = case compare (slVersion a) (slVersion b) of
        LT -> b
        EQ -> VersionedValue (slVersion a) (slValue a ^* slValue b)
        GT -> a
