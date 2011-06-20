module Scenario1 where

import qualified Cluster
import Cluster (NodeID(..), formatNodeID)
import Control.Monad
import Data.List
import qualified Data.Map as Map
import Database
import qualified Model
import qualified Metadata as MD
import qualified Semilattice as SL

-- Function to compute whether there are outstanding messages waiting on a
-- network connection
outstandingMessages state = or [not (null msgs) || not (null acks)
    | Cluster.ConnectionAlive msgs acks <- Map.elems (Cluster.clusterConnections state)]

-- Fails if the mutation was not successfully delivered to every node
testEndCondition state =
    when (not (null bad))
        (fail ("message not delivered on nodes " ++ intercalate ", " (map formatNodeID bad)))
    where bad = do
            (nodeID, node) <- Map.toList (Cluster.clusterNodes state)
            let nodeState = Cluster.nodeMemory node
            -- Skip this node if the shard is not present on this node
            shard <- case Map.lookup (MD.VersionID "the_version", MD.Key "the_key") (nodeShards nodeState) of
                Just shard -> return shard
                Nothing -> []
            let value = shardValue shard
            -- Skip this node if it has the correct value
            guard (value /= Value ["the_mutation"])
            return nodeID

scenario1step1 = do

    let state0 = Cluster.empty :: DatabaseCluster

    -- Spawn cluster nodes
    let initialMetadata = MD.Database {
            MD.databaseVersions = SL.singleton (MD.VersionID "the_version") (MD.Version {
                MD.versionShards = Map.singleton (MD.Key "the_key") (MD.Shard {
                    MD.shardPrimaryNode = SL.VersionedValue 0 (Just (NodeID "primary")),
                    MD.shardSecondaryNodes = SL.fromList [
                        (MD.GUID "secondary_1_session", NodeID "secondary_1"),
                        (MD.GUID "secondary_2_session", NodeID "secondary_2")
                        ]
                    })
                })
            }
    let state1 = Cluster.join (NodeID "primary") (NodeState {
            nodeMetadata = initialMetadata,
            nodeShards = Map.empty,
            nodeHubs = Map.singleton (MD.VersionID "the_version", MD.Key "the_key") (HubState {
                hubCounter = 0,
                hubQueues = Map.fromList [
                    (NodeID "secondary_1", []),
                    (NodeID "secondary_2", [])
                    ]
                })
            }) state0
    let state2 = Cluster.join (NodeID "secondary_1") (NodeState {
            nodeMetadata = initialMetadata,
            nodeShards = Map.singleton (MD.VersionID "the_version", MD.Key "the_key") (ShardState {
                shardValue = Value [],
                shardCounter = 0
                }),
            nodeHubs = Map.empty
            }) state1
    let state3 = Cluster.join (NodeID "secondary_2") (NodeState {
            nodeMetadata = initialMetadata,
            nodeShards = Map.singleton (MD.VersionID "the_version", MD.Key "the_key") (ShardState {
                shardValue = Value [],
                shardCounter = 0
                }),
            nodeHubs = Map.empty
            }) state2

    -- Establish initial connections
    state4 <- Cluster.connect (NodeID "primary", NodeID "secondary_1") state3
    state5 <- Cluster.connect (NodeID "secondary_1", NodeID "primary") state4
    state6 <- Cluster.connect (NodeID "primary", NodeID "secondary_2") state5
    state7 <- Cluster.connect (NodeID "secondary_2", NodeID "primary") state6
    state8 <- Cluster.connect (NodeID "secondary_1", NodeID "secondary_2") state7
    state9 <- Cluster.connect (NodeID "secondary_2", NodeID "secondary_1") state8

    -- Spawn a write
    (state10, ()) <- Cluster.act
        (onWriteFromClient (MD.VersionID "the_version", MD.Key "the_key") (Mutation "the_mutation"))
        (NodeID "secondary_1")
        state9

    return state10

scenario1step2 = do

    -- Allow messages to be delivered
    state11 <- Model.closure Cluster.step scenario1step1

    -- Check for end condition
    when (not (outstandingMessages state11)) (testEndCondition state11)

    -- Kill a random connection
    victimSrc <- msum $ map return [NodeID "primary", NodeID "secondary_1", NodeID "secondary_2"]
    victimDest <- msum $ map return [NodeID "primary", NodeID "secondary_1", NodeID "secondary_2"]
    guard (victimSrc /= victimDest)
    Model.trace ("killing connection " ++ formatNodeID victimSrc ++ " -> " ++ formatNodeID victimDest)
    state12 <- Cluster.disconnect (victimSrc, victimDest) state11

    return (state12, (victimSrc, victimDest))

scenario1step3 = do

    -- Allow messages to be delivered
    (state13, (victimSrc, victimDest)) <- Model.closure (\ (s, vs) -> Cluster.step s >>= \ s' -> return (s', vs)) scenario1step2

    -- Resurrect the connection
    Model.trace ("resurrecting connection")
    state14 <- Cluster.connect (victimSrc, victimDest) state13

    return state14

scenario1step4 = do

    -- Allow messages to be delivered
    state15 <- Model.closure Cluster.step scenario1step3

    -- Check for end condition
    when (not (outstandingMessages state15)) (testEndCondition state15)

    return ()

scenario1 :: Model.Action ()
scenario1 = scenario1step4
