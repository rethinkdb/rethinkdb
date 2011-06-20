module Metadata where

import Cluster (NodeID)
import Semilattice as SL
import qualified Data.Map as Map
import Data.Map (Map)

{- `GUID` is just a `String` for modelling purposes -}

data GUID = GUID String deriving (Eq, Ord, Show)

{- These data structures are for the specific situation where the user
requests immediate consistency and wants writes to be asynchronously
replicated to the secondary data centers. If the user wants synchronous
replication or eventual consistency, these data structures would have
to be defined differently. -}

data VersionID = VersionID String deriving (Eq, Ord, Show)

data Database = Database {
    databaseVersions :: SL.Set VersionID Version
    }
    deriving (Eq, Ord, Show)

instance Semilattice Database where
    a ^* b = Database {
        databaseVersions = databaseVersions a ^* databaseVersions b
        }

data Key = Key String deriving (Ord, Eq, Show)

data Version = Version {
    -- Every `versionShards` map has the same keys
    versionShards :: Map Key Shard
    }
    deriving (Eq, Ord, Show)

instance Semilattice Version where
    a ^* b = Version {
        versionShards = Map.unionWith (^*) (versionShards a) (versionShards b)
        }

data Shard = Shard {
    shardPrimaryNode :: SL.VersionedValue (Maybe NodeID),
    shardSecondaryNodes :: SL.Set GUID NodeID
    }
    deriving (Eq, Ord, Show)

instance Semilattice Shard where
    a ^* b = Shard {
        shardPrimaryNode = shardPrimaryNode a ^* shardPrimaryNode b,
        shardSecondaryNodes = shardSecondaryNodes a ^* shardSecondaryNodes b
        }

instance Semilattice a => Semilattice (Maybe a) where
    Nothing ^* Nothing = Nothing
    Just a ^* Just b = Just (a ^* b)
    _ ^* _ = error "cannot join"

instance Semilattice NodeID where
    a ^* b = if a == b then a else error "cannot join"
