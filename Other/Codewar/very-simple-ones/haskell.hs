module YesNo where

boolToWord :: Bool -> String
boolToWord True  = "Yes"
boolToWord False = "No"


module Stringify where

numberToString :: Int -> String
numberToString num = show num
