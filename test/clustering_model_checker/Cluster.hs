{-# LANGUAGE MultiParamTypeClasses #-}

module Cluster where

import Control.Exception
import Control.Monad
import Control.Monad.State
import qualified Data.Map as Map
import Data.Map (Map)
import qualified Model

{- The `Cluster` module simulates a cluster of computers. There are a fixed
number of nodes; each one has a unique `NodeID`.

Nodes can send messages to one another and receive acknowledgements of those
messages. Messages are never reordered, and neither are acknowledgements.
Connections between nodes can "go down". There are considered to be two
connections between any pair of nodes, one going in each direction. Either
one can go down independently. Whenever a connection goes down, any messages or
acks that were on the wire are lost. When a connection goes down or comes back
up, the source end of the connection is notified. A node can also send messages
to itself, but the "connection" to itself cannot ever go down. -}

data NodeID = NodeID String deriving (Ord, Eq, Show)
formatNodeID (NodeID n) = show n

-- `Cluster mem msg` represents the state of a cluster for which the state of a
-- node can be represented as `mem` and a message can be encoded as `msg`.
data Cluster mem msg = Cluster {
    -- `clusterNodes` is the current state of each node
    clusterNodes :: Map NodeID (Node mem),
    -- `clusterConnections` is the current state of each connection. The key is
    -- a pair where the first element is the source node and the second element
    -- is the destination node. They can be the same.
    clusterConnections :: Map (NodeID, NodeID) (Connection msg)
    }
    deriving (Eq, Ord)

-- `Node mem` represents the current state of a node whose in-memory state can
-- be represented as `mem`
data Node mem = Node {
    -- The current contents of the node's memory
    nodeMemory :: mem
    -- Later, there will be a field `nodeDisk` with the contents of the node's
    -- disk
    }
    deriving (Ord, Eq)

-- `Connection msg` represents the current state of one direction of a
-- connection between two nodes.
data Connection msg
    -- The connection is alive.
    = ConnectionAlive {
        -- Messages in transit. First message on this list can be "delivered",
        -- in which case `onMessage` is run on the destination and then the
        -- message is moved to the second list.
        connectionMessages :: [msg],
        -- Messages delivered but not acked yet. First message on this list can
        -- be "acked", in which case `onMessageAck` is run on the source.
        connectionAcks :: [msg]
        }
    -- The connection is dead.
    | ConnectionDead
    deriving (Ord, Eq, Show)

-- `Action` represents an action performed at a cluster node in response
-- to an event. It allows an imperative-ish programming style. It can be used
-- to get/put the local node's `mem`, send messages, and access some connection
-- state information. It also exposes `Model.Action`'s failure, path-tracing, and
-- nondeterminism capabilities.

-- A `Action` is considered to complete atomically and instantaneously
-- on whatever node it runs on. `Action`s are never interleaved; no
-- events occur while a `Action` is running; it is impossible for a
-- `Action` to "wait" for something to happen.

data Action mem msg a = Action {
    act :: NodeID -> Cluster mem msg -> Model.Action (Cluster mem msg, a)
    }

instance Monad (Action mem msg) where
    return x = Action (\ me cluster -> return (cluster, x))
    ma >>= fmb = Action (\ me cluster -> do
        (cluster2, a) <- act ma me cluster
        (cluster3, b) <- act (fmb a) me cluster2
        return (cluster3, b)
        )
    fail msg = Action (\ me cluster -> fail msg)

instance MonadPlus (Action mem msg) where
    mzero = Action (\ me cluster -> mzero)
    a `mplus` b = Action (\ me cluster ->
        act a me cluster `mplus` act b me cluster)

-- Wrapper around `Model.trace` for `Cluster.Action`
trace :: String -> Action mem msg ()
trace msg = Action (\ me cluster -> Model.trace msg >> return (cluster, ()))

getNodeState = Action (\ me cluster ->
    case Map.lookup me (clusterNodes cluster) of
        Just node -> return (cluster, nodeMemory node)
        Nothing -> error ("i don't exist: " ++ formatNodeID me)
        )

putNodeState mem = Action (\ me cluster ->
    case Map.lookup me (clusterNodes cluster) of
        Just node -> let
            cluster2 = cluster {
                clusterNodes = Map.insert
                    me (node { nodeMemory = mem })
                    (clusterNodes cluster)
                }
            in return (cluster2, ())
        Nothing -> error ("i don't exist: " ++ formatNodeID me)
        )

-- `myNodeID` returns the `NodeID` of the node that the `Action` is
-- running on.
myNodeID :: Action mem msg NodeID
myNodeID = Action (\ me cluster -> return (cluster, me))

-- `canSendMessage` returns `True` if the connection from the local node to the
-- given node is up. It always returns true if they are the same node.
canSendMessage :: NodeID -> Action mem msg Bool
canSendMessage dest = Action (\ me cluster ->
    case Map.lookup (me, dest) (clusterConnections cluster) of
        Just (ConnectionAlive { }) -> return (cluster, True)
        Just (ConnectionDead) -> return (cluster, False)
        Nothing -> error ("bad node pair: " ++ formatNodeID me ++ " -> " ++ formatNodeID dest)
        )

-- `sendMessage` puts a message on the queue to go to the given node. It fails
-- if the connection to that node is down. (You can check that using
-- `canSendMessage`.)
sendMessage :: Show msg => NodeID -> msg -> Action mem msg ()
sendMessage dest message = Action (\ me cluster ->
    case Map.lookup (me, dest) (clusterConnections cluster) of
        Just (conn @ ConnectionAlive { }) -> do
            Model.trace ("post (" ++ show message ++ ") " ++ formatNodeID me ++ " -> " ++ formatNodeID dest)
            -- Push the message onto the end of the queue
            let conn2 = conn {
                connectionMessages = (connectionMessages conn) ++ [message]
                }
            let cluster2 = cluster {
                clusterConnections = Map.insert (me, dest) conn2 (clusterConnections cluster)
                }
            return (cluster2, ())
        Just (ConnectionDead) -> fail ("tried to send to unreachable node: " ++
            formatNodeID dest)
        Nothing -> error ("bad node pair: " ++ formatNodeID me ++ " -> " ++ formatNodeID dest)
        )

-- To use the `Cluster` module, you must provide an instance of `ClusterModel`.
-- The `mem` parameter of `ClusterModel` describes the state of a cluster node.
-- The `msg` parameter describes a message from one cluster node to another.
-- The implementations you provide for `onMessage`, `onMessageAck` and so on
-- define how the cluster nodes will behave in response to various events.
class ClusterModel mem msg where
    -- `onMessage` defines how a node responds to a message delivery.
    onMessage :: NodeID -> msg -> Action mem msg ()
    -- `onMessageAck` defines how a node responds to an acknowledgement.
    onMessageAck :: NodeID -> msg -> Action mem msg ()
    -- `onConnectionAlive` defines how a node responds to a connection from
    -- this node to another node being established.
    onConnectionAlive :: NodeID -> Action mem msg ()
    -- `onConnectionDead` defines how a node responds to a connection from
    -- this node to another node being declared dead.
    onConnectionDead :: NodeID -> Action mem msg ()

-- `step` advances the cluster state by delivering messages.
step :: (Show msg, ClusterModel mem msg) => Cluster mem msg -> Model.Action (Cluster mem msg)
step cluster = deliverMessage `mplus` deliverAck
    where
        -- `deliverMessage` delivers a waiting message
        deliverMessage = do
            -- Pick the node to deliver from and the node to deliver to. Note
            -- that they can be the same node.
            src <- msum $ map return (Map.keys (clusterNodes cluster))
            dest <- msum $ map return (Map.keys (clusterNodes cluster))
            -- Check that the connection is alive and there is a waiting message
            case Map.lookup (src, dest) (clusterConnections cluster) of
                -- The connection is alive and there's a waiting message
                Just (conn @ ConnectionAlive {
                        connectionMessages = (m:messages),
                        connectionAcks = acks }) -> do
                    -- Record what choice we made
                    Model.trace ("deliver (" ++ show m ++ ") " ++ formatNodeID src ++ " -> " ++ formatNodeID dest)
                    -- Transfer message from message list to ack list
                    let cluster2 = cluster {
                        clusterConnections = Map.insert (src, dest)
                            (conn { connectionMessages = messages, connectionAcks = m:acks })
                            (clusterConnections cluster)
                        }
                    -- Deliver message
                    (cluster3, ()) <- act (onMessage src m) dest cluster2
                    return cluster3
                -- The connection is dead, or there are no messages waiting to
                -- be delivered
                _ -> mzero
        -- `deliverAck` delivers a waiting ack
        deliverAck = do
            -- Pick the node to deliver from and the node to deliver to. Note
            -- that they can be the same node.
            src <- msum $ map return (Map.keys (clusterNodes cluster))
            dest <- msum $ map return (Map.keys (clusterNodes cluster))
            -- Check that the connection is alive and there is a waiting ack
            case Map.lookup (src, dest) (clusterConnections cluster) of
                -- The connection is alive and there's a waiting ack
                Just (conn @ ConnectionAlive { connectionAcks = (a:acks) }) -> do
                    -- Record what choice we made
                    Model.trace ("ack (" ++ show a ++ ") " ++ formatNodeID src ++ " <- " ++ formatNodeID dest)
                    -- Remove message from ack list
                    let cluster2 = cluster {
                        clusterConnections = Map.insert (src, dest)
                            (conn { connectionAcks = acks })
                            (clusterConnections cluster)
                        }
                    -- Deliver ack (to sender of original message)
                    (cluster3, ()) <- act (onMessageAck dest a) src cluster2
                    return cluster3
                -- The connection is dead, or there are no acks waiting to be
                -- delivered
                _ -> mzero

-- `connect` computes the cluster state after reestablishing a connection
-- between the given pair of nodes
connect :: ClusterModel mem msg => (NodeID, NodeID) -> Cluster mem msg -> Model.Action (Cluster mem msg)
connect (src, dest) cluster =
    assert (src /= dest) $
    -- Make sure the connection is currently down
    case Map.lookup (src, dest) (clusterConnections cluster) of
        Just ConnectionDead -> do
            -- Update connection list
            let cluster2 = cluster {
                clusterConnections = Map.insert
                    (src, dest) (ConnectionAlive [] [])
                    (clusterConnections cluster)
                }
            -- Deliver reconnection notification to sending end of connection
            (cluster3, ()) <- act (onConnectionAlive dest) src cluster2
            return cluster3
        Just (ConnectionAlive { }) ->
            error ("connection from " ++ formatNodeID src ++ " to " ++
                formatNodeID dest ++ " is already up")

-- `disconnect` computes the cluster state after destroying the connection
-- between the given pair of nodes
disconnect :: ClusterModel mem msg => (NodeID, NodeID) -> Cluster mem msg -> Model.Action (Cluster mem msg)
disconnect (src, dest) cluster =
    assert (src /= dest) $
    -- Make sure the connection is currently up
    case Map.lookup (src, dest) (clusterConnections cluster) of
        Just (ConnectionAlive { }) -> do
            -- Update connection list
            let cluster2 = cluster {
                clusterConnections = Map.insert
                    (src, dest) ConnectionDead
                    (clusterConnections cluster)
                }
            -- Deliver death notification to sending end of connection
            (cluster3, ()) <- act (onConnectionDead dest) src cluster2
            return cluster3
        Just (conn @ ConnectionDead) ->
            error ("connection from " ++ formatNodeID src ++ " to " ++
                formatNodeID dest ++ " is already down")

-- `empty` is the empty cluster, with no nodes or anything
empty :: Cluster mem msg
empty = Cluster {
    clusterNodes = Map.empty,
    clusterConnections = Map.empty
    }

-- `join` adds a new node to a cluster. Initially it is connected only to itself.
join :: NodeID -> mem -> Cluster mem msg -> Cluster mem msg
join node state cluster = Cluster {
    -- Insert new node into nodes list
    clusterNodes = Map.insert node (Node { nodeMemory = state }) (clusterNodes cluster),
    -- Insert new connections into connections list
    clusterConnections =
        -- Connection from node to itself
        Map.insert (node, node) (ConnectionAlive [] []) $
        -- Connections from node to others
        Map.union (Map.fromList [((node, dest), ConnectionDead) | dest <- Map.keys (clusterNodes cluster)]) $
        -- Connections from others to node
        Map.union (Map.fromList [((src, node), ConnectionDead) | src <- Map.keys (clusterNodes cluster)]) $
        clusterConnections cluster
    }
