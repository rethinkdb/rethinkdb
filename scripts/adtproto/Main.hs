module Main where

import ADTProto
import Parse (parseProgram)
import Compile (protoProgram, checkProgram)

import System.Environment (getArgs)
import System.Exit (exitFailure)
import System.IO (hPrint, stderr)

import Text.Parsec (runP)

main :: IO ()
main = do [filename] <- getArgs
          input <- readFile filename
          prog <- case runP parseProgram () filename input of
                    Right result -> return result
                    Left err -> do hPrint stderr err
                                   exitFailure
          writeFile (filename ++ ".proto") $ show $ protoProgram prog
          writeFile (filename ++ ".cc") $ show $ checkProgram prog
