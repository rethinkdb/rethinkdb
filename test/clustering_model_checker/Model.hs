module Model where

import qualified Data.Map as Map
import Data.Map (Map)
import Control.Monad

{- `Action` provides nondeterminism, path-tracing, and failure. -}

type Path = [String]
data WithPath a = WithPath Path a deriving Show

type FailureMessage = String

data Action a = Action {
    actionFailures :: [WithPath FailureMessage],
    actionLeads :: [WithPath a]
    }

addPath :: Path -> WithPath a -> WithPath a
addPath p1 (WithPath p2 x) = WithPath (p1 ++ p2) x

addPathToAction :: Path -> Action a -> Action a
addPathToAction p (Action f l) =
    Action (map (addPath p) f) (map (addPath p) l)

instance Monad Action where

    return x = Action [] [WithPath [] x]

    a >>= b = let
        Action failures leads = a
        Action furtherFailures furtherLeads = msum [
            addPathToAction p (b x)
            | WithPath p x <- leads]
        in Action (failures ++ furtherFailures) furtherLeads

    fail msg = Action [WithPath [] msg] []

instance MonadPlus Action where

    mzero = Action [] []

    a `mplus` b = Action {
        actionFailures = actionFailures a ++ actionFailures b,
        actionLeads = actionLeads a ++ actionLeads b
        }

trace :: String -> Action ()
trace msg = Action [] [WithPath [msg] ()]

{- `closure` computes the transitive closure of the given transition function.
It de-duplicates states that are reached by multiple paths. (That's why it
needs an `Ord` instance: so it can efficiently check if a state has been seen.)
-}

closure :: Ord s => (s -> Action s) -> (Action s -> Action s)
closure transition initial = Action allFailures [WithPath p x | (x, p) <- Map.toList allSeen]
    where
        (newFailures, allSeen) = search (actionLeads initial) Map.empty
        allFailures = actionFailures initial ++ newFailures

        -- Given a list of unexplored states and a set of explored states,
        -- `search` returns a list of failures and a set of explored states
        -- (which is a superset of the original set). With each explored state,
        -- it includes the path to that state.

        -- search :: Ord t => [WithPath t] -> Map t Path -> ([WithPath FailureMessage], Map t Path)

        -- We're done
        search [] seen = ([], seen)
        -- We've been to this state before; skip it
        search (WithPath path state:rest) seen | state `Map.member` seen = search rest seen
        -- We have a new state to explore
        search (WithPath path state:rest) seen = (failures ++ failures', seen'')
            where
                Action failures leads = addPathToAction path (transition state)
                seen' = Map.insert state path seen
                (failures', seen'') = search (rest ++ leads) seen'

printFailures :: Action a -> IO ()
printFailures a = do
    putStrLn "===================="
    forM_ (actionFailures a) $ \ (WithPath path failure) -> do
        forM_ path putStrLn
        putStrLn failure
        putStrLn "===================="
