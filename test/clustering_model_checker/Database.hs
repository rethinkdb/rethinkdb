{-# LANGUAGE MultiParamTypeClasses #-}

module Database where

import Cluster
import Control.Monad
import qualified Data.Map as Map
import Data.Map (Map)
import qualified Metadata as MD
import qualified Semilattice as SL

type DatabaseCluster = Cluster NodeState Message
type DatabaseAction a = Action NodeState Message a

-- For simplicity, values are lists of strings and writes always append a new
-- integer to the list. This is enough to check that writes are delivered in
-- the correct order.
data Value = Value [String] deriving (Ord, Eq, Show)
data Mutation = Mutation String deriving (Ord, Eq, Show)
applyMutation (Mutation i) (Value l) = Value (i:l)

-- `NodeState` is the in-memory state of a node in the cluster.
data NodeState = NodeState {
    -- `nodeMetadata` is the node's current view of the metadata.
    nodeMetadata :: MD.Database,
    -- `nodeShards` are the database shards currently present on the node.
    nodeShards :: Map (MD.VersionID, MD.Key) ShardState,
    -- `nodeHubs` are the data structures necessary for acting as a master
    nodeHubs :: Map (MD.VersionID, MD.Key) HubState
    }
    deriving (Eq, Ord)

-- Getter/setter methods for convenience. Intended to be used in the
-- `ClusterAction` monad.
getNodeMetadata = do
    state <- getNodeState
    return (nodeMetadata state)
putNodeMetadata x = do
    state <- getNodeState
    putNodeState (state { nodeMetadata = x })
getNodeShards = do
    state <- getNodeState
    return (nodeShards state)
putNodeShards x = do
    state <- getNodeState
    putNodeState (state { nodeShards = x })
getNodeMaybeShard (version, key) = do
    shards <- getNodeShards
    return (Map.lookup (version, key) shards)
putNodeMaybeShard (version, key) x = do
    shards <- getNodeShards
    let shards' = case x of
            Just shard -> Map.insert (version, key) shard shards
            Nothing -> Map.delete (version, key) shards
    putNodeShards shards'
getNodeShard (version, key) = do
    maybeShard <- getNodeMaybeShard (version, key)
    case maybeShard of
        Just shard -> return shard
        Nothing -> fail ("not a secondary for " ++ show (version, key))
putNodeShard (version, key) x = putNodeMaybeShard (version, key) (Just x)
getNodeHubs = do
    state <- getNodeState
    return (nodeHubs state)
putNodeHubs x = do
    state <- getNodeState
    putNodeState (state { nodeHubs = x })
getNodeMaybeHub (version, key) = do
    hubs <- getNodeHubs
    return (Map.lookup (version, key) hubs)
putNodeMaybeHub (version, key) x = do
    hubs <- getNodeHubs
    let hubs' = case x of
            Just hub -> Map.insert (version, key) hub hubs
            Nothing -> Map.delete (version, key) hubs
    putNodeHubs hubs'
getNodeHub (version, key) = do
    maybeHub <- getNodeMaybeHub (version, key)
    case maybeHub of
        Just hub -> return hub
        Nothing -> fail ("not a primary for " ++ show (version, key))
putNodeHub (version, key) x = putNodeMaybeHub (version, key) (Just x)

getNodeMetadataVersionShard :: (MD.VersionID, MD.Key) -> DatabaseAction MD.Shard
getNodeMetadataVersionShard (versionID, key) = do
    metadata <- getNodeMetadata
    case SL.lookup versionID (MD.databaseVersions metadata) of
        Just version -> case Map.lookup key (MD.versionShards version) of
            Just shard -> return shard
            Nothing -> error ("no such key: " ++ show key)
        Nothing -> fail ("no such version: " ++ show versionID)

-- `ShardState` is the in-memory state of a shard on a node.
data ShardState = ShardState {
    shardValue :: Value,
    shardCounter :: Integer
    }
    deriving (Eq, Ord)

-- `HubState` is the in-memory state of a hub on a node
data HubState = HubState {
    hubCounter :: Integer,
    hubQueues :: Map NodeID [(Integer, Mutation)]
    }
    deriving (Eq, Ord)

getNodeHubCounter (version, key) = do
    hub <- getNodeHub (version, key)
    return (hubCounter hub)
putNodeHubCounter (version, key) x = do
    hub <- getNodeHub (version, key)
    putNodeHub (version, key) (hub { hubCounter = x })
getNodeHubQueues (version, key) = do
    hub <- getNodeHub (version, key)
    return (hubQueues hub)
putNodeHubQueues (version, key) x = do
    hub <- getNodeHub (version, key)
    putNodeHub (version, key) (hub { hubQueues = x })
getNodeHubMaybeQueue (version, key) node = do
    queues <- getNodeHubQueues (version, key)
    return (Map.lookup node queues)
putNodeHubMaybeQueue (version, key) node x = do
    queues <- getNodeHubQueues (version, key)
    let queues' = case x of
            Just queue -> Map.insert node queue queues
            Nothing -> Map.delete node queues
    putNodeHubQueues (version, key) queues'
getNodeHubQueue (version, key) node = do
    maybeQueue <- getNodeHubMaybeQueue (version, key) node
    case maybeQueue of
        Just queue -> return queue
        Nothing -> fail ("missing queue for " ++ show (version, key) ++ ", " ++ show node)
putNodeHubQueue (version, key) node x = putNodeHubMaybeQueue (version, key) node (Just x)

data Message
    = MetadataMsg MD.Database
    | UnorderedWriteMsg (MD.VersionID, MD.Key) Mutation
    | WriteMsg (MD.VersionID, MD.Key) Integer Mutation
    deriving (Eq, Ord, Show)

onWriteFromClient :: (MD.VersionID, MD.Key) -> Mutation -> DatabaseAction ()
onWriteFromClient (versionID, key) mutation = do
    shard <- getNodeMetadataVersionShard (versionID, key)
    case SL.latestVersion (MD.shardPrimaryNode shard) of
        Just primary -> do
            primaryUp <- canSendMessage primary
            if primaryUp
                then sendMessage primary (UnorderedWriteMsg (versionID, key) mutation)
                else fail ("primary down for " ++ show (versionID, key))   -- TODO improve this
        Nothing -> fail ("no primary for " ++ show (versionID, key))

instance ClusterModel NodeState Message where

    onMessage _ (MetadataMsg metadataA) = do
        metadataB <- getNodeMetadata
        let metadataAB = (SL.^*) metadataA metadataB
        putNodeMetadata (metadataAB)

    onMessage _ (UnorderedWriteMsg (versionID, key) mutation) = do
        nextCounter <- getNodeHubCounter (versionID, key)
        putNodeHubCounter (versionID, key) (nextCounter + 1)
        shard <- getNodeMetadataVersionShard (versionID, key)
        forM_ (Map.elems $ SL.slActive (MD.shardSecondaryNodes shard)) $ \ secondary -> do
            accessible <- canSendMessage secondary
            when accessible $ do
                sendMessage secondary (WriteMsg (versionID, key) nextCounter mutation)
            queue <- getNodeHubQueue (versionID, key) secondary
            let queue' = queue ++ [(nextCounter, mutation)]
            putNodeHubQueue (versionID, key) secondary queue'

    onMessage _ (WriteMsg (versionID, key) counter mutation) = do
        shard <- getNodeShard (versionID, key)
        case compare (shardCounter shard) counter of
            EQ -> do
                let shard' = shard {
                    shardValue = applyMutation mutation (shardValue shard),
                    shardCounter = counter + 1
                    }
                putNodeShard (versionID, key) shard'
            LT -> fail ("skipped operation " ++ show (shardCounter shard))
            GT -> return ()   -- Ignore duplicate

    onMessageAck secondary (WriteMsg (versionID, key) counter mutation) = do
        queue <- getNodeHubQueue (versionID, key) secondary
        case queue of
            ((counter', mut):rest) | counter == counter' -> do
                putNodeHubQueue (versionID, key) secondary rest
            _ -> fail ("unexpected ack for " ++ show counter ++ ", queue = " ++ show queue)

    onMessageAck _ _ = return ()

    onConnectionAlive peer = do
        hubs <- getNodeHubs
        forM_ (Map.keys hubs) $ \ (versionID, key) -> do
            maybeQueue <- getNodeHubMaybeQueue (versionID, key) peer
            case maybeQueue of
                Just q -> do
                    forM_ q $ \ (counter, mutation) -> do
                        sendMessage peer (WriteMsg (versionID, key) counter mutation)
                Nothing -> return ()

    onConnectionDead _ = return ()
