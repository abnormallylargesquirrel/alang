import System.Environment
import Data.Char

--lexer types
type Token = String
type Parser a = [Token] -> [(a, [Token])]

--parser
data PartialExpr = NoOp | FoundOp String CoreExpr
data Expr a =
    EVar String
    | ENum Int
    | EConstr Int Int
    | EAp (Expr a) (Expr a)
    | ELet
        IsRec
        [(a, Expr a)]
        (Expr a)
    | ECase
        (Expr a)
        [Alter a]
    | ELam [a] (Expr a)
    deriving (Show, Read)

type IsRec = Bool
recursive, nonRecursive :: IsRec
recursive = True
nonRecursive = False

type Alter a = (Int, [a], Expr a)

type CoreAlt = Alter String
type CoreExpr = Expr String

type ScDef a = (String, [a], Expr a)
type CoreScDef = ScDef String

type Program a = [ScDef a]
type CoreProgram = Program String

isAtomicExpr :: Expr a -> Bool
isAtomicExpr (EVar v) = True
isAtomicExpr (ENum n) = True
isAtomicExpr e = False

pSat :: (String -> Bool) -> Parser String
pSat pred [] = []
pSat pred (x:xs)
    | pred x = [(x, xs)]
    | otherwise = []

pLit :: String -> Parser String
pLit s = pSat (== s)

keywords :: [String]
keywords = ["let", "letrec", "in", "case", "of"]

make_sc :: String -> [String] -> a -> CoreExpr -> CoreScDef
make_sc name args eq expr = (name, args, expr)

makeOp :: CoreExpr -> PartialExpr -> CoreExpr
makeOp e1 NoOp = e1
makeOp e1 (FoundOp op e2) = EAp (EAp (EVar op) e1) e2

pEmpty :: a -> Parser a
pEmpty a = (\toks -> [(a, toks)])

pApply :: Parser a -> (a -> b) -> Parser b
pApply p f toks = [(f val, toks1) | (val, toks1) <- p toks]

pParens :: Parser a -> Parser a
pParens p = let combine x y z = y in pThen3 combine (pLit "(") p (pLit ")")

pVar :: Parser String
pVar = pSat (\tok -> (let h = head tok in isAlpha h || h == '_')
        && not (elem tok keywords))

pEVar :: Parser (Expr a)
pEVar = pApply pVar (\x -> EVar x)

pNum :: Parser String
pNum = pSat (\tok -> all isDigit tok)

pENum :: Parser (Expr a)
pENum = pApply pNum (\x -> ENum (read x))

pAlt :: Parser a -> Parser a -> Parser a
pAlt p1 p2 toks = (p1 toks) ++ (p2 toks)

pThen :: (a -> b -> c) -> Parser a -> Parser b -> Parser c
pThen combine p1 p2 toks = [(combine v1 v2, toks2) |
    (v1, toks1) <- p1 toks,
    (v2, toks2) <- p2 toks1]

pThen3 :: (a -> b -> c -> d) -> Parser a -> Parser b -> Parser c -> Parser d
pThen3 combine p1 p2 p3 toks = [(combine v1 v2 v3, toks3) |
    (v1, toks1) <- p1 toks,
    (v2, toks2) <- p2 toks1,
    (v3, toks3) <- p3 toks2]

pThen4 :: (a -> b -> c -> d -> e) -> Parser a -> Parser b -> Parser c ->
    Parser d -> Parser e
pThen4 combine p1 p2 p3 p4 toks = [(combine v1 v2 v3 v4, toks4) |
    (v1, toks1) <- p1 toks,
    (v2, toks2) <- p2 toks1,
    (v3, toks3) <- p3 toks2,
    (v4, toks4) <- p4 toks3]

pZeroOrMore_internal :: Parser a -> Parser [a]
pZeroOrMore_internal p = pAlt (pOneOrMore_internal p) (pEmpty [])

pZeroOrMore p = take 1 . pZeroOrMore_internal p

pOneOrMore_internal :: Parser a -> Parser [a]
pOneOrMore_internal p = pThen (\x xs -> x:xs) p (pZeroOrMore_internal p)

pOneOrMore p = take 1 . pOneOrMore_internal p

pOneOrMoreWithSep :: Parser a -> Parser b -> Parser [a]
pOneOrMoreWithSep p1 p2 = pThen (\x xs -> x:xs) p1 (pZeroOrMore (pThen (\x y -> y) p2 p1))

--expression parsing
pAExpr :: Parser CoreExpr
pAExpr = pAlt (pAlt pENum (pParens pExpr4)) pEVar -- add "just one" parameter, like c++

pExpr :: Parser CoreExpr
pExpr = pApply (pOneOrMore pAExpr) make_ap_chain

--operator precedence parsing
make_ap_chain :: [CoreExpr] -> CoreExpr
make_ap_chain [x] = x
make_ap_chain (x:xs) = EAp (make_ap_chain xs) x

pExpr4 :: Parser CoreExpr
pExpr4 = pThen makeOp pExpr5 pExpr4c

pExpr4c :: Parser PartialExpr
pExpr4c = pAlt (pAlt
        (pThen FoundOp (pLit "+") pExpr4)
        (pThen FoundOp (pLit "-") pExpr5))
        (pEmpty NoOp)

pExpr5 :: Parser CoreExpr
pExpr5 = pThen makeOp pExpr6 pExpr5c

pExpr5c :: Parser PartialExpr
pExpr5c = pAlt (pAlt
        (pThen FoundOp (pLit "*") pExpr5)
        (pThen FoundOp (pLit "/") pExpr6))
        (pEmpty NoOp)

pExpr6 = pApply (pOneOrMore pAExpr) make_ap_chain

pSc :: Parser CoreScDef
pSc = pThen4 make_sc pVar (pZeroOrMore pVar) (pLit "=") pExpr

pProgram :: Parser CoreProgram
pProgram = pOneOrMoreWithSep pSc (pLit ";")

syntax :: [Token] -> CoreProgram
syntax = take_first_parse . pProgram
    where
        take_first_parse ((prog, []):others) = prog
        take_first_parse ((prog, [";"]):others) = prog
        take_first_parse (parse:others) = take_first_parse others -- unnecessary?
        take_first_parse other = error "Syntax error"

--lexer
clex :: String -> [Token]
clex [] = []
clex (x:xs)
    | isSpace x = clex xs
    | isDigit x = let num_token = x : takeWhile isDigit xs
                      rest_xs = dropWhile isDigit xs
                  in
                    num_token : clex rest_xs
    | isFirstIdChar x = let var_token = x : takeWhile isIdChar xs
                            rest_xs = dropWhile isIdChar xs
                        in
                            var_token : clex rest_xs
    | otherwise = [x] : clex xs
    where
        isFirstIdChar x = isAlpha x || x == '_'
        isIdChar x = isFirstIdChar x || isDigit x

parse :: String -> CoreProgram
parse = syntax . clex

main :: IO()
main = do
    --putStrLn . show . head . pOneOrMore (pLit "a") $ (replicate 10000 "a")
    [name] <- getArgs
    contents <- readFile name
    putStrLn . show . parse $ contents
